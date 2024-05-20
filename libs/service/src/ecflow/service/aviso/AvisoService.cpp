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

#include "ecflow/service/Registry.hpp"
#include "ecflow/service/aviso/etcd/Client.hpp"

namespace ecf::service::aviso {

namespace {

std::string get_authentication_credential(const std::string& auth_file) {
    std::ifstream file(auth_file);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file: " + auth_file);
    }

    std::string token;
    std::getline(file, token);
    file.close();

    return token;
}

} // namespace

void AvisoService::start() {

    // Update list of listeners

    auto new_subscriptions = subscribe_();
    for (auto&& subscription : new_subscriptions) {
        if (subscription.is_start()) {
            register_listener(subscription);
        }
        else {
            unregister_listener(subscription.path());
        }
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

    ALOG(D, "AvisoService: start polling, with polling interval: " << expiry << " s");
    executor_.start(std::chrono::seconds{expiry});
}

void AvisoService::operator()(const std::chrono::system_clock::time_point& now) {

    // Update list of listeners

    auto new_subscriptions = subscribe_();
    for (auto&& subscription : new_subscriptions) {
        if (subscription.is_start()) {
            register_listener(subscription);
        }
        else {
            unregister_listener(subscription.path());
        }
    }

    // Check notification for each listener
    {
        for (auto& entry : listeners_) {
            // For the associated host(+port)
            aviso::etcd::Client client{entry.address(), entry.auth_token};

            ALOG(D,
                 "AvisoService: polling " << entry.address().address() << " for Aviso " << entry.path() << " (key: "
                                          << entry.prefix() << ", rev: " << entry.listener().revision() << ")");

            // Poll notifications on the key prefix
            auto updated_keys = client.poll(entry.prefix(), entry.listener().revision() + 1);

            // Pass updated keys to the listener
            for (auto&& [key, value] : updated_keys) {
                if (key == "latest_revision") {
                    auto revision = value;
                    ALOG(D, "AvisoService: updating revision for " << entry.path() << " to " << revision);
                    entry.listener().update_revision(std::stoll(value));
                    ALOG(D, "AvisoService: revision for " << entry.path() << " is now " << entry.listener().revision());
                    continue;
                }

                if (auto notification = entry.listener().accepts(key, value, entry.listener().revision());
                    notification) {
                    notification_t n{std::string{entry.path()}, entry.listener(), *notification};
                    notify_(n);
                }
            }
        }
    }
}

void AvisoService::register_listener(const AvisoRequest& listen) {
    auto listener   = ConfiguredListener::make_configured_listener(listen);
    auto address    = listener.address();
    auto key_prefix = listener.prefix();

    ALOG(D,
         "AvisoService: creating listener {" << listener.path() << ", " << address.address() << ", " << key_prefix
                                             << "}");

    auto& inserted = listeners_.emplace_back(listener);

    if (auto auth = listen.auth(); !auth.empty()) {
        inserted.auth_token = get_authentication_credential(listen.auth());
    }
}

void AvisoService::unregister_listener(const std::string& unlisten_path) {

    ALOG(D, "AvisoService: removing listener: {" << unlisten_path << "}");

    listeners_.erase(std::remove_if(std::begin(listeners_),
                                    std::end(listeners_),
                                    [&unlisten_path](auto&& listener) { return listener.path() == unlisten_path; }),
                     std::end(listeners_));
}

AvisoController::AvisoController()
    : base_t{[this](const AvisoController::notification_t& notification) {
                 this->notify(notification);
                 // The following is a hack to force the server to increment the job generation count
                 TheOneServer::server().increment_job_generation_count();
             },
             [this]() { return this->get_subscriptions(); }} {
}

} // namespace ecf::service::aviso
