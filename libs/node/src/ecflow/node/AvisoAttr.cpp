/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/AvisoAttr.hpp"

#include <sstream>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/core/exceptions/Exceptions.hpp"
#include "ecflow/node/Node.hpp"

namespace ecf {

bool AvisoAttr::is_valid_name(const std::string& name) {
    return ecf::Str::valid_name(name);
}

AvisoAttr::AvisoAttr(Node* parent,
                     name_t name,
                     listener_t listener,
                     url_t url,
                     schema_t schema,
                     polling_t polling,
                     revision_t revision,
                     auth_t auth,
                     reason_t reason)
    : parent_{parent},
      parent_path_{parent ? parent->absNodePath() : ""},
      name_{std::move(name)},
      listener_{std::move(listener)},
      url_{std::move(url)},
      schema_{std::move(schema)},
      polling_{std::move(polling)},
      auth_{std::move(auth)},
      reason_{std::move(reason)},
      revision_{revision},
      controller_{nullptr} {
    if (!ecf::Str::valid_name(name_)) {
        throw ecf::InvalidArgument(ecf::Message("Invalid AvisoAttr name :", name_));
    }
}

void AvisoAttr::set_listener(std::string_view listener) {
    state_change_no_ = Ecf::incr_state_change_no();

    listener_ = listener;
}

void AvisoAttr::set_revision(revision_t revision) {
    state_change_no_ = Ecf::incr_state_change_no();

    revision_ = revision;
}

std::string AvisoAttr::path() const {
    std::string path = parent_path_;
    path += ':';
    path += name_;
    return path;
}

bool AvisoAttr::why(std::string& theReasonWhy) const {
    if (isFree()) {
        return false;
    }

    theReasonWhy += ecf::Message(" is Aviso dependent (", listener_, "), but no notification received");
    return true;
}

void AvisoAttr::reset() {
    state_change_no_ = Ecf::incr_state_change_no();

    if (parent_->state() == NState::QUEUED) {
        start();
    }
}

bool AvisoAttr::isFree() const {
    std::string aviso_path = path();

    ALOG(D, "AvisoAttr: check Aviso attribute (name: " << name_ << ", listener: " << listener_ << ") is free");

    if (controller_ == nullptr) {
        return false;
    }

    // Task associated with Attribute is free when any notification is found
    auto notifications = controller_->poll_notifications(aviso_path);

    if (notifications.empty()) {
        // No notifications, nothing to do -- task continues to wait
        return false;
    }

    // Notifications found -- task can continue

    // (a) get the latest revision
    auto max = std::max_element(notifications.begin(), notifications.end(), [](const auto& a, const auto& b) {
        return a.configuration.revision() < b.configuration.revision();
    });

    // (b) update the revision, in the listener configuration
    this->revision_ = max->configuration.revision();
    ALOG(D, "AvisoAttr: " << aviso_path << " updated revision to " << this->revision_);
    state_change_no_ = Ecf::incr_state_change_no();

    return true;
}

void AvisoAttr::start() const {
    LOG(Log::DBG, Message("AvisoAttr: subscribe Aviso attribute (name: ", name_, ", listener: ", listener_, ")"));

    // Path -- the unique identifier of the Aviso listener
    std::string aviso_path = path();

    // Listener -- the configuration for the Aviso listener
    auto aviso_listener = listener_;
    aviso_listener      = aviso_listener.substr(1, aviso_listener.size() - 2);

    // URL -- the URL for the Aviso server
    std::string aviso_url = url_;
    parent_->variableSubstitution(aviso_url);
    if (aviso_url.empty()) {
        throw std::runtime_error("AvisoAttr: invalid Aviso URL detected for " + aviso_path);
    }

    // Schema -- the path to the Schema used to interpret the Aviso notifications
    std::string aviso_schema = schema_;
    parent_->variableSubstitution(aviso_schema);

    std::string aviso_polling = polling_;
    parent_->variableSubstitution(aviso_polling);
    if (aviso_polling.empty()) {
        throw std::runtime_error("AvisoAttr: invalid Aviso polling interval detected for " + aviso_path);
    }
    auto polling = boost::lexical_cast<std::uint32_t>(aviso_polling);

    std::string aviso_auth = auth_;
    parent_->variableSubstitution(aviso_auth);

    start_controller(aviso_path, aviso_listener, aviso_url, aviso_schema, polling, aviso_auth);
}

void AvisoAttr::start_controller(const std::string& aviso_path,
                                 const std::string& aviso_listener,
                                 const std::string& aviso_url,
                                 const std::string& aviso_schema,
                                 std::uint32_t polling,
                                 const std::string& aviso_auth) const {

    // Controller -- start up the Aviso controller, and subscribe the Aviso listener
    controller_ = std::make_shared<controller_t>();
    controller_->subscribe(ecf::service::aviso::AvisoRequest::make_listen_start(
        aviso_path, aviso_listener, aviso_url, aviso_schema, polling, revision_, aviso_auth));
    // Controller -- effectively start the Aviso listener
    // n.b. this must be done after subscribing in the controller, so that the polling interval is set
    controller_->start();
}

void AvisoAttr::stop_controller(const std::string& aviso_path) const {
    if (controller_ != nullptr) {
        controller_->unsubscribe(ecf::service::aviso::AvisoRequest::make_listen_finish(aviso_path));

        // Controller -- shutdown up the Aviso controller
        controller_->stop();
        controller_ = nullptr;
    }
}

void AvisoAttr::finish() const {
    using namespace ecf;
    LOG(Log::DBG, Message("AvisoAttr: unsubscribe Aviso attribute (name: ", name_, ", listener: ", listener_, ")"));

    std::string aviso_path = path();
    stop_controller(aviso_path);
}

} // namespace ecf
