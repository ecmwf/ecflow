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
#include "ecflow/core/Overload.hpp"
#include "ecflow/core/exceptions/Exceptions.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/Operations.hpp"

namespace ecf {

namespace implementation {

std::string ensure_single_quotes(const AvisoAttr::listener_t listener) {
    using namespace std::string_literals;
    if (!listener.empty() && listener.front() == '\'' && listener.back() == '\'') {
        return listener;
    }
    else {
        return "'"s + listener + "'"s;
    }
}

} // namespace implementation

bool AvisoAttr::is_valid_name(const std::string& name) {
    return ecf::Str::valid_name(name);
}

AvisoAttr::AvisoAttr(Node* parent,
                     name_t name,
                     const listener_t& listener,
                     url_t url,
                     schema_t schema,
                     polling_t polling,
                     revision_t revision,
                     auth_t auth,
                     const reason_t& reason)
    : parent_{parent},
      parent_path_{parent ? parent->absNodePath() : ""},
      name_{std::move(name)},
      listener_{implementation::ensure_single_quotes(listener)},
      url_{std::move(url)},
      schema_{std::move(schema)},
      polling_{std::move(polling)},
      auth_{std::move(auth)},
      reason_{implementation::ensure_single_quotes(reason)},
      revision_{revision},
      controller_{nullptr} {
    if (!ecf::Str::valid_name(name_)) {
        throw ecf::InvalidArgument(ecf::Message("Invalid AvisoAttr name :", name_));
    }
}

AvisoAttr AvisoAttr::make_detached() const {
    AvisoAttr detached   = *this;
    detached.parent_     = nullptr;
    detached.controller_ = nullptr;
    return detached;
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

    if (parent_ && (parent_->state() == NState::QUEUED)) {
        start();
    }
}

void AvisoAttr::reload() {
    if (controller_) {
        state_change_no_ = Ecf::incr_state_change_no();
        finish();
        start();
    }
}

bool AvisoAttr::isFree() const {

    if (controller_ == nullptr) {
        return false;
    }

    // Task associated with Attribute is free when any notification is found
    auto notifications = controller_->get_notifications(this->path());

    if (notifications.empty()) {
        // No notifications, nothing to do -- task continues to wait
        SLOG(D,
             "AvisoAttr: (path: " << this->path() << ", name: " << name_ << ", listener: " << listener_
                                  << "): no notifications found");
        return false;
    }

    // Notifications found -- task can continue

    // (a) get the latest revision
    auto& back = notifications.back();

    state_change_no_ = Ecf::incr_state_change_no();

    // (b) update the revision, in the listener configuration
    auto is_free = std::visit(
        ecf::overload{
            [this](const ecf::service::aviso::NotificationPackage<service::aviso::ConfiguredListener,
                                                                  service::aviso::AvisoNotification>& response) {
                SLOG(D, "AvisoAttr::isFree: " << this->path() << " updated revision to " << this->revision_);
                this->revision_ = response.configuration.revision();
                parent_->get_flag().clear(Flag::REMOTE_ERROR);
                parent_->get_flag().set_state_change_no(state_change_no_);
                reason_ = "";
                return true;
            },
            [this](const ecf::service::aviso::AvisoNoMatch& response) {
                parent_->get_flag().clear(Flag::REMOTE_ERROR);
                parent_->get_flag().set_state_change_no(state_change_no_);
                reason_ = "";
                return false;
            },
            [this](const ecf::service::aviso::AvisoError& response) {
                parent_->get_flag().set(Flag::REMOTE_ERROR);
                parent_->get_flag().set_state_change_no(state_change_no_);
                reason_ = response.reason();
                return false;
            }},
        back);

    ecf::visit_parents(*parent_, [n = this->state_change_no_](Node& node) { node.set_state_change_no(n); });

    SLOG(D,
         "AvisoAttr: (path: " << this->path() << ", name: " << name_ << ", listener: " << listener_ << ") "
                              << std::string{(is_free ? "" : "no ")} + "notifications found");

    return is_free;
}

namespace {

void ensure_resolved_variable(std::string_view value, std::string_view default_value, std::string_view msg) {
    if (value.find(default_value) != std::string::npos) {
        throw std::runtime_error(Message(msg, value).str());
    }
}

} // namespace

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

