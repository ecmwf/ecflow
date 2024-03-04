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

#include "aviso/ConfiguredListener.hpp"
#include "aviso/ListenerSchema.hpp"
#include "aviso/PeriodicExecutor.hpp"
#include "aviso/etcd/Client.hpp"

namespace aviso {

void ListenService::operator()() {

    // Update list of listeners

    auto new_subscriptions = subscribe_();
    for (auto&& subscription : new_subscriptions) {
        register_listener(subscription);
    }

    // Check notification for each listener
    {
        int64_t highest_revision = 0;
        // For each host(+port)
        for (auto&& [address, prefix_listerners] : listeners_) {
            etcd::Client client{address};
            // For each key prefix
            for (auto&& [key_prefix, registered_listeners] : prefix_listerners) {
                // Poll notifications on the key prefix
                auto updated_keys = client.poll(key_prefix, latest_revision_ + 1);
                // Pass updated keys to the listeners
                for (auto&& [key, value] : updated_keys) {
                    if (key == "latest_revision") {
                        highest_revision = std::max(static_cast<int64_t>(std::stoll(value)), highest_revision);
                        continue;
                    }
                    // For each listener
                    for (auto&& listener : registered_listeners) {
                        // Accept the notification, if the listener is interested in the key/value
                        if (auto notification = listener.accepts(key, value); notification) {
                            notify_(listener, *notification);
                        }
                    }
                }
            }
        }

        // Ensure lastest revisision is up-to-date
        latest_revision_ = std::max(latest_revision_, highest_revision);
    }
}

void ListenService::register_listener(const ListenRequest& request) {
    auto listener = create_configured_listener(request, schema_);
    register_listener(listener);
}

void ListenService::register_listener(const listener_t& listener) {
    auto address    = listener.address();
    auto key_prefix = listener.prefix();

    listeners_[address][key_prefix].push_back(listener);
}

} // namespace aviso
