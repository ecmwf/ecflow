// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/http/DefsStorage.hpp"

#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "ecflow/base/ConnectionDiagnosis.hpp"
#include "ecflow/base/PermissionDenied.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/http/HttpServerException.hpp"
#include "ecflow/http/Options.hpp"
#include "ecflow/node/Defs.hpp"

namespace ecf::http {

extern std::atomic<unsigned int> last_request_time;

static std::shared_ptr<Defs> defs_ = nullptr;

static std::mutex def_mutex;
static std::mutex cv_mutex;
static std::condition_variable defs_cv;
static std::atomic update_defs(true);

static std::unique_ptr<std::thread> loop_thread;
static std::atomic stop_loop(false);

// Whether the ecFlow server uses Permissions attributes, as learnt from the last refresh (D6.6)
static std::atomic uses_permissions(false);

namespace /* __anonymous__ */ {

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

} // namespace

std::shared_ptr<Defs> get_defs() {
    // The copy and the mode are changed together, under the same lock: a request admitted just before a refresh
    // reveals that the server uses Permissions never receives the tree of that refresh
    std::scoped_lock lock(def_mutex);
    if (uses_permissions.load()) {
        throw HttpServerException(HttpStatusCode::client_error_forbidden, permissions_refusal());
    }
    if (defs_ == nullptr) {
        throw HttpServerException(HttpStatusCode::server_error_service_unavailable,
                                  "The copy of the tree of the ecFlow server is not available");
    }
    return defs_;
}

bool server_uses_permissions() noexcept {
    return uses_permissions.load();
}

const std::string& permissions_refusal() {
    static const std::string message =
        "The REST API does not support Permissions: the ecFlow server uses Permissions attributes, and every request "
        "is refused";
    return message;
}

namespace /* __anonymous__ */ {

void defs_update(int interval) {
    ClientInvoker client;

    auto get_current_time = [] {
        struct timeval curtime;
        gettimeofday(&curtime, nullptr);
        return static_cast<unsigned int>(curtime.tv_sec);
    };

    // A request denied by the authorisation of Permissions (role r missing at /): the server uses Permissions and does
    // not list the account of the service. A denial by the white list names an access, not a role, and is reported as
    // any other failure.
    auto denied_by_permissions = [](const std::exception& e) {
        return dynamic_cast<const ecf::PermissionDenied*>(&e) != nullptr &&
               std::string(e.what()).find("(role ") != std::string::npos;
    };

    // Asks the server whether it uses Permissions, whatever the change numbers: a server restarted with Permissions can
    // answer the news and an incremental synchronisation of an earlier copy without revealing its server attribute.
    // Reading the server attribute succeeds, or is denied, only on a server using Permissions; any other answer of the
    // server (no attribute, or a server that does not know the command) means that it does not use them. A failure
    // to reach the server is thrown, and changes nothing.
    auto server_uses_permissions_now = [&] {
        try {
            client.permissions("read", "/", "", false);
            return true;
        }
        catch (const std::exception& e) {
            if (denied_by_permissions(e)) {
                return true;
            }
            const auto failure = client.connection_diagnosis().failure;
            if (failure == ecf::ConnectionFailure::None || failure == ecf::ConnectionFailure::RejectedRequest) {
                return false;
            }
            throw;
        }
    };

    // Drops the copy: every request is refused, and each refresh is a full one, so that a server restarted without
    // Permissions is recognised whatever its change numbers
    auto refuse = [&] {
        uses_permissions = true;
        defs_            = nullptr;
        client.reset();
    };

    // Refreshes the copy of the tree. A server using Permissions (see above), a refresh denied by Permissions, or a
    // tree carrying a server Permissions attribute, drops the copy: every request is refused until a refresh succeeds
    // against a server that does not use them
    auto update = [&] {
        const bool permissions = server_uses_permissions_now();
        {
            std::scoped_lock lock(def_mutex);
            if (permissions) {
                refuse();
                return;
            }
            try {
                if (defs_ != nullptr) {
                    client.sync(defs_);
                }
                else {
                    client.sync_local();
                }
            }
            catch (const std::exception& e) {
                if (!denied_by_permissions(e)) {
                    throw;
                }
                refuse();
                if (opts.verbose) {
                    printf("Refresh denied: the ecFlow server uses Permissions, every request is refused\n");
                }
                return;
            }
            defs_ = client.defs();
            if (defs_ != nullptr && defs_->server_state().permissions() != nullptr) {
                refuse(); // the tree of a server using Permissions is never kept
                return;
            }
            uses_permissions = false;
        }
        if (opts.verbose && defs_ != nullptr) {
            printf(
                "Defs modify_change_no: %d state_change_no: %d\n", defs_->modify_change_no(), defs_->state_change_no());
        }
    };

    // Asks the server for news; without a copy of the tree, after a denied refresh, a refresh is attempted instead.
    // A server that uses Permissions has news: the refresh that follows records it
    auto has_news = [&] {
        {
            std::scoped_lock lock(def_mutex);
            if (defs_ == nullptr) {
                return true;
            }
        }
        if (server_uses_permissions_now()) {
            return true;
        }
        try {
            client.news(defs_);
        }
        catch (const std::exception& e) {
            if (!denied_by_permissions(e)) {
                throw;
            }
            return true; // the refresh that follows is denied as well, and records it
        }
        return client.get_news();
    };

    // These will throw is ecflow server is not present; we will not
    // try to reconnect if there was not a connection to begin with.
    // A server that denies the refresh by Permissions is present: the service starts, and refuses every request.
    try {
        client.news_local();
    }
    catch (const std::exception& e) {
        if (!denied_by_permissions(e)) {
            throw;
        }
    }
    update();

    // Implement a drift to update cycle. The basic idea is that if we
    // do not get requests to the interface, we slowly start to increase
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
                std::unique_lock lock(cv_mutex);
                if (defs_cv.wait_for(lock, sleeptime, [] { return update_defs.load(); })) {
                    if (stop_loop.load() == true) {
                        // we received the 'signal' to stop, so we terminate the loop
                        return;
                    }

                    // update requested by some other thread
                    if (opts.verbose) {
                        printf("defs update requested\n");
                    }
                    update();
                    update_defs = false;
                }
                else {
                    if (stop_loop.load() == true) {
                        // we received the 'signal' to stop, so we terminate the loop
                        return;
                    }

                    // update triggered by timeout
                    if (has_news()) {
                        update();
                        update_defs = false;
                    }
                }

                if (opts.max_polling_interval <= opts.polling_interval) {
                    // drift disabled
                    continue;
                }
                const auto last_request_age = static_cast<double>(get_current_time() - last_request_time.load());
                const auto drift            = std::chrono::seconds(static_cast<int>(floor(last_request_age / 60.)));

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
}

} // namespace

void start_update_defs_loop(int interval) {
    loop_thread = std::make_unique<std::thread>(defs_update, interval);

    while (update_defs.load() == true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void stop_update_defs_loop() {
    {
        std::unique_lock lock(cv_mutex);
        // set the 'signal' to stop the loop
        stop_loop = true;
        // and trigger the update loop immediately
        update_defs = true;
        defs_cv.notify_one();
    }

    loop_thread->join();
}

template <typename F>
void trigger_defs_update_predicate(F f) {
    {
        std::unique_lock lock(cv_mutex);
        update_defs = true;
        defs_cv.notify_one();
    }

    while (update_defs.load() == true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    f();
}

void trigger_defs_update() {
    trigger_defs_update_predicate([] { /* No OP */ });
}

void trigger_defs_update(std::function<void()> function) {
    trigger_defs_update_predicate(function);
}

} // namespace ecf::http
