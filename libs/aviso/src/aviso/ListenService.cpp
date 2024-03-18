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
                    ALOG(D, "Updating revision for " << entry.path() << " to " << value);
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
    auto listener = create_configured_listener(listen, schema_);
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

int ListenService::load_default_polling_interval() {
    nlohmann::json json;
    std::ifstream file(load_cfg_location());
    if (file.is_open()) {
        file >> json;
        file.close();
    }

    int polling_interval = 20;
    if (json.contains("polling_interval")) {
        polling_interval = json["polling_interval"];
        ALOG(I, "Aviso polling interval loaded from configuration file: " << polling_interval << "s");
        return polling_interval;
    }

    ALOG(W, "Polling interval not found in configuration file. Using default value: " << polling_interval << "s");
    return polling_interval;
}

std::string ListenService::load_cfg_location() {
    return "ecf_aviso.json";
}

} // namespace aviso
