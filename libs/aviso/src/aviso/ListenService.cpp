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

#include <iostream>

#include "aviso/ConfiguredListener.hpp"
#include "aviso/ListenerSchema.hpp"
#include "aviso/PeriodicExecutor.hpp"
#include "aviso/etcd/Client.hpp"

namespace aviso {

void ListenService::operator()() {

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

void ListenService::register_listener(const ListenRequest& listen) {
    auto listener = create_configured_listener(listen, schema_);
    register_listener(listener);
}

void ListenService::register_listener(const listener_t& listener) {
    auto address    = listener.address();
    auto key_prefix = listener.prefix();

    std::cout << "Register listener path " << listener.path() << " for " << address.address() << " and " << key_prefix
              << std::endl;
    listeners_[address][key_prefix].push_back(listener);
}

void ListenService::unregister_listener(const std::string& unlisten_path) {

    std::cout << "Unregister listener path " << unlisten_path << std::endl;

    // TODO[MB]: The key_prefix and address maps should be clears when they contain no more listeners
    //    By not clearing empty map, we keep creating etcd clients, which are will consume/drop notifications sent in
    //    the meanwhile. This needs to be fixed!!!

    for (auto&& [address, prefix_listerners] : listeners_) {
        for (auto&& [key_prefix, registered_listeners] : prefix_listerners) {
            registered_listeners.erase(
                std::remove_if(std::begin(registered_listeners),
                               std::end(registered_listeners),
                               [&unlisten_path](auto&& listener) { return listener.path() == unlisten_path; }),
                std::end(registered_listeners));
        }
    }
}

} // namespace aviso
