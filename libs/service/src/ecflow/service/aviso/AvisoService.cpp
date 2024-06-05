/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/service/aviso/AvisoService.hpp"

#include "ecflow/core/Overload.hpp"
#include "ecflow/service/Registry.hpp"
#include "ecflow/service/auth/Credentials.hpp"
#include "ecflow/service/aviso/etcd/Client.hpp"

namespace ecf::service::aviso {

std::ostream& operator<<(std::ostream& os, const AvisoResponse& r) {
    std::visit(ecf::overload{[&os](const NotificationPackage<ConfiguredListener, AvisoNotification>& p) { os << p; },
                             [&os](const AvisoNoMatch& a) { os << a; },
                             [&os](const AvisoError& a) { os << a; }},
               r);
    return os;
}

void AvisoService::start() {

    // Update list of listeners

    auto new_subscriptions = subscribe_();
    for (auto&& subscription : new_subscriptions) {
        std::visit(ecf::overload{[this](const AvisoSubscribe& subscription) { this->register_listener(subscription); },
                                 [this](const AvisoUnsubscribe& subscription) {
                                     this->unregister_listener(subscription.path());
                                 }},
                   subscription);
    }

    // Start polling...

    std::uint32_t expiry = 40;

    if (!listeners_.empty()) {
        std::vector<std::uint32_t> polling_intervals;
        auto found = std::max_element(std::begin(listeners_), std::end(listeners_), [](const auto& a, const auto& b) {
            return a.listener().polling() < b.listener().polling();
        });
        expiry     = found->listener().polling();
    }

    SLOG(D, "AvisoService: start polling, with polling interval: " << expiry << " s");
    executor_.start(std::chrono::seconds{expiry});
}

void AvisoService::operator()(const std::chrono::system_clock::time_point& now) {

    // Update list of listeners

    auto new_subscriptions = subscribe_();
    for (auto&& subscription : new_subscriptions) {
        std::visit(ecf::overload{[this](const AvisoSubscribe& subscription) { this->register_listener(subscription); },
                                 [this](const AvisoUnsubscribe& subscription) {
                                     this->unregister_listener(subscription.path());
                                 }},
                   subscription);
    }

    // Check notification for each listener
    {
        for (auto& entry : listeners_) {
            SLOG(D,
                 "AvisoService: polling " << entry.listener().address() << " for Aviso " << entry.listener().path()
                                          << " (key: " << entry.listener().prefix()
                                          << ", rev: " << entry.listener().revision() << ")");

            // Poll notifications on the key prefix

            std::vector<std::pair<std::string, std::string>> updated_keys;
            try {
                // For the associated host(+port)
                aviso::etcd::Client client{entry.listener().address(), entry.auth_token};
                updated_keys = client.poll(entry.listener().prefix(), entry.listener().revision() + 1);
            }
            catch (const std::exception& e) {
                notify_(AvisoError(e.what())); // Notification regarding failure to contact the server
                return;
            }

            auto matched = false;
            // Pass updated keys to the listener
            for (auto&& [key, value] : updated_keys) {
                if (key == "latest_revision") {
                    auto revision = value;
                    SLOG(D, "AvisoService: updating revision for " << entry.listener().path() << " to " << revision);
                    entry.listener().update_revision(std::stoll(value));
                    SLOG(D,
                         "AvisoService: revision for " << entry.listener().path() << " is now "
                                                       << entry.listener().revision());
                    continue;
                }

                if (auto notification = entry.listener().accepts(key, value, entry.listener().revision());
                    notification) {
                    NotificationPackage<ConfiguredListener, AvisoNotification> n{
                        std::string{entry.listener().path()}, entry.listener(), *notification};
                    notify_(n); // Notification regarding a successful match
                    matched = true;
                }
            }

            if (!matched) {
                notify_(AvisoNoMatch{}); // Notification regarding no match
            }
        }
    }
}

void AvisoService::register_listener(const AvisoSubscribe& listen) {
    auto listener   = ConfiguredListener::make_configured_listener(listen);
    auto address    = listener.address();
    auto key_prefix = listener.prefix();

    SLOG(D, "AvisoService: creating listener {" << listener.path() << ", " << address << ", " << key_prefix << "}");

    auto& inserted = listeners_.emplace_back(listener);

    if (auto auth = listen.auth(); !auth.empty()) {
        auto credentials = ecf::service::auth::Credentials::load(auth);
        if (auto key_credentials = credentials.key(); key_credentials) {
            inserted.auth_token = key_credentials->key;
        }
        else {
            SLOG(I, "AvisoService: no key found in auth token for listener {" << listener.path() << "}");
        }
    }
}

void AvisoService::unregister_listener(const std::string& unlisten_path) {

    SLOG(D, "AvisoService: removing listener: {" << unlisten_path << "}");

    listeners_.erase(
        std::remove_if(std::begin(listeners_),
                       std::end(listeners_),
                       [&unlisten_path](auto&& entry) { return entry.listener().path() == unlisten_path; }),
        std::end(listeners_));
}

AvisoController::AvisoController()
    : base_t{[this](const AvisoController::notification_t& notification) {
                 this->notify(notification);

                 if (auto* server = TheOneServer::server(); server) {
                     // The following forces the server to increment the job generation count and traverse the defs
                     server->increment_job_generation_count();
                 }
                 else {
                     SLOG(E, "AvisoController: no server available, thus unable to increment job generation count");
                 }
             },
             [this]() { return this->get_subscriptions(); }} {
}

} // namespace ecf::service::aviso
