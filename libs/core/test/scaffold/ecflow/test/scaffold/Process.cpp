// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "Process.hpp"

#include <array>
#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <csignal>
#include <fcntl.h>
#include <mutex>
#include <optional>
#include <poll.h>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <utility>

#include <boost/version.hpp>
#include <sys/wait.h>

#if BOOST_VERSION < 106600

    #error "Boost version >= 1.66.0 is required"

#elif BOOST_VERSION >= 106600 && BOOST_VERSION < 108600

    #define BOOST_PROCESS_VERSION 1
    #include <boost/process.hpp>

namespace bp = boost::process;

#else // BOOST_VERSION >= 108600

    #define BOOST_PROCESS_VERSION 2
    #include <boost/asio.hpp>
    #include <boost/process.hpp>

namespace ba = boost::asio;
    #if BOOST_VERSION < 108800
namespace bp = boost::process::v2;
    #else
namespace bp = boost::process;
    #endif

#endif

namespace ecf::test::scaffold {

namespace {

// Blocks until the child has exited, leaving it unreaped, so that its process id remains reserved
void wait_for_exit_without_reaping(int process_id) {
    siginfo_t info{};
    int ret = 0;
    do {
        ret = ::waitid(P_PID, static_cast<id_t>(process_id), &info, WEXITED | WNOWAIT);
    } while (ret == -1 && errno == EINTR);
    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), "waitid(2) failed");
    }
}

///
/// @brief Reads a pipe continuously on a background thread, so that the writing child never blocks on a full pipe.
///
/// A child blocks as soon as it writes more than the pipe holds, which on Linux can be as little as 8 KiB once the
/// user's pipes exceed fs.pipe-user-pages-soft. Reading only after the child has exited would then never end.
///
/// @invariant The thread is the only reader of the pipe; it stops at the end of the stream, or on destruction.
///
class PipeDrain {
public:
    explicit PipeDrain(int fd)
        : fd_{fd},
          reader_{[this] { run(); }} {}

    PipeDrain(const PipeDrain&)            = delete;
    PipeDrain& operator=(const PipeDrain&) = delete;

    ~PipeDrain() {
        stop_ = true;
        reader_.join();
    }

    // Waits until the writers close the pipe, and returns the data not yet taken
    std::string take() {
        std::unique_lock<std::mutex> lock(mutex_);
        closed_.wait(lock, [this] { return eof_; });
        return std::exchange(buffer_, {});
    }

private:
    void run() {
        try {
            read_until_end(true);
        }
        catch (...) {
            // An exception escaping the thread would terminate the test program (e.g. std::bad_alloc while storing
            // the output): the output stored so far is kept, and the rest is read and discarded, so that the child
            // still never blocks on the pipe
            try {
                read_until_end(false);
            }
            catch (...) {
            }
        }
        {
            std::scoped_lock lock(mutex_);
            eof_ = true;
        }
        closed_.notify_all();
    }

    void read_until_end(bool store) {
        std::array<char, 65536> chunk{};
        while (!stop_) {
            pollfd request{fd_, POLLIN, 0};
            // A bounded poll, so that destruction stops the thread even while a writer keeps the pipe open
            int ready = ::poll(&request, 1, 100);
            if (ready == 0 || (ready < 0 && errno == EINTR)) {
                continue;
            }
            if (ready < 0) {
                break;
            }
            ssize_t count = ::read(fd_, chunk.data(), chunk.size());
            if (count > 0) {
                if (store) {
                    std::scoped_lock lock(mutex_);
                    buffer_.append(chunk.data(), static_cast<std::size_t>(count));
                }
                continue;
            }
            if (count < 0 && (errno == EINTR || errno == EAGAIN)) {
                continue;
            }
            break; // End of the stream, or an error
        }
    }

    int fd_;
    std::atomic<bool> stop_{false};
    std::mutex mutex_;
    std::condition_variable closed_;
    bool eof_{false};
    std::string buffer_;
    std::thread reader_; // Last, so that it starts once every other member is ready
};

} // namespace

///
/// @brief Shares child identity, native-wait ownership and cached completion across process backends.
///
/// @invariant exit_status is set exactly once, under status_mutex; wait_mutex serialises the native waits.
///
struct ProcessLifecycle
{
    int process_id{0};
    std::mutex wait_mutex;
    std::mutex status_mutex;
    std::optional<int> exit_status;
    bool output_to_file{false}; ///< True when the output goes to a file, and no pipe is there to read
};

#if BOOST_PROCESS_VERSION == 1

struct Process::Impl : ProcessLifecycle
{
    Impl(std::string_view executable,
         const std::vector<std::string>& args,
         const fs::path& cwd,
         const std::optional<fs::path>& output)
        : out_{},
          err_{},
          handle_{} {
        // Determine the invocation command
        auto exe  = std::string{executable};
        auto argv = std::vector<std::string>{args.begin(), args.end()};

        // Start the process
        if (output) {
            handle_ = bp::child{
                exe, bp::args(argv), (bp::std_out & bp::std_err) > output->string(), bp::start_dir(cwd.string())};
            output_to_file = true;
        }
        else {
            handle_ =
                bp::child{exe, bp::args(argv), bp::std_out > out_, bp::std_err > err_, bp::start_dir(cwd.string())};
            out_drain_.emplace(out_.pipe().native_source());
            err_drain_.emplace(err_.pipe().native_source());
        }
        process_id = handle_.id();
    }
    ~Impl() = default;

