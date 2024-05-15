/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/MirrorAttr.hpp"

#include <sstream>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/core/exceptions/Exceptions.hpp"
#include "ecflow/node/Node.hpp"

namespace ecf {

bool MirrorAttr::is_valid_name(const std::string& name) {
    return ecf::Str::valid_name(name);
}

MirrorAttr::MirrorAttr(Node* parent,
                       name_t name,
                       remote_path_t remote_path,
                       remote_host_t remote_host,
                       remote_port_t remote_port,
                       polling_t polling)
    : parent_{parent},
      name_{std::move(name)},
      remote_path_{std::move(remote_path)},
      remote_host_{std::move(remote_host)},
      remote_port_{std::move(remote_port)},
      polling_{std::move(polling)},
      controller_{nullptr} {
    if (!is_valid_name(name_)) {
        throw ecf::InvalidArgument(ecf::Message("Invalid MirrorAttr name :", name_));
    }
}

MirrorAttr::~MirrorAttr() {
    stop_controller();
}

bool MirrorAttr::why(std::string& theReasonWhy) const {
    theReasonWhy += ecf::Message(" is a Mirror of ", remote_path(), " at '", remote_host(), ":", remote_port(), "'");
    return true;
}

void MirrorAttr::reset() {
    state_change_no_ = Ecf::incr_state_change_no();

    ALOG(D,
         "MirrorAttr::reset: start polling for Mirror attribute (name: " << name_ << ", host: " << remote_host_
                                                                         << ", port: " << remote_port_ << ")");

    start_controller();
}

void MirrorAttr::finish() {

    ALOG(D,
         "MirrorAttr::reset: start polling for Mirror attribute (name: " << name_ << ", host: " << remote_host_
                                                                         << ", port: " << remote_port_ << ")");

    stop_controller();
}

void MirrorAttr::mirror() {
    ALOG(D, "**** Check Mirror attribute (name: " << name_ << ")");

    start_controller();

    // Task associated with Attribute is free when any notification is found
    if (auto notifications = controller_->poll_notifications(remote_path_); !notifications.empty()) {
        // Notifications found -- Node state to be updated
        ALOG(D, "MirrorAttr::isFree: found notifications for Mirror attribute (name: " << name_ << ")");

        auto latest_state = static_cast<NState::State>(notifications.back().status);
        parent_->setStateOnly(latest_state, true);
        parent_->handleStateChange();
    }

    // No notifications, nothing to do...
}

void MirrorAttr::start_controller() const {
    if (controller_ == nullptr) {
        ALOG(D,
             "MirrorAttr::reset: start polling for Mirror attribute (name: " << name_ << ", host: " << remote_host_
                                                                             << ", port: " << remote_port_ << ")");

        // Substitute variables in Mirror configuration
        std::string remote_host = remote_host_;
        parent_->variableSubstitution(remote_host);
        std::string remote_port = remote_port_;
        parent_->variableSubstitution(remote_port);
        std::string polling = polling_;
        parent_->variableSubstitution(polling);

        // Controller -- start up the Mirror controller, and configure the Mirror request
        controller_ = std::make_shared<controller_t>();
        controller_->subscribe(ecf::service::mirror::MirrorRequest{
            remote_path_, remote_host, remote_port, boost::lexical_cast<std::uint32_t>(polling)});
        // Controller -- effectively start the Mirror process
        // n.b. this must be done after subscribing in the controller, so that the polling interval is set
        controller_->start();
    }
}

void MirrorAttr::stop_controller() const {
    if (controller_ != nullptr) {
        ALOG(D,
             "MirrorAttr::reset: stop polling for Mirror attribute (name: " << name_ << ", host: " << remote_host_
                                                                            << ", port: " << remote_port_ << ")");

        controller_->stop();
        controller_.reset();
    }
}

} // namespace ecf
