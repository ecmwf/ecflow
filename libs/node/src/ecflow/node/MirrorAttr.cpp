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

void MirrorAttr::reload() {
    if (controller_) {
        state_change_no_ = Ecf::incr_state_change_no();
        stop_controller();
        start_controller();
    }
}

void MirrorAttr::finish() {
    state_change_no_ = Ecf::incr_state_change_no();
    stop_controller();
}

void MirrorAttr::mirror() {
    SLOG(D, "MirrorAttr: poll Mirror attribute '" << absolute_name() << "'");

    start_controller();
    if (!controller_) {
        return; // Can't continue without a running controller...
    }

    // Task associated with Attribute is free when any notification is found
    if (auto notifications = controller_->get_notifications(remote_path_); !notifications.empty()) {

        // Update the 'local' state change number
        state_change_no_ = Ecf::incr_state_change_no();

        // Notifications found -- Node state to be updated or error to be reported
        std::visit(ecf::overload{[this](const service::mirror::MirrorNotification& notification) {
                                     auto latest_state = static_cast<NState::State>(notification.data().state);

                                     SLOG(D,
                                          "MirrorAttr: Updating Mirror attribute (name: " << name_ << ") to state "
                                                                                          << latest_state);

                                     // ** Node State
                                     reason_ = "";
                                     parent_->flag().clear(Flag::REMOTE_ERROR);
                                     parent_->flag().set_state_change_no(state_change_no_);
                                     parent_->setStateOnly(latest_state, true);

                                     // ** Node Variables
                                     std::vector<Variable> all_variables = notification.data().regular_variables;
                                     for (const auto& variable : notification.data().inherited_variables) {
                                         all_variables.push_back(variable);
                                     }
                                     for (const auto& variable : notification.data().generated_variables) {
                                         all_variables.push_back(variable);
                                     }

                                     parent_->replace_variables(all_variables);

                                     // ** Node Labels
                                     parent_->replace_labels(notification.data().labels);
                                     // ** Node Meters
                                     parent_->replace_meters(notification.data().meters);
                                     // ** Node Events
                                     parent_->replace_events(notification.data().events);
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

std::optional<std::string> MirrorAttr::resolve_cfg(const std::string& value, std::string_view default_value) const {
    // Substitude variable in local value
    std::string local = value;
    if(!parent_) {
        return std::nullopt;
    }

    parent_->variableSubstitution(local);

    // Ensure substituted value is not default
    if (local.find(default_value) != std::string::npos) {
        return std::nullopt;
    }

    return {local};
}

std::string MirrorAttr::resolve_cfg(const std::string& value,
                                    std::string_view default_value,
                                    std::string_view fallback_value) const {
    // Substitude variable in local value
    std::string local = value;
    parent_->variableSubstitution(local);

    // Ensure substituted value is not default
    if (local.find(default_value) != std::string::npos) {
        return std::string{fallback_value};
    }

    return local;
}

void MirrorAttr::start_controller() {
    if (!controller_) {

        // Resolve variables in configuration
        // In the case of the 'remote_host', we have to resolve the configuration
        std::string remote_host;
        if (auto found = resolve_cfg(remote_host_, default_remote_host); found) {
            remote_host = found.value();
        }
        else {
            // Update the 'local' state change number
            state_change_no_ = Ecf::incr_state_change_no();

            reason_ = Message("Unable to start mirror. Failed to resolve mirror remote host: ", remote_host_).str();
            if (parent_) {
                parent_->flag().set(Flag::REMOTE_ERROR);
                parent_->flag().set_state_change_no(state_change_no_);
                parent_->setStateOnly(NState::UNKNOWN, true);
            }
            return;
        }
        // For the remaining configuration, we use fallback values if the resolution fails
        auto remote_port = resolve_cfg(remote_port_, default_remote_port, fallback_remote_port);
        auto polling     = resolve_cfg(polling_, default_polling, fallback_polling);
        auto auth        = resolve_cfg(auth_, default_remote_auth, fallback_remote_auth);

        SLOG(D,
             "MirrorAttr: start polling Mirror attribute '" << absolute_name() << "', from " << remote_path_ << " @ "
                                                            << remote_host << ':' << remote_port << ") using polling: "
                                                            << polling << " s");

        std::uint32_t polling_value;
        try {
            polling_value = boost::lexical_cast<std::uint32_t>(polling);
        }
        catch (boost::bad_lexical_cast& e) {
            // Update the 'local' state change number
            state_change_no_ = Ecf::incr_state_change_no();

            reason_ =
                Message("Unable to start mirror. Failed to use polling; expected an integer, but found: ", polling)
                    .str();
            parent_->flag().set(Flag::REMOTE_ERROR);
            parent_->flag().set_state_change_no(state_change_no_);
            parent_->setStateOnly(NState::UNKNOWN, true);
            return;
        }

        // Controller -- start up the Mirror controller, and configure the Mirror request
        controller_ = std::make_shared<controller_t>();
        controller_->subscribe(
            ecf::service::mirror::MirrorRequest{remote_path_, remote_host, remote_port, polling_value, ssl_, auth});
        // Controller -- effectively start the Mirror process
        // n.b. this must be done after subscribing in the controller, so that the polling interval is set
        controller_->start();
    }
}

void MirrorAttr::stop_controller() {
    if (controller_) {
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