    int wait_native() {
        // Reap without bp::child::wait(), which on older Boost throws for a signalled child after reaping it,
        // and reports a signalled child as a successful exit
        int status = 0;
        pid_t ret  = 0;
        do {
            ret = ::waitpid(process_id, &status, 0);
        } while (ret == -1 && errno == EINTR);
        if (ret == -1) {
            throw std::system_error(errno, std::generic_category(), "waitpid(2) failed");
        }
        // The child is reaped, so the handle must no longer probe or signal its process id
        handle_.detach();

        // Match the exit status reported by the Boost.Process v2 backend
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            return WTERMSIG(status);
        }
        return status;
    }

    std::string read_stdout() { return output_to_file ? std::string{} : out_drain_->take(); }

    std::string read_stderr() { return output_to_file ? std::string{} : err_drain_->take(); }

    bp::ipstream out_;
    bp::ipstream err_;

    bp::child handle_;

    // Declared last, so that the readers stop before the pipes close
    std::optional<PipeDrain> out_drain_;
    std::optional<PipeDrain> err_drain_;
};

#elif BOOST_PROCESS_VERSION == 2

struct Process::Impl : ProcessLifecycle
{
    Impl(std::string_view executable,
         const std::vector<std::string>& args,
         const fs::path& cwd,
         const std::optional<fs::path>& output)
        : ctx_{},
          stdout_pipe_{ctx_},
          stderr_pipe_{ctx_},
          start_dir_{cwd.c_str()},
          handle_{launch(executable, args, output)} {
        process_id = handle_.id();
        if (!output_to_file) {
            out_drain_.emplace(stdout_pipe_.native_handle());
            err_drain_.emplace(stderr_pipe_.native_handle());
        }
    }

    bp::process
    launch(std::string_view executable, const std::vector<std::string>& args, const std::optional<fs::path>& output) {
        if (!output) {
            return bp::process{
                ctx_, executable, args, start_dir_, bp::process_stdio{nullptr, {stdout_pipe_}, {stderr_pipe_}}};
        }

        // A single file description serves both outputs, so that they share one offset and never overwrite each other
        int fd = ::open(output->c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
        if (fd == -1) {
            throw std::system_error(errno, std::generic_category(), "open(2) failed for " + output->string());
        }
        output_to_file = true;
        try {
            auto process = bp::process{ctx_, executable, args, start_dir_, bp::process_stdio{nullptr, fd, fd}};
            ::close(fd);
            return process;
        }
        catch (...) {
            ::close(fd);
            throw;
        }
    }
    ~Impl() = default;

    int wait_native() { return handle_.wait(); }

    std::string read_stdout() { return output_to_file ? std::string{} : out_drain_->take(); }

    std::string read_stderr() { return output_to_file ? std::string{} : err_drain_->take(); }

    ba::io_context ctx_;
    ba::readable_pipe stdout_pipe_;
    ba::readable_pipe stderr_pipe_;
    bp::process_start_dir start_dir_;

    bp::process handle_;

    // Declared last, so that the readers stop before the pipes close
    std::optional<PipeDrain> out_drain_;
    std::optional<PipeDrain> err_drain_;
};

#else
    #error "Unsupported boost::process version"
#endif

Process::Process()
    : impl_{nullptr} {
}

Process::Process(Process&& rhs)
    : impl_{std::move(rhs.impl_)} {
    rhs.impl_ = nullptr;
}

Process::Process(const fs::path& executable, std::vector<std::string> args, fs::path cwd)
    : impl_{std::make_unique<Impl>(executable.c_str(), args, cwd, std::nullopt)} {
}

Process::Process(const fs::path& executable, std::vector<std::string> args, fs::path cwd, const fs::path& output)
    : impl_{std::make_unique<Impl>(executable.c_str(), args, cwd, output)} {
}

Process& Process::operator=(Process&& rhs) {
    if (this != &rhs) {
        impl_     = std::move(rhs.impl_);
        rhs.impl_ = nullptr;
    }
    return *this;
}

Process::~Process() = default;

int Process::pid() const {
    if (!impl_) {
        throw std::logic_error("Process::pid() requires an owned child process");
    }
    return impl_->process_id;
}

int Process::wait() {
    if (!impl_) {
        throw std::logic_error("Process::wait() requires an owned child process");
    }
    std::lock_guard<std::mutex> waiter(impl_->wait_mutex);
    {
        std::lock_guard<std::mutex> status(impl_->status_mutex);
        if (impl_->exit_status) {
            return *impl_->exit_status;
        }
    }

    wait_for_exit_without_reaping(impl_->process_id);

    // Reap and record under the status lock: terminate() signals under the same lock, and therefore only ever
    // signals a child that is still running or not yet reaped, whose process id cannot have been reused
    std::lock_guard<std::mutex> status(impl_->status_mutex);
    impl_->exit_status = impl_->wait_native();
    return *impl_->exit_status;
}

int Process::terminate() noexcept {
    if (!impl_) {
        return 0;
    }
    try {
        {
            std::lock_guard<std::mutex> status(impl_->status_mutex);
            if (impl_->exit_status) {
                return 0;
            }
            // Signal without Boost.Process::terminate(), which reaps on some backends.
            // The single waiter below (or an existing asynchronous waiter) owns reaping.
            if (::kill(impl_->process_id, SIGKILL) != 0 && errno != ESRCH) {
                return 1;
            }
        }
        wait();
        return 0;
    }
    catch (...) {
        return 1;
    }
}

bool Process::is_running() const {
    if (!impl_) {
        return false;
    }
    std::lock_guard<std::mutex> status(impl_->status_mutex);
    return !impl_->exit_status;
}

std::string Process::read_stdout() const {
    return impl_->read_stdout();
}

std::string Process::read_stderr() const {
    return impl_->read_stderr();
}

} // namespace ecf::test::scaffold
