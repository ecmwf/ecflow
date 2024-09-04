/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_server_PeriodicScheduler_HPP
#define ecflow_server_PeriodicScheduler_HPP

#include <chrono>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "ecflow/core/Log.hpp"

namespace ecf {

class Timer {
public:
    explicit Timer(boost::asio::io_context& io) : io_{io}, timer_{io_, boost::posix_time::seconds(0)} {}
    Timer(const Timer&) = delete;

    Timer& operator=(const Timer&) = delete;

    void cancel() { timer_.cancel(); }

    template <typename CALLBACK>
    void set(CALLBACK callback, std::chrono::seconds expiry) {
        /// Appears that `expires_from_now` is more accurate `then expires_at`
        ///   i.e. timer_.expires_at( timer_.expires_at() + boost::posix_time::seconds( poll_at ) );
        timer_.expires_from_now(boost::posix_time::seconds(expiry.count()));
        timer_.async_wait(boost::asio::bind_executor(io_, callback));
    }

private:
    boost::asio::io_context& io_;
    boost::asio::deadline_timer timer_;
};

///
/// \brief PeriodicScheduler class sets up the periodic execution of a given task.
///
/// The implementation considers *SOFT* real-time by using an 1-second polling
/// mechanism, to coarsely detect the 1-minute boundary.
/// In practice, the task is scheduled loosely at 1-second intervals, and
/// provided with a flag indicating whenever the 1-minute boundary is crossed.
///
/// Important: Since the task might take a significant time to run, some
/// slots could be missed, without consequence.
///
/// The Task is expected to implement the following `operator()`:
///
/// ```
/// struct Task {
///   using instant_t  = ecf::PeriodicScheduler<Task>::instant_t;
///   void operator()(instant_t now, instant_t last, instant_t next, bool is_boundary) {
///     /* activity execution details */
///   }
/// };
/// ```
///

template <typename TASK>
class PeriodicScheduler {
public:
    using clock_t    = std::chrono::system_clock;
    using instant_t  = clock_t::time_point;
    using duration_t = clock_t::duration;

public:
    PeriodicScheduler(const PeriodicScheduler&) = delete;
    PeriodicScheduler(boost::asio::io_context& io, std::chrono::seconds interval, TASK& activity)
        : timer_(io),
          last_execution_(),
          next_execution_(),
          execution_interval_(interval),
          first_(true),
          running_(false),
          activity_(activity) {}

    ~PeriodicScheduler() = default;

    PeriodicScheduler& operator=(const PeriodicScheduler&) = delete;

    void start();
    void stop();
    void terminate();

    void poll(const boost::system::error_code& error);

    void execute();

private:
    void reset_timer();

private:
    Timer timer_;
    instant_t last_execution_;
    instant_t next_execution_;
    duration_t execution_interval_;

    bool first_;
    bool running_;

    TASK& activity_;
};

template <typename TASK>
void PeriodicScheduler<TASK>::reset_timer() {
    timer_.set([this](const boost::system::error_code& error) { poll(error); }, std::chrono::seconds(1));
}

template <typename TASK>
void PeriodicScheduler<TASK>::start() {
    if (running_) {
        // Nothing to do...
        return;
    }

    running_ = true;
    if (first_) {
        first_ = false;

        auto now = clock_t::now();

        last_execution_ = now;
        next_execution_ = last_execution_ + execution_interval_;

        activity_(now, last_execution_, next_execution_, true);

        reset_timer();
    }
}

template <typename TASK>
void PeriodicScheduler<TASK>::stop() {
    running_ = false;
}

template <typename TASK>
void PeriodicScheduler<TASK>::terminate() {
    timer_.cancel();
}

template <typename TASK>
void PeriodicScheduler<TASK>::poll(const boost::system::error_code& error) {
    if (error) {
        if (error == boost::asio::error::operation_aborted) {
            // This was intentional! Nothing to do...
            return;
        }

        LogToCout toCoutAsWell;
        std::string msg = "NodeTreeTraverser::traverse error: ";
        msg += error.message();
        ecf::log(Log::ERR, msg);
        return;
    }

    execute();
}

template <typename TASK>
void PeriodicScheduler<TASK>::execute() {
    auto now = clock_t::now();

    // (A) Handle the case where `now < next_execution_instant_`

    if (now < next_execution_) {
        reset_timer();

        // When `now` is before the next planned execution,
        // the activity is called with the indication `is_boundary = false`
        activity_(now, last_execution_, next_execution_, false);
        return;
    }

    // (B) Handle the case where `now >= next_execution_instant_`

    // (B.1) Log a warning in case the execution slot was missing due to _severe_ execution delay
    {
        auto since_last_execution = std::chrono::duration_cast<std::chrono::seconds>(now - last_execution_);
        if (since_last_execution > execution_interval_) {
            /// This is tolerable from time to time, so it is only reported for troublesome times
            auto delay = since_last_execution - execution_interval_;
            if (delay > (execution_interval_ * 0.25)) {
                LogFlusher logFlusher;
                LOG(Log::WAR,
                    ": interval is " << execution_interval_.count() << " seconds, but last request took "
                                     << since_last_execution.count() << " seconds.");
            }
        }
    }

    // (B.2) Update last/next execution times, reset the timer and run the activity

    // We poll *EVERY second but update the next_poll_time_ to be consistent with the job submission interval
    // Hence the next poll times *will* vary (SOFT REAL TIME).
    // Note: On server start, we modified the next_poll_time_ to hit the minute boundary.
    //
    //         FIRST          time_now(skip)                                   time_now(traverse node tree)
    //           |                |         L                                      |         S
    //           V                V------------------------|                       V-----------------|
    // ==========0====================0====================0====================0====================0
    //           ^                    ^                    ^                    ^                    ^
    //           |                    |                    |                    |                    |
    //         last_time_       next_poll_time_       next_poll_time_      next_poll_time_      next_poll_time_
    //         next_poll_time_
    //                                ^                                         ^
    //                                |                                         |
    //                             last_time_                                last_time_
    //                             reset to previous next_poll_time_, to avoid yo-yoing, messages about poll being
    //                             long/short
    //
    // Update next_poll_time_: WE  *ONLY* get here if time_now >= next_poll_time

    if (now > next_execution_) {
        /// Update next execution time, by adding execution interval, until it is in the future
        while (next_execution_ <= now) {
            next_execution_ += execution_interval_;
        }
    }
    else {
        /// We hit the poll time!
        /// Note: this also happens when calling for the *FIRST* time (i.e. now == next execution time)
        next_execution_ += execution_interval_;
    }

    // At begin time for very large suites, slow disk, and in test, during job generation we can miss the next poll
    // time(i.e a,b) This means that on the next poll time because last_time was not updated, to be immediately behind
    // the next poll time by 'interval_' seconds an erroneous report is logged about missing the poll time
    //
    //         FIRST                                                               Time_now
    //           |                                                                    |
    //           V                    a                    b                    c     X
    // ==========0====================0====================0====================0====================0
    //           ^                    ^                    ^                    ^                    ^
    //           |                    |                    |                    |                    |
    //         last_time_       next_poll_time_       next_poll_time_      next_poll_time_      next_poll_time_
    //
    // Hence we need to ensure that last_time is always less that next_poll_time_ by interval_
    // In the diagram above missed poll time a and b, then we need to set last_time_ to 'c' and *NOT* 'a' | 'b'
    last_execution_ = next_execution_ - execution_interval_;

    reset_timer();

    activity_(now, last_execution_, next_execution_, true);
}

} // namespace ecf

#endif /* ecflow_server_PeriodicActivity_HPP */
