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
#include "ecflow/core/Overload.hpp"
#include "ecflow/core/exceptions/Exceptions.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/Operations.hpp"

namespace ecf {

bool MirrorAttr::is_valid_name(const std::string& name) {
    return ecf::Str::valid_name(name);
}

MirrorAttr::MirrorAttr(Node* parent,
                       name_t name,
                       remote_path_t remote_path,
                       remote_host_t remote_host,
                       remote_port_t remote_port,
                       polling_t polling,
                       flag_t ssl,
                       auth_t auth,
                       reason_t reason)
    : parent_{parent},
      name_{std::move(name)},
      remote_path_{std::move(remote_path)},
      remote_host_{std::move(remote_host)},
      remote_port_{std::move(remote_port)},
      polling_{std::move(polling)},
      ssl_{ssl},
      auth_{std::move(auth)},
      reason_{std::move(reason)},
      controller_{nullptr} {
    if (!is_valid_name(name_)) {
        throw ecf::InvalidArgument(ecf::Message("Invalid MirrorAttr name :", name_));
    }
}

MirrorAttr::~MirrorAttr() {
    stop_controller();
}

[[nodiscard]] MirrorAttr MirrorAttr::make_detached() const {
    MirrorAttr clone{*this};
    clone.parent_     = nullptr;
    clone.controller_ = nullptr;
    return clone;
}

std::string MirrorAttr::absolute_name() const {
    return parent_->absNodePath() + ':' + name_;
}

bool MirrorAttr::why(std::string& theReasonWhy) const {
    theReasonWhy += ecf::Message(" is a Mirror of ", remote_path(), " at '", remote_host(), ":", remote_port(), "'");
    return true;
}

void MirrorAttr::reset() {
    state_change_no_ = Ecf::incr_state_change_no();
    start_controller();
}

void MirrorAttr::finish() {
    stop_controller();
}

void MirrorAttr::mirror() {
    SLOG(D, "MirrorAttr: poll Mirror attribute '" << absolute_name() << "'");

    start_controller();

    // Task associated with Attribute is free when any notification is found
    if (auto notifications = controller_->get_notifications(remote_path_); !notifications.empty()) {

        // Update the 'local' state change number
        state_change_no_ = Ecf::incr_state_change_no();

        // Notifications found -- Node state to be updated or error to be reported
        std::visit(ecf::overload{[this](const service::mirror::MirrorNotification& notification) {
                                     SLOG(D,
                                          "MirrorAttr: Updating Mirror attribute (name: " << name_ << ") to state "
                                                                                          << notification.status());
                                     auto latest_state = static_cast<NState::State>(notification.status());
                                     reason_           = "";
                                     parent_->flag().clear(Flag::REMOTE_ERROR);
                                     parent_->flag().set_state_change_no(state_change_no_);
                                     parent_->setStateOnly(latest_state, true);
                                 },
                                 [this](const service::mirror::MirrorError& error) {
                                     SLOG(D,
                                          "MirrorAttr: Failure detected on Mirror attribute (name: "
                                              << name_ << ") due to " << error.reason());
                                     reason_ = error.reason();
                                     parent_->flag().set(Flag::REMOTE_ERROR);
                                     parent_->flag().set_state_change_no(state_change_no_);
                                     parent_->setStateOnly(NState::UNKNOWN, true);
                                 }},
                   notifications.back());

        // Propagate the 'local' state change number to all parents
        ecf::visit_parents(*parent_, [n = this->state_change_no_](Node& node) { node.set_state_change_no(n); });

        // Propagate the 'local' state change number to the top level suite
        auto find_suite = [](Node* node) -> Suite* {
            Node* top = node;
            while (top->parent() != nullptr) {
                top = top->parent();
            }
            if (top->isSuite()) {
                return static_cast<Suite*>(top);
            }
            return nullptr;
        };
        if (Suite* suite = find_suite(parent_); suite) {
            suite->Suite::set_state_change_no(state_change_no_);
        }
    }
    else {
        SLOG(D, "MirrorAttr: No notifications found for Mirror attribute (name: " << name_ << ")");
    }

    // No notifications, nothing to do...
}

void MirrorAttr::start_controller() const {
    if (controller_ == nullptr) {
        // Substitute variables in Mirror configuration
        std::string remote_host = remote_host_;
        parent_->variableSubstitution(remote_host);
        std::string remote_port = remote_port_;
        parent_->variableSubstitution(remote_port);
        std::string polling = polling_;
        parent_->variableSubstitution(polling);
        std::string auth = auth_;
        parent_->variableSubstitution(auth);

        SLOG(D,
             "MirrorAttr: start polling Mirror attribute '" << absolute_name() << "', from " << remote_path_ << " @ "
                                                            << remote_host << ':' << remote_port << ")");

        // Controller -- start up the Mirror controller, and configure the Mirror request
        controller_ = std::make_shared<controller_t>();
        controller_->subscribe(ecf::service::mirror::MirrorRequest{
            remote_path_, remote_host, remote_port, boost::lexical_cast<std::uint32_t>(polling), ssl_, auth});
        // Controller -- effectively start the Mirror process
        // n.b. this must be done after subscribing in the controller, so that the polling interval is set
        controller_->start();
    }
}

void MirrorAttr::stop_controller() const {
    if (controller_ != nullptr) {
        SLOG(D,
             "MirrorAttr: finishing polling for Mirror attribute \"" << parent_->absNodePath() << ":" << name_
                                                                     << "\", from host: " << remote_host_
                                                                     << ", port: " << remote_port_ << ")");

        controller_->stop();
        controller_.reset();
    }
}

bool operator==(const MirrorAttr& lhs, const MirrorAttr& rhs) {
    return lhs.name() == rhs.name() && lhs.remote_path() == rhs.remote_path() &&
           lhs.remote_host() == rhs.remote_host() && lhs.remote_port() == rhs.remote_port() &&
           lhs.polling() == rhs.polling() && lhs.ssl() == rhs.ssl() && lhs.auth() == rhs.auth() &&
           lhs.reason() == rhs.reason();
}

} // namespace ecf
