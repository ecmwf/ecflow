// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

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

std::string ensure_single_quotes(const AvisoAttr::listener_t& listener) {
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
    return ecf::algorithm::is_valid_name(name);
}

AvisoAttr::AvisoAttr(Node* parent,
                     name_t name,
                     const listener_t& listener,
                     url_t url,
                     revision_t revision,
                     auth_t auth,
                     const reason_t& reason)
    : parent_{parent},
      parent_path_{parent ? parent->absNodePath() : ""},
      name_{std::move(name)},
      listener_{implementation::ensure_single_quotes(listener)},
      url_{std::move(url)},
      auth_{std::move(auth)},
      reason_{implementation::ensure_single_quotes(reason)},
      revision_{revision},
      backend_{nullptr} {
    if (!ecf::algorithm::is_valid_name(name_)) {
        THROW_EXCEPTION(ecf::InvalidArgument, "Invalid AvisoAttr name :" << name_);
    }
}

AvisoAttr AvisoAttr::make_detached() const {
    AvisoAttr detached = *this;
    detached.parent_   = nullptr;
    detached.backend_  = nullptr;
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
    if (backend_) {
        state_change_no_ = Ecf::incr_state_change_no();
        finish();
        start();
    }
}

bool AvisoAttr::isFree() const {

    if (backend_ == nullptr) {
        return false;
    }

    // Task associated with Attribute is free when any notification is found
    auto responses = backend_->drain();

    if (responses.empty()) {
        // No notifications, nothing to do -- task continues to wait
        SLOG(D,
             "AvisoAttr: (path: " << this->path() << ", name: " << name_ << ", listener: " << listener_
                                  << "): no notifications found");
        return false;
    }

    // Only the latest response is relevant
    const auto& latest = responses.back();

    auto is_free = std::visit(
        ecf::overload{[this](const ecf::service::aviso::AvisoNotification& notification) {
                          this->revision_ = notification.sequence();
                          SLOG(D, "AvisoAttr::isFree: " << this->path() << " updated revision to " << this->revision_);
                          clear_error();
                          return true;
                      },
                      [this](const ecf::service::aviso::AvisoError& error) {
                          set_error(error.reason());
                          return false;
                      }},
        latest);

    SLOG(D,
         "AvisoAttr: (path: " << this->path() << ", name: " << name_ << ", listener: " << listener_ << ") "
                              << std::string{(is_free ? "" : "no ")} + "notifications found");

    return is_free;
}

namespace {

void ensure_resolved_variable(std::string_view value, std::string_view default_value, std::string_view msg) {
    if (value.find(default_value) != std::string::npos) {
        THROW_RUNTIME(msg << value);
    }
}

} // namespace

void AvisoAttr::start() const {
    LOG(Log::DBG, Message("AvisoAttr: subscribe Aviso attribute (name: ", name_, ", listener: ", listener_, ")"));

    // Path -- the unique identifier of the Aviso listener
    std::string aviso_path = path();

    // Listener -- the configuration for the Aviso listener
    active_ = listener_;
    //  .. replace ecflow variables in the listener
    parent_->variableSubstitution(active_);
    //  .. ensure that the listener is a single-quoted string
    active_ = active_.substr(1, active_.size() - 2);
    LOG(Log::DBG, Message("AvisoAttr: listener after variable substitution: ", active_, ")"));

    // URL -- the URL for the Aviso server
    std::string aviso_url = url_;
    parent_->variableSubstitution(aviso_url);
    if (aviso_url.empty()) {
        THROW_RUNTIME("AvisoAttr: invalid Aviso URL detected for " + aviso_path);
    }

    std::string aviso_auth = auth_;
    parent_->variableSubstitution(aviso_auth);

    ensure_resolved_variable(aviso_url, AvisoAttr::default_url, "AvisoAttr: failed to resolve Aviso URL: ");
    ensure_resolved_variable(aviso_auth, AvisoAttr::default_auth, "AvisoAttr: failed to resolve Aviso auth: ");

    start_backend(aviso_path, active_, aviso_url, aviso_auth);
}

void AvisoAttr::start_backend(const std::string& aviso_path,
                              const std::string& aviso_listener,
                              const std::string& aviso_url,
                              const std::string& aviso_auth) const {

    if (backend_) {
        return;
    }

    backend_ = ecf::service::aviso::make_backend();
    if (!backend_) {
        // No backend available (e.g. a server built without Aviso support) -- the task remains queued
        set_error(std::string{ecf::service::aviso::no_backend});
        return;
    }

    backend_->subscribe(
        ecf::service::aviso::AvisoSubscribe{aviso_path, aviso_listener, aviso_url, revision_, aviso_auth});

    state_change_no_ = Ecf::incr_state_change_no();
}

void AvisoAttr::stop_backend() const {
    if (backend_ != nullptr) {
        SLOG(D, "AvisoAttr: finishing notifications for Aviso attribute (" << parent_path_ << ":" << name_ << ")");

        backend_ = nullptr;

        state_change_no_ = Ecf::incr_state_change_no();
    }

    // Reset the configured listener buffer
    active_ = "";
}

void AvisoAttr::set_error(const std::string& reason) const {
    state_change_no_ = Ecf::incr_state_change_no();
    reason_          = implementation::ensure_single_quotes(reason);
    if (parent_) {
        parent_->get_flag().set(Flag::REMOTE_ERROR);
        parent_->get_flag().set_state_change_no(state_change_no_);
        ecf::visit_parents(*parent_, [n = this->state_change_no_](Node& node) { node.set_state_change_no(n); });
    }
}

void AvisoAttr::clear_error() const {
    state_change_no_ = Ecf::incr_state_change_no();
    reason_          = implementation::ensure_single_quotes("");
    if (parent_) {
        parent_->get_flag().clear(Flag::REMOTE_ERROR);
        parent_->get_flag().set_state_change_no(state_change_no_);
        ecf::visit_parents(*parent_, [n = this->state_change_no_](Node& node) { node.set_state_change_no(n); });
    }
}

void AvisoAttr::finish() const {
    stop_backend();
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
           lhs.revision() == rhs.revision() && lhs.auth() == rhs.auth() && lhs.reason() == rhs.reason();
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
    s += ", revision=";
    s += std::to_string(aviso.revision());
    s += ", auth=";
    s += aviso.auth();
    s += ", reason=";
    s += aviso.reason();
    s += ")";
    return s;
}

} // namespace ecf
