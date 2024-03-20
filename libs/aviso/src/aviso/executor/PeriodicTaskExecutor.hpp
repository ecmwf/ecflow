/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef aviso_executor_PeriodicTaskExecutor_HPP
#define aviso_executor_PeriodicTaskExecutor_HPP

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace aviso {

struct InvalidExecutorArgument : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

template <typename TASK>
class PeriodicTaskExecutor {
public:
    explicit PeriodicTaskExecutor(TASK task) : running_{false}, worker_{}, task_{std::move(task)} {}
    PeriodicTaskExecutor(const PeriodicTaskExecutor&)            = delete;
    PeriodicTaskExecutor(PeriodicTaskExecutor&&)                 = delete;
    PeriodicTaskExecutor& operator=(const PeriodicTaskExecutor&) = delete;
    PeriodicTaskExecutor& operator=(PeriodicTaskExecutor&&)      = delete;
    ~PeriodicTaskExecutor() { stop(); }

    void stop() {
        if (running_) {
            running_ = false;
            worker_.join();
        }
    }

    template <typename DURATION>
    void start(DURATION expiry) {
        if (expiry < liveness_) {
            throw InvalidExecutorArgument("PeriodicTaskExecutor: expiry must be greater than liveness");
        }

        start_   = std::chrono::system_clock::now();
        next_    = start_;
        running_ = true;

        worker_ = std::thread([this, expiry]() {
            while (running_) {
                auto now = std::chrono::system_clock::now();
                if (now < next_) {
                    std::this_thread::sleep_for(liveness_);
                }
                else {
                    this->task_(now);
                    if (!running_) {
                        break;
                    }
                    next_ = now + expiry;
                }
            }
        });
    }

private:
    std::chrono::system_clock::duration liveness_ = std::chrono::milliseconds(250);
    // the liveness is the amount of time to wait before checking if the executor should stop

    std::chrono::system_clock::time_point start_;
    std::chrono::system_clock::time_point next_;
    std::atomic<bool> running_;

    std::thread worker_;
    TASK task_;
};

} // namespace aviso

#endif /* aviso_PeriodicExecutor_HPP */
