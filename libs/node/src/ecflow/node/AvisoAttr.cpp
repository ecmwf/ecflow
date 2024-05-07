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

AvisoAttr::AvisoAttr(Node* parent,
                     name_t name,
                     listener_t listener,
                     url_t url,
                     schema_t schema,
                     polling_t polling,
                     revision_t revision)
    : parent_{parent},
      parent_path_{parent->absNodePath()},
      name_{std::move(name)},
      listener_{std::move(listener)},
      url_{std::move(url)},
      schema_{std::move(schema)},
      polling_{std::move(polling)},
      revision_{revision},
      controller_{nullptr},
      runner_{nullptr} {
    if (!ecf::Str::valid_name(name_)) {
        throw ecf::InvalidArgument(ecf::Message("Invalid AvisoAttr name :", name_));
    }
};

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
};

void AvisoAttr::reset() {
    state_change_no_ = Ecf::incr_state_change_no();
}

bool AvisoAttr::isFree() const {
    std::string aviso_path = path();

    LOG(Log::MSG, "**** Check Aviso attribute (name: " << name_ << ", listener: " << listener_ << ")");

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
    ALOG(D, "AvisoAttr::isFree: " << aviso_path << " updated revision to " << this->revision_);
    state_change_no_ = Ecf::incr_state_change_no();

    return true;
}

void AvisoAttr::start() const {
    LOG(Log::DBG, Message("**** Subscribe Aviso attribute (name: ", name_, ", listener: ", listener_, ")"));

    // Path -- the unique identifier of the Aviso listener
    std::string aviso_path = path();

    // Listener -- the configuration for the Aviso listener
    auto aviso_listener = listener_;
    aviso_listener      = aviso_listener.substr(1, aviso_listener.size() - 2);

    // URL -- the URL for the Aviso server
    std::string aviso_url = url_;
    parent_->variableSubstitution(aviso_url);
    if (aviso_url.empty()) {
        throw std::runtime_error("AvisoAttr::requeue: Invalid Aviso URL detected for " + aviso_path);
    }

    // Schema -- the path to the Schema used to interpret the Aviso notifications
    std::string aviso_schema = schema_;
    parent_->variableSubstitution(aviso_schema);

    std::string aviso_polling = polling_;
    parent_->variableSubstitution(aviso_polling);
    if (aviso_polling.empty()) {
        throw std::runtime_error("AvisoAttr::requeue: Invalid Aviso polling interval detected for " + aviso_path);
    }
    auto polling = boost::lexical_cast<std::uint32_t>(aviso_polling);

    // Controller -- start up the Aviso controller
    controller_ = std::make_shared<ecf::service::AvisoController>();
    controller_->subscribe(aviso::ListenRequest::make_listen_start(
        aviso_path, aviso_listener, aviso_url, aviso_schema, polling, revision_));
    // Runner -- start up the Aviso runner, and thus effectively start the Aviso listener
    // n.b. this must be done after subscribing in the controller, so that the polling interval is set
    runner_ = std::make_shared<ecf::service::AvisoRunner>(*controller_);
    runner_->start();
}

void AvisoAttr::finish() const {
    using namespace ecf;
    LOG(Log::DBG, Message("**** Unsubscribe Aviso attribute (name: ", name_, ", listener: ", listener_, ")"));

    std::string aviso_path = path();
    controller_->unsubscribe(aviso::ListenRequest::make_listen_finish(aviso_path));

    // Controller -- shutdown up the Aviso controller
    runner_->stop();
    runner_     = nullptr;
    controller_ = nullptr;
}

} // namespace ecf
