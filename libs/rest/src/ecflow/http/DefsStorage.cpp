// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/http/DefsStorage.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

#include "ecflow/client/ShadowDefs.hpp"
#include "ecflow/http/Options.hpp"
#include "ecflow/node/Defs.hpp"

namespace ecf::http {

extern std::atomic<unsigned int> last_request_time;

namespace /* __anonymous__ */ {

std::unique_ptr<ShadowDefs> shadow;

void print_polling_interval_notification(long long sleeptime,
                                         long long base_sleeptime,
                                         long long drift,
                                         long long max_sleeptime) {
    printf("Polling interval is now %lld (base: %lld drift: %lld max: %lld)\n",
           sleeptime,
           base_sleeptime,
           drift,
           max_sleeptime);
}

std::chrono::seconds idle_drift(double last_request_age) {
    return std::chrono::seconds(static_cast<int>(std::floor(last_request_age / 60.)));
}

unsigned int current_time() {
    struct timeval curtime;
    gettimeofday(&curtime, nullptr);
    return static_cast<unsigned int>(curtime.tv_sec);
}

} // namespace

std::chrono::seconds polling_interval(int base, int max_option, double last_request_age) {
    // Implement a drift to update cycle. The basic idea is that if we
    // do not get requests to the interface, we slowly start to increase
    // the interval which we use to poll ecFlow server. This is done to
    // reduce ecFlow server load when there is no activity on the REST
    // API.
    // For every minute that goes by without any activity on the REST API,
    // we increase the drift by one second, up to the maximum interval, which
    // is never below 30 seconds. Activity on the API resets the drift to zero.
    const std::chrono::seconds base_sleeptime(base);
    if (max_option <= base) {
        // drift disabled
        return base_sleeptime;
    }
    const std::chrono::seconds max_sleeptime(std::max(30, max_option));
    return std::min(max_sleeptime, base_sleeptime + idle_drift(last_request_age));
}

std::shared_ptr<Defs> get_defs() {
    return shadow ? shadow->get() : nullptr;
}

void start_update_defs_loop(int interval) {
    auto previous = std::make_shared<std::chrono::seconds>(interval);

    auto next_interval = [interval, previous]() -> std::chrono::milliseconds {
        const auto last_request_age = static_cast<double>(current_time() - last_request_time.load());
        const auto sleeptime        = polling_interval(interval, opts.max_polling_interval, last_request_age);
        if (opts.verbose && sleeptime != *previous) {
            print_polling_interval_notification(sleeptime.count(),
                                                interval,
                                                idle_drift(last_request_age).count(),
                                                std::max(30, opts.max_polling_interval));
        }
        *previous = sleeptime;
        return sleeptime;
    };

    auto report = [](const std::string& /* reason */) {
        printf("ERROR: Communication problem with ecflow server? Retrying in 5s\n");
    };

    auto updated = [](const Defs& defs) {
        if (opts.verbose) {
            printf("Defs modify_change_no: %d state_change_no: %d\n", defs.modify_change_no(), defs.state_change_no());
        }
    };

    shadow = std::make_unique<ShadowDefs>(
        std::make_unique<ClientInvokerShadowDefsSource>(), next_interval, report, std::chrono::seconds(5), updated);
    if (opts.verbose) {
        printf("defs update requested\n");
    }

    // This throws if the ecflow server is not present; there is no attempt to reconnect if there was not a
    // connection to begin with
    shadow->start();
    updated(*shadow->get());
}

void stop_update_defs_loop() {
    if (shadow) {
        shadow->stop();
    }
}

void trigger_defs_update() {
    trigger_defs_update([] { /* No OP */ });
}

void trigger_defs_update(std::function<void()> function) {
    if (opts.verbose) {
        printf("defs update requested\n");
    }
    shadow->refresh(function);
}

} // namespace ecf::http
