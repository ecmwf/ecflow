/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef aviso_PeriodicExecutor_HPP
#define aviso_PeriodicExecutor_HPP

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace aviso {

class PeriodicExecutor {
public:
    explicit PeriodicExecutor() : running_{false}, worker_{} {}
    PeriodicExecutor(const PeriodicExecutor&)            = delete;
    PeriodicExecutor(PeriodicExecutor&&)                 = delete;
    PeriodicExecutor& operator=(const PeriodicExecutor&) = delete;
    PeriodicExecutor& operator=(PeriodicExecutor&&)      = delete;
    ~PeriodicExecutor() { stop(); }

    void stop() {
        if (running_) {
            running_ = false;
            worker_.join();
        }
    }

    template <typename CALLBACK>
    void start(CALLBACK& callback, std::chrono::seconds expiry) {
        running_ = true;
        worker_  = std::thread([expiry, &callback, this]() {
            while (running_) {
                callback();
                std::this_thread::sleep_for(expiry);
            }
        });
    }

private:
    std::atomic<bool> running_;
    std::thread worker_;
};

} // namespace aviso

#endif /* aviso_PeriodicExecutor_HPP */
