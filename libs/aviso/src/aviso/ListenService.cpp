/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "aviso/ListenService.hpp"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "aviso/ConfiguredListener.hpp"
#include "aviso/ListenerSchema.hpp"
#include "aviso/Log.hpp"
#include "aviso/etcd/Client.hpp"

namespace aviso {

void ListenService::start() {

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

    ALOG(D, "Start polling, with polling interval: " << expiry << " s");
    executor_.start(std::chrono::seconds{expiry});
}

void ListenService::operator()(const std::chrono::system_clock::time_point& now) {

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
            etcd::Client client{entry.address()};

            ALOG(D,
                 "Polling: " << entry.address().address() << " for Aviso " << entry.path()
                             << " (key: " << entry.prefix() << ", rev: " << entry.listener().revision() << ")");

            // Poll notifications on the key prefix
            auto updated_keys = client.poll(entry.prefix(), entry.listener().revision() + 1);

            // Pass updated keys to the listener
            for (auto&& [key, value] : updated_keys) {
                if (key == "latest_revision") {
                    auto revision = value;
                    ALOG(D, "Updating revision for " << entry.path() << " to " << revision);
                    entry.listener().update_revision(std::stoll(value));
                    ALOG(D, "Revision for " << entry.path() << " is now " << entry.listener().revision());
                    continue;
                }

                if (auto notification = entry.listener().accepts(key, value, entry.listener().revision());
                    notification) {
                    notify_(entry.listener(), *notification);
                }
            }
        }
    }
}

void ListenService::register_listener(const ListenRequest& listen) {
    auto listener = create_configured_listener(listen);
    register_listener(listener);
}

void ListenService::register_listener(const listener_t& listener) {
    auto address    = listener.address();
    auto key_prefix = listener.prefix();

    ALOG(D, "Creating listener: {" << listener.path() << ", " << address.address() << ", " << key_prefix << "}");

    listeners_.emplace_back(listener);
}

void ListenService::unregister_listener(const std::string& unlisten_path) {

    ALOG(D, "Removing listener: {" << unlisten_path << "}");

    listeners_.erase(std::remove_if(std::begin(listeners_),
                                    std::end(listeners_),
                                    [&unlisten_path](auto&& listener) { return listener.path() == unlisten_path; }),
                     std::end(listeners_));
}

} // namespace aviso
