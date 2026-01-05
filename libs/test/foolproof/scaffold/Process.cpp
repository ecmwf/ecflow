/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "Process.hpp"

#include <iostream>
#include <sstream>

#include <boost/version.hpp>

#include "Naming.hpp"
#include "ecflow/core/Environment.hpp"

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

namespace foolproof::scaffolf {

#if BOOST_PROCESS_VERSION == 1

struct Process::Impl
{
    Impl(std::string_view executable, const std::vector<std::string>& args, const fs::path& cwd)
        : out_{},
          err_{},
          handle_{} {
        // Determine the invocation command
        auto invoke_command = std::string(executable);
        for (const auto& arg : args) {
            invoke_command += " " + arg;
        }

        // Start the process
        handle_ = bp::child{invoke_command, bp::std_out > out_, bp::std_err > err_, bp::start_dir(cwd.string())};
    }
    ~Impl() = default;

    int pid() const { return handle_.id(); }

    int wait() {
        handle_.wait();
        return handle_.exit_code();
    }

    int terminate() {
        handle_.terminate();
        return 0;
    }

    std::string read_stdout() {
        std::ostringstream buffer;
        while (out_.good() && out_.peek() != EOF) {
            char c = out_.get();
            buffer << c;
        }
        return buffer.str();
    }

    std::string read_stderr() {
        std::ostringstream buffer;
        while (err_.good() && err_.peek() != EOF) {
            char c = err_.get();
            buffer << c;
        }
        return buffer.str();
    }

    bp::ipstream out_;
    bp::ipstream err_;

    bp::child handle_;
};

#elif BOOST_PROCESS_VERSION == 2

struct Process::Impl
{
    Impl(std::string_view executable, const std::vector<std::string>& args, const fs::path& cwd)
        : ctx_{},
          stdout_buffer_{},
          stderr_buffer_{},
          stdout_pipe_{ctx_},
          stderr_pipe_{ctx_},
          handle_{ctx_,
                  executable,
                  args,
                  bp::process_start_dir{cwd.c_str()},
                  bp::process_stdio{{/* stdin default */}, {stdout_pipe_}, {stderr_pipe_}}} {
        // The process is started in the constructor
    }
    ~Impl() = default;

    int pid() const { return handle_.id(); }

    int wait() { return handle_.wait(); }

    int terminate() {
        bp::error_code ec;
        handle_.terminate(ec);
        if (ec) {
            ECF_TEST_DBG("Failed to terminate process: " << ec.message());
            return 1;
        }
        return 0;
    }

    std::string read_stdout() {
        std::string stdout_buffer;
        boost::system::error_code ec;
        ba::read(stdout_pipe_, ba::dynamic_buffer(stdout_buffer), ec);
        assert(ec == boost::asio::error::eof);
        return stdout_buffer;
    }

    std::string read_stderr() {
        std::string stderr_buffer;
        boost::system::error_code ec;
        ba::read(stderr_pipe_, ba::dynamic_buffer(stderr_buffer), ec);
        assert(ec == boost::asio::error::eof);
        return stderr_buffer;
    }

    ba::io_context ctx_;
    std::string stdout_buffer_;
    std::string stderr_buffer_;
    ba::readable_pipe stdout_pipe_;
    ba::readable_pipe stderr_pipe_;

    bp::process handle_;
};

#else
    #error "Unsupported boost::process version"
#endif

Process::Process() : impl_{nullptr} {
}

Process::Process(Process&& rhs) : impl_{std::move(rhs.impl_)} {
    rhs.impl_ = nullptr;
}

Process::Process(const fs::path& executable, std::vector<std::string> args, fs::path cwd)
    : impl_{std::make_unique<Impl>(executable.c_str(), args, cwd)} {
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
    return impl_->pid();
}

int Process::wait() {
    return impl_->wait();
}

int Process::terminate() {
    return impl_->terminate();
}

bool Process::is_running() const {
    return impl_ != nullptr;
}

std::string Process::read_stdout() const {
    return impl_->read_stdout();
}

std::string Process::read_stderr() const {
    return impl_->read_stderr();
}

} // namespace foolproof::scaffolf
