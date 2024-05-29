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

namespace ecf::service::mirror {

namespace {

std::pair<std::string, std::string> load_auth_credentials(const std::string& auth_file) {
    std::ifstream file(auth_file);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file: " + auth_file);
    }

    std::string username, password;
    std::getline(file, username);
    std::getline(file, password);
    file.close();

    return {username, password};
}

} // namespace

/* MirrorService */

std::optional<std::string> MirrorService::key(const MirrorService::notification_t& notification) {
    return std::visit(ecf::overload{[](const service::mirror::MirrorNotification& notification) {
                                        return std::make_optional(notification.path);
                                    },
                                    [](const service::mirror::MirrorError&) { return std::optional<std::string>{}; }},
                      notification);
}

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
                auto latest_status =
                    mirror_.get_node_status(remote_host, remote_port, remote_path, ssl, remote_user, remote_pass);

                SLOG(D, "MirrorService: Notifying remote node state: " << latest_status);
                MirrorNotification notification{true, remote_path, "", latest_status};
                notify_(notification);
            }
            catch (std::runtime_error& e) {
                SLOG(W, "MirrorService: Failed to sync with remote node: " << e.what());
                MirrorNotification notification{false, remote_path, e.what(), -1};
                notify_(notification);
            }
        }
    }
}

void MirrorService::register_listener(const MirrorRequest& request) {
    SLOG(D, "MirrorService: Registering Mirror: {" << request.path << "}");
    Entry& inserted = listeners_.emplace_back(Entry{request, "", ""});
    if (!request.auth.empty()) {
        SLOG(D, "MirrorService: Loading auth {" << request.auth << "}");
        try {
            auto [username, password] = load_auth_credentials(request.auth);
            inserted.remote_username_ = username;
            inserted.remote_password_ = password;
        }
        catch (std::runtime_error& e) {
            throw std::runtime_error("MirrorService: Unable to load auth credentials");
        }
    }
}

MirrorController::MirrorController()
    : base_t{[this](const MirrorService::notification_t& notification) {
                 this->notify(notification);
                 // The following is a hack to force the server to increment the job generation count
                 SLOG(D, "MirrorController: forcing server to traverse the defs");
                 TheOneServer::server().increment_job_generation_count();
             },
             [this]() { return this->get_subscriptions(); }} {
}

} // namespace ecf::service::mirror
