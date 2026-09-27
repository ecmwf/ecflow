// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <string>
#include <unistd.h>

#include <boost/test/unit_test.hpp>
#include <sys/resource.h>
#include <sys/wait.h>

#if defined(__linux__)
    #include <sys/syscall.h>
#endif

#include "ecflow/node/Signal.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

namespace {

///
/// @brief Tells whether the running kernel offers the close_range system call.
///
/// A call with an empty range (the first descriptor above the last) fails with EINVAL where the call exists, and
/// with ENOSYS where it does not; it closes nothing either way.
///
bool close_range_available() {
#if defined(__linux__) && defined(__NR_close_range)
    errno = 0;
    return ::syscall(__NR_close_range, 1U, 0U, 0U) == -1 && errno == EINVAL;
#else
    return false;
#endif
}

///
/// @brief Raises the soft limit on open files of the test process, within the hard limit, and restores it when
/// destroyed.
///
struct RaisedLimit
{
    explicit RaisedLimit(rlim_t wanted) {
        if (::getrlimit(RLIMIT_NOFILE, &saved_) != 0) {
            return;
        }
        rlimit raised   = saved_;
        raised.rlim_cur = std::max(saved_.rlim_cur, std::min(wanted, saved_.rlim_max));
        restore_        = raised.rlim_cur != saved_.rlim_cur && ::setrlimit(RLIMIT_NOFILE, &raised) == 0;
    }

    RaisedLimit(const RaisedLimit&)            = delete;
    RaisedLimit& operator=(const RaisedLimit&) = delete;

    ~RaisedLimit() {
        if (restore_) {
            ::setrlimit(RLIMIT_NOFILE, &saved_);
        }
    }

private:
    rlimit saved_{};
    bool restore_{false};
};

using milliseconds = std::chrono::duration<double, std::milli>;

///
/// @brief Returns the time taken to fork a child that closes every descriptor from 3 up, and to wait for it.
///
milliseconds time_closing_in_child(bool allow_close_range) {
    // With System in place, SIGCHLD stays blocked, so that its handler does not reap the child waited for here
    System::instance();

    const auto start = std::chrono::steady_clock::now();
    pid_t pid        = ::fork();
    if (pid == 0) {
        close_descriptors_from(3, allow_close_range);
        ::_exit(0);
    }
    BOOST_REQUIRE_MESSAGE(pid > 0, "fork() failed");
    int status = 0;
    BOOST_REQUIRE(::waitpid(pid, &status, 0) == pid);
    return std::chrono::steady_clock::now() - start;
}

///
/// @brief Returns the time taken to spawn a command as the server spawns a job, and to wait for it to terminate.
///
milliseconds time_spawn() {
    const auto start = std::chrono::steady_clock::now();
    std::string errorMsg;
    BOOST_REQUIRE_MESSAGE(System::instance()->spawn(System::ECF_STATUS_CMD, "true", "", errorMsg),
                          "System::instance()->spawn() failed: " << errorMsg);
    while (System::instance()->process() != 0) {
        Signal unblock_on_destruction_then_reblock;
        System::instance()->processTerminatedChildren();
    }
    return std::chrono::steady_clock::now() - start;
}

template <typename Measure>
milliseconds best_of(int runs, Measure measure) {
    milliseconds best = measure();
    for (int run = 1; run < runs; ++run) {
        best = std::min(best, measure());
    }
    return best;
}

} // namespace

BOOST_AUTO_TEST_SUITE(P_Node)

BOOST_AUTO_TEST_SUITE(T_SystemDescriptors)

///
/// With a large limit on open files, closing the descriptors inherited by a job, and so spawning it, takes a small
/// fraction of the time of closing every descriptor number in turn, wherever the kernel offers close_range.
///
BOOST_AUTO_TEST_CASE(test_closing_time_does_not_grow_with_the_limit) {
    ECF_NAME_THIS_TEST();

    if (!close_range_available()) {
        BOOST_TEST_MESSAGE("Skipped: the kernel does not offer close_range, and every descriptor number is closed in "
                           "turn, as before");
        return;
    }

    // The limit is raised far enough for the loop to take a measurable time, but bounded, so that the loop does
    // not take minutes where the hard limit is very large (1073741816 in the containers of kind, for example)
    constexpr rlim_t wanted = 1 << 20;
    constexpr long enough   = 1 << 18;
    RaisedLimit raised_limit(wanted);
    const long limit = ::sysconf(_SC_OPEN_MAX);
    if (limit < enough) {
        BOOST_TEST_MESSAGE("Skipped: the limit on open files is " << limit << ", below " << enough
                                                                  << ", too small for a meaningful comparison");
        return;
    }

    const milliseconds loop  = best_of(3, [] { return time_closing_in_child(false); });
    const milliseconds fast  = best_of(5, [] { return time_closing_in_child(true); });
    const milliseconds spawn = best_of(5, [] { return time_spawn(); });

    BOOST_TEST_MESSAGE("With a limit of " << limit << " open files: the loop took " << loop.count()
                                          << " ms, close_range " << fast.count() << " ms, a spawned command "
                                          << spawn.count() << " ms");

    BOOST_CHECK_MESSAGE(fast * 5 < loop,
                        "close_range (" << fast.count() << " ms) is not markedly faster than the loop (" << loop.count()
                                        << " ms)");
    BOOST_CHECK_MESSAGE(spawn * 5 < loop,
                        "A spawned command (" << spawn.count() << " ms) is not markedly faster than the loop ("
                                              << loop.count() << " ms)");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
