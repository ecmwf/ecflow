/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/service/MirrorService.hpp"

#include <thread>

#include "aviso/Log.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"

namespace ecf::service {

struct MirrorClient::Impl
{
    std::shared_ptr<Defs> defs_;
    ClientInvoker invoker_;
};

MirrorClient::MirrorClient() : impl_(std::make_unique<Impl>()) {
}

MirrorClient::~MirrorClient() = default;

int MirrorClient::get_node_status(const std::string& remote_host,
                                  const std::string& remote_port,
                                  const std::string& node_path) const {
    ALOG(D, "Sync'ing: using " << remote_host << ":" << remote_port << ", path=" << node_path);

    try {
        impl_ = std::make_unique<Impl>();
        impl_->invoker_.set_host_port(remote_host, remote_port);
        ALOG(D, "Sync'ing: retrieving the latest defs");
        impl_->invoker_.sync(impl_->defs_);
        ALOG(D, "Sync'ing finished");

        if (!impl_->defs_) {
            ALOG(E, "Sync'ing: unable to sync with remote defs");
            return NState::UNKNOWN;
        }

        auto node = impl_->defs_->findAbsNode(node_path);

        if (!node) {
            ALOG(E, "Sync'ing: requested node (" << node_path << ") not found in remote defs");
            return NState::UNKNOWN;
        }

        auto state = node->state();
        ALOG(D, "Sync'ing: found node (" << node_path << "), with status " << state);
        return state;
    }
    catch (std::exception& e) {
        ALOG(W, "Sync'ing: failure to sync, due to: " << e.what());
        return NState::UNKNOWN;
    }
}

MirrorRunner::MirrorRunner(MirrorController& controller)
    : base_t{controller,
             [this](const MirrorConfiguration& listener, const MirrorNotification& notification) {
                 typename MirrorController::notification_t n{listener.path, listener, notification};
                 MirrorRunner::notify(this->controller_, n);
                 // The following is a hack to force the server to increment the job generation count
                 ALOG(D, "Sync'ing: forcing server to traverse the defs");
                 TheOneServer::server().increment_job_generation_count();
             },
             [this]() { return MirrorRunner::subscribe(this->controller_); }} {};

void MirrorService::start() {

    // Update list of listeners

    auto new_subscriptions = subscribe_();
    for (auto&& subscription : new_subscriptions) {
        register_listener(subscription);
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

    ALOG(D, "Sync'ing: start polling, with polling interval: " << expiry << " s");
    executor_.start(std::chrono::seconds{expiry});
}

void MirrorService::operator()(const std::chrono::system_clock::time_point& now) {

    // Update list of listeners

    auto new_subscriptions = subscribe_();
    for (auto&& subscription : new_subscriptions) {
        register_listener(subscription);
    }

    // Check notification for each listener
    {
        for (auto& entry : listeners_) {
            ALOG(D,
                 "Mirroring: " << entry.mirror_request_.path << " node from " << entry.mirror_request_.host << ":"
                               << entry.mirror_request_.port);

            auto remote_path = entry.mirror_request_.path;
            auto remote_host = entry.mirror_request_.host;
            auto remote_port = entry.mirror_request_.port;

            // Collect the latest remote status
            auto latest_status = mirror_.get_node_status(remote_host, remote_port, remote_path);

            ALOG(D, "Mirroring: Notifying remote node state: " << latest_status);
            notify_(MirrorConfiguration{remote_path}, MirrorNotification{remote_path, latest_status});
        }
    }
}

void MirrorService::register_listener(const MirrorRequest& request) {
    ALOG(D, "Registering Mirror listener: {" << request.path << "}");
    listeners_.emplace_back(Entry{request});
}

void MirrorService::unregister_listener(const std::string& unlisten_path) {
    // Nothing to do ...
}

} // namespace ecf::service
