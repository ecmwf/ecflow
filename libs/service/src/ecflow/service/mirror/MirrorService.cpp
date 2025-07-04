/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/service/mirror/MirrorService.hpp"

#include <thread>
#include <utility>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/core/Overload.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/service/Log.hpp"
#include "ecflow/service/Registry.hpp"
#include "ecflow/service/auth/Credentials.hpp"

namespace ecf::service::mirror {

/* MirrorService */

void MirrorService::start() {

    // Update list of listeners

    auto new_subscriptions = subscribe_();
    for (auto&& subscription : new_subscriptions) {
        try {
            register_listener(subscription);
        }
        catch (std::runtime_error& e) {
            SLOG(E, "MirrorService: Unable to register listener: " << e.what());
            // TODO[MB]: Must gracefully handle this error, by notifying the main thread and providing a reason
        }
    }

    // Start polling...

    std::uint32_t expiry = 40;

    if (!listeners_.empty()) {
        std::vector<std::uint32_t> polling_intervals;
        auto found = std::max_element(std::begin(listeners_), std::end(listeners_), [](const auto& a, const auto& b) {
            return a.mirror_request_.polling < b.mirror_request_.polling;
        });
        expiry     = found->mirror_request_.polling;
    }

    SLOG(D, "MirrorService: start polling, with polling interval: " << expiry << " s");
    executor_.start(std::chrono::seconds{expiry});
}

void MirrorService::operator()(const std::chrono::system_clock::time_point& now) {

    // Check notification for each listener
    {
        for (auto& entry : listeners_) {
            SLOG(D,
                 "MirrorService: Mirroring " << entry.mirror_request_.path << " node from "
                                             << entry.mirror_request_.host << ":" << entry.mirror_request_.port);

            auto remote_path = entry.mirror_request_.path;
            auto remote_host = entry.mirror_request_.host;
            auto remote_port = entry.mirror_request_.port;
            auto ssl         = entry.mirror_request_.ssl;
            auto remote_user = entry.remote_username_;
            auto remote_pass = entry.remote_password_;

            // Collect the latest remote status
            try {
                auto data =
                    mirror_.get_node_status(remote_host, remote_port, remote_path, ssl, remote_user, remote_pass);

                SLOG(D, "MirrorService: Notifying remote node state: " << data.state);
                MirrorNotification notification{remote_path, data};
                notify_(notification);
            }
            catch (std::runtime_error& e) {
                SLOG(W, "MirrorService: Failed to sync with remote node: " << e.what());
                MirrorError error{remote_path, e.what()};
                notify_(error);
            }
        }
    }
}

void MirrorService::register_listener(const MirrorRequest& request) {
    SLOG(D, "MirrorService: Registering Mirror: {" << request.path << "}");
    Entry& inserted = listeners_.emplace_back(Entry{request, "", ""});
    if (!request.auth.empty()) {
        SLOG(D, "MirrorService: Loading auth {" << request.auth << "}");
        auto found = ecf::service::auth::Credentials::load(request.auth);
        std::visit(ecf::overload{[&](const ecf::service::auth::Credentials& credentials) {
                                     auto url = credentials.value("url").value_or("unknown");
                                     SLOG(D, "MirrorService: using credentials for mirror: " << url);

                                     if (auto user = credentials.user(); user) {
                                         inserted.remote_username_ = user->username;
                                         inserted.remote_password_ = user->password;
                                     }
                                 },
                                 [](const ecf::service::auth::Credentials::Error& error) {
                                     SLOG(E, "MirrorService: unable to load credentials: " << error.message);
                                 }},
                   found);
    }
}

MirrorController::MirrorController()
    : base_t{[this](const MirrorService::notification_t& notification) {
                 this->notify(notification);

                 if (auto* server = TheOneServer::server(); server) {
                     // The following forces the server to increment the job generation count and traverse the defs
                     server->increment_job_generation_count();
                 }
                 else {
                     SLOG(E, "MirrorController: no server available, thus unable to increment job generation count");
                 }
             },
             [this]() { return this->get_subscriptions(); }} {
}

} // namespace ecf::service::mirror
