// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/AvisoAttr.hpp"

#include <algorithm>
#include <sstream>

#include <nlohmann/json.hpp>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/core/Overload.hpp"
#include "ecflow/core/exceptions/Exceptions.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/Operations.hpp"

namespace ecf {

AvisoEvent AvisoEvent::from(const ecf::service::aviso::AvisoNotification& notification) {
    return AvisoEvent{notification.event_type(),
                      notification.sequence(),
                      notification.identifier_json(),
                      notification.payload_json()};
}

AvisoEvent AvisoEvent::from_option(const std::string& option) {
    std::string text = option;
    if (text.size() >= 2 && text.front() == '\'' && text.back() == '\'') {
        text = text.substr(1, text.size() - 2);
    }
    try {
        auto event = nlohmann::ordered_json::parse(text);
        return AvisoEvent{event.at("type").get<std::string>(),
                          event.at("sequence").get<sequence_t>(),
                          event.at("identifier").get<std::string>(),
                          event.at("payload").get<std::string>()};
    }
    catch (const nlohmann::ordered_json::exception& e) {
        throw std::runtime_error("AvisoAttr: invalid event " + option + " (" + e.what() + ")");
    }
}

std::string AvisoEvent::to_option() const {
    nlohmann::ordered_json event{
        {"type", type}, {"sequence", sequence}, {"identifier", identifier}, {"payload", payload}};
    // Single quotes only occur within JSON strings, where they can be escaped, so that the value can be quoted
    std::string text = event.dump();
    std::string escaped;
    escaped.reserve(text.size());
    for (char c : text) {
        if (c == '\'') {
            escaped += "\\u0027";
        }
        else {
            escaped += c;
        }
    }
    return "'" + escaped + "'";
}

bool operator==(const AvisoEvent& lhs, const AvisoEvent& rhs) {
    return lhs.type == rhs.type && lhs.sequence == rhs.sequence && lhs.identifier == rhs.identifier &&
           lhs.payload == rhs.payload;
}

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
                     const reason_t& reason,
                     bool collapse)
    : parent_{parent},
      parent_path_{parent ? parent->absNodePath() : ""},
      name_{std::move(name)},
      listener_{implementation::ensure_single_quotes(listener)},
      url_{std::move(url)},
      auth_{std::move(auth)},
      collapse_{collapse},
      reason_{implementation::ensure_single_quotes(reason)},
      revision_{revision} {
    if (!ecf::algorithm::is_valid_name(name_)) {
        THROW_EXCEPTION(ecf::InvalidArgument, "Invalid AvisoAttr name :" << name_);
    }
}

AvisoAttr AvisoAttr::make_detached() const {
    AvisoAttr detached = *this;
    detached.parent_   = nullptr;
    detached.backend_  = nullptr;
    detached.queued_.clear();
    detached.pending_.reset();
    return detached;
}

std::string AvisoAttr::path() const {
    std::string path = parent_path_;
    path += ':';
    path += name_;
    return path;
}

void AvisoAttr::set_event(const AvisoEvent& event) {
    state_change_no_ = Ecf::incr_state_change_no();
    event_           = event;
}

void AvisoAttr::update_gen_variables() const {
    // The job is generated while the node is still queued, before the held notification is committed
    const AvisoEvent event = pending_ ? AvisoEvent::from(*pending_) : event_;
    genvar_event_type_.set_value(event.type);
    genvar_event_sequence_.set_value(std::to_string(event.sequence));
    genvar_event_data_identifier_.set_value(event.identifier);
    genvar_event_data_payload_.set_value(event.payload);
}

void AvisoAttr::gen_variables(std::vector<Variable>& vars) const {
    update_gen_variables();
    vars.push_back(genvar_event_type_);
    vars.push_back(genvar_event_sequence_);
    vars.push_back(genvar_event_data_identifier_);
    vars.push_back(genvar_event_data_payload_);
}

const Variable& AvisoAttr::find_gen_variable(const std::string& name) const {
    for (const auto* var :
         {&genvar_event_type_, &genvar_event_sequence_, &genvar_event_data_identifier_, &genvar_event_data_payload_}) {
        if (var->name() == name) {
            // Refreshed only when found, as every variable lookup on the node reaches this function
            update_gen_variables();
            return *var;
        }
    }
    return Variable::EMPTY();
}

bool AvisoAttr::has_error() const {
    return !reason_.empty() && reason_ != "''";
}

bool AvisoAttr::why(std::string& theReasonWhy) const {
    if (isFree()) {
        return false;
    }

    theReasonWhy += ecf::Message(" is Aviso dependent (", listener_, "), but no notification received");
    return true;
}

void AvisoAttr::reset() {
    free_            = false;
    state_change_no_ = Ecf::incr_state_change_no();

    if (parent_ && (parent_->state() == NState::QUEUED)) {
        start();
    }
}

void AvisoAttr::reload() {
    // An attribute is reloaded when it is running, but also when its node is queued without a running attribute
    // (e.g. after a configuration error), so that a corrected configuration takes effect
    bool is_queued = parent_ && parent_->state() == NState::QUEUED;
    if (backend_ || is_queued) {
        state_change_no_ = Ecf::incr_state_change_no();
        finish();
        start();
    }
}

