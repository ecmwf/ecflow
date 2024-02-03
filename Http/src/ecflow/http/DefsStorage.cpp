/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/http/Options.hpp"
#include "ecflow/node/Defs.hpp"

namespace ecf::http {

extern std::atomic<unsigned int> last_request_time;

std::shared_ptr<Defs> defs_ = nullptr;

static std::mutex def_mutex, cv_mutex;
static std::condition_variable defs_cv;
std::atomic<bool> update_defs(true);

namespace /* __anonymous__ */ {

void print_polling_interval_notification(long long sleeptime,
                                         long long base_sleeptime,
                                         long long drift,
                                         long long max_sleeptime) {
    const char* fmt = "Polling interval is now %lld (base: %lld drift: %lld max: %lld)\n";
    printf(fmt, sleeptime, base_sleeptime, drift, max_sleeptime);
}

} // namespace

std::shared_ptr<Defs> get_defs() {
    std::lock_guard<std::mutex> lock(def_mutex);
    return defs_;
}

void update_defs_loop(int interval) {
    std::thread t([&]() {
        ClientInvoker client;

        auto get_current_time = [] {
            struct timeval curtime;
            gettimeofday(&curtime, nullptr);
            return static_cast<unsigned int>(curtime.tv_sec);
        };

        auto update = [&] {
            {
                std::lock_guard<std::mutex> lock(def_mutex);
                if (defs_ != nullptr)
                    client.sync(defs_);
                defs_ = client.defs();
            }
            if (opts.verbose) {
                printf("Defs modify_change_no: %d state_change_no: %d\n",
                       defs_->modify_change_no(),
                       defs_->state_change_no());
            }
        };

        // These will throw is ecflow server is not present; we will not
        // try to reconnect if there wasn't a connection to begin with
        client.news_local();
        client.sync_local();

        // Implement a drift to update cycle. The basic idea is that if we
        // don't get requests to the interface, we slowly start to increase
        // the interval which we use to poll ecFlow server. This is done to
        // reduce ecFlow server load when there is no activity on the REST
        // API.
        // For every minute that goes by without any activity on the REST API,
        // we increase the drift by one second. The maximum drift value is
        // 10 * polling interval, but minimum 30 seconds. Activity on the API
        // will reset drift to zero.

        const std::chrono::seconds base_sleeptime(interval);
        std::chrono::seconds sleeptime(interval);
        const std::chrono::seconds max_sleeptime(std::max(30, opts.max_polling_interval));
        std::chrono::seconds previous_sleeptime(interval);

        for (;;) {
            try {
                while (true) {
                    std::unique_lock<std::mutex> lock(cv_mutex);
                    if (defs_cv.wait_for(lock, sleeptime, [] { return update_defs.load(); })) {
                        // update requested by some other thread
                        if (opts.verbose)
                            printf("defs update requested\n");
                        update();
                        update_defs = false;
                    }
                    else {
                        // update triggered by timeout
                        client.news(defs_);
                        if (client.get_news()) {
                            update();
                            update_defs = false;
                        }
                    }

                    if (opts.max_polling_interval <= opts.polling_interval) {
                        // drift disabled
                        continue;
                    }
                    const double last_request_age = static_cast<double>(get_current_time() - last_request_time.load());
                    const auto drift = std::chrono::seconds(static_cast<int>(floor(last_request_age / 60.)));

                    sleeptime = min(max_sleeptime, base_sleeptime + drift);
                    if (opts.verbose && sleeptime != previous_sleeptime) {
                        print_polling_interval_notification(
                            sleeptime.count(), base_sleeptime.count(), drift.count(), max_sleeptime.count());
                    }

                    previous_sleeptime = sleeptime;
                }
            }
            catch (const std::exception& e) {
                printf("ERROR: Communication problem with ecflow server? Retrying in 5s\n");
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    });

    t.detach();

    while (update_defs.load() == true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

template <typename T>
void trigger_defs_update_predicate(T&& func) {
    {
        std::lock_guard<std::mutex> lock(cv_mutex);
        update_defs = true;
        defs_cv.notify_one();
    }

    while (update_defs.load() == true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    func();
}

void trigger_defs_update() {
    trigger_defs_update_predicate([] {});
}

void trigger_defs_update(std::function<void()> function) {
    trigger_defs_update_predicate(function);
}

} // namespace ecf::http