    std::string aviso_auth = auth_;
    parent_->variableSubstitution(aviso_auth);

    ensure_resolved_variable(aviso_url, AvisoAttr::default_url, "AvisoAttr: failed to resolve Aviso URL: ");
    ensure_resolved_variable(aviso_schema, AvisoAttr::default_schema, "AvisoAttr: failed to resolve Aviso schema: ");
    ensure_resolved_variable(aviso_polling, AvisoAttr::default_polling, "AvisoAttr: failed to resolve Aviso polling: ");
    ensure_resolved_variable(aviso_auth, AvisoAttr::default_auth, "AvisoAttr: failed to resolve Aviso auth: ");

    std::uint32_t polling;
    try {
        polling = boost::lexical_cast<std::uint32_t>(aviso_polling);
    }
    catch (boost::bad_lexical_cast& e) {
        throw std::runtime_error(
            Message("AvisoAttr: failed to convert polling; expected an integer, but found: ", aviso_polling).str());
    }

    start_controller(aviso_path, aviso_listener, aviso_url, aviso_schema, polling, aviso_auth);
}

void AvisoAttr::start_controller(const std::string& aviso_path,
                                 const std::string& aviso_listener,
                                 const std::string& aviso_url,
                                 const std::string& aviso_schema,
                                 std::uint32_t polling,
                                 const std::string& aviso_auth) const {

    if (!controller_) {
        // Controller -- start up the Aviso controller, and subscribe the Aviso listener
        controller_ = std::make_shared<controller_t>();
        controller_->subscribe(ecf::service::aviso::AvisoSubscribe{
            aviso_path, aviso_listener, aviso_url, aviso_schema, polling, revision_, aviso_auth});
        // Controller -- effectively start the Aviso listener
        // n.b. this must be done after subscribing in the controller, so that the polling interval is set
        controller_->start();
    }
}

void AvisoAttr::stop_controller(const std::string& aviso_path) const {
    if (controller_ != nullptr) {
        SLOG(D, "AvisoAttr: finishing polling for Aviso attribute (" << parent_path_ << ":" << name_ << ")");

        controller_->subscribe(ecf::service::aviso::AvisoUnsubscribe{aviso_path});

        // Controller -- shutdown up the Aviso controller
        controller_->stop();
        controller_ = nullptr;
    }
}

void AvisoAttr::finish() const {
    using namespace ecf;

    std::string aviso_path = path();
    stop_controller(aviso_path);
}

void AvisoAttr::finish(const std::vector<AvisoAttr>& avisos) {
    for (const auto& aviso : avisos) {
        aviso.finish();
    }
}

void AvisoAttr::finish(const std::vector<AvisoAttr>& avisos, NState::State state) {
    if (NState::is_any_of<NState::ABORTED, NState::COMPLETE, NState::UNKNOWN>(state)) {
        finish(avisos);
    }
}

bool operator==(const AvisoAttr& lhs, const AvisoAttr& rhs) {
    return lhs.name() == rhs.name() && lhs.listener() == rhs.listener() && lhs.url() == rhs.url() &&
           lhs.schema() == rhs.schema() && lhs.polling() == rhs.polling() && lhs.revision() == rhs.revision() &&
           lhs.auth() == rhs.auth() && lhs.reason() == rhs.reason();
}

std::string to_python_string(const AvisoAttr& aviso) {
    std::string s;
    s += "AvisoAttr(";
    s += "name=";
    s += aviso.name();
    s += ", listener=";
    s += aviso.listener();
    s += ", url=";
    s += aviso.url();
    s += ", schema=";
    s += aviso.schema();
    s += ", polling=";
    s += aviso.polling();
    s += ", revision=";
    s += aviso.revision();
    s += ", auth=";
    s += aviso.auth();
    s += ", reason=";
    s += aviso.reason();
    s += ")";
    return s;
}

} // namespace ecf