bool AvisoAttr::isFree() const {

    // The attribute released the node and the node was not queued again (e.g. it is retried after an abort): the
    // attribute stays free, as a time attribute whose slot fired
    if (free_) {
        return true;
    }

    // A notification already consumed keeps the node free, until the node is started again
    if (pending_) {
        return true;
    }

    if (backend_ == nullptr) {
        return false;
    }

    // Errors and (re)started watches update the error flag and reason; notifications not yet consumed are queued
    for (const auto& response : backend_->drain()) {
        std::visit(ecf::overload{[this](const ecf::service::aviso::AvisoNotification& notification) {
                                     bool is_new = notification.sequence() > revision_ &&
                                                   std::none_of(queued_.begin(), queued_.end(), [&](const auto& q) {
                                                       return q.sequence() == notification.sequence();
                                                   });
                                     if (is_new) {
                                         queued_.push_back(notification);
                                     }
                                 },
                                 [this](const ecf::service::aviso::AvisoError& error) { set_error(error.reason()); },
                                 [this](const ecf::service::aviso::AvisoWatchStarted&) { clear_error(); }},
                   response);
    }

    if (queued_.empty()) {
        SLOG(D,
             "AvisoAttr: (path: " << this->path() << ", name: " << name_ << ", listener: " << listener_
                                  << "): no notifications found");
        return false;
    }

    // Consume the oldest notification, or all of them when collapsing
    std::sort(queued_.begin(), queued_.end(), [](const auto& a, const auto& b) { return a.sequence() < b.sequence(); });
    if (collapse_) {
        pending_ = queued_.back();
        queued_.clear();
    }
    else {
        pending_ = queued_.front();
        queued_.erase(queued_.begin());
    }

    clear_error();
    SLOG(D,
         "AvisoAttr::isFree: " << this->path() << " holds notification " << pending_->sequence() << " ("
                               << queued_.size() << " queued)");

    return true;
}

namespace {

bool is_unresolved(std::string_view value, std::string_view default_value) {
    return value.find(default_value) != std::string::npos;
}

} // namespace

void AvisoAttr::start() const {
    free_ = false;
    // The node is started again. A notification held but not committed (the node was not released) is queued
    // again, while the backend runs; the running watch keeps its configuration (reload() applies a new one)
    if (backend_) {
        if (pending_) {
            queued_.push_back(*pending_);
        }
        pending_.reset();
        return;
    }

    // Otherwise, a new backend delivers again every notification after the revision
    pending_.reset();
    queued_.clear();

    if (!parent_) {
        return;
    }

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

    std::string aviso_auth = auth_;
    parent_->variableSubstitution(aviso_auth);

    // A configuration error is reported on the node, which stays queued; it never fails the command (e.g. begin,
    // requeue) that started the attribute, so that the other nodes are not affected
    std::string error;
    if (aviso_url.empty()) {
        error = "Aviso URL is empty (see option --url, or variable ECF_AVISO_URL)";
    }
    else if (is_unresolved(aviso_url, AvisoAttr::default_url)) {
        error = "failed to resolve Aviso URL " + aviso_url + " (define variable ECF_AVISO_URL)";
    }
    else if (is_unresolved(aviso_auth, AvisoAttr::default_auth)) {
        error = "failed to resolve Aviso credentials " + aviso_auth + " (define variable ECF_AVISO_AUTH)";
    }

    if (!error.empty()) {
        LOG(Log::ERR, Message("AvisoAttr: unable to start ", aviso_path, ": ", error));
        set_error(error);
        return;
    }

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

    // The configuration is complete; any earlier configuration error no longer applies
    clear_error();
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
    if (!has_error()) {
        return;
    }
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

void AvisoAttr::commit() const {
    if (pending_) {
        state_change_no_ = Ecf::incr_state_change_no();
        revision_        = std::max(revision_, pending_->sequence());
        free_            = true;
        // The notification that released the node remains available to the job, as generated variables
        event_ = AvisoEvent::from(*pending_);
        pending_.reset();
        SLOG(D, "AvisoAttr::commit: " << this->path() << " consumed notification " << revision_);
    }
}

void AvisoAttr::state_changed(const std::vector<AvisoAttr>& avisos, NState::State state) {
    // The node is queued again (e.g. forced): the attributes watch again
    if (state == NState::QUEUED) {
        for (const auto& aviso : avisos) {
            aviso.start();
        }
        return;
    }
    // The node leaves the queued state: the notification that released it is consumed
    for (const auto& aviso : avisos) {
        aviso.commit();
    }
    if (NState::is_any_of<NState::ABORTED, NState::COMPLETE, NState::UNKNOWN>(state)) {
        finish(avisos);
    }
}

bool operator==(const AvisoAttr& lhs, const AvisoAttr& rhs) {
    return lhs.name() == rhs.name() && lhs.listener() == rhs.listener() && lhs.url() == rhs.url() &&
           lhs.revision() == rhs.revision() && lhs.auth() == rhs.auth() && lhs.reason() == rhs.reason() &&
           lhs.collapse() == rhs.collapse() && lhs.event() == rhs.event() && lhs.isSetFree() == rhs.isSetFree();
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
    s += ", collapse=";
    s += aviso.collapse() ? "true" : "false";
    s += ")";
    return s;
}

} // namespace ecf
