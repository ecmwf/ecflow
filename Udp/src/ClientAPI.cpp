/*
 * Copyright 2009-2023 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ClientAPI.hpp"

#include "ClientInvoker.hpp"

namespace ecf {

template <typename F>
void ClientAPI::try_invoke(F f) const {
    try {
        f(invoker_);
    }
    catch (std::runtime_error& e) {
        throw ClientAPIException(std::string("Client error detected: ") + e.what());
    }
    catch (...) {
        throw ClientAPIException("Unknown client error detected");
    }
}

ClientAPI::ClientAPI() : invoker_(std::make_unique<ClientInvoker>()) {
}

ClientAPI::~ClientAPI() = default;

void ClientAPI::user_set_name(const std::string& username) {
    invoker_->set_user_name(username);
}

void ClientAPI::user_set_password(const std::string& password) {
    invoker_->set_password(password);
}

void ClientAPI::user_update_meter(const std::string& path, const std::string& name, const std::string& value) const {
    try_invoke([&path, &name, &value](const auto& invoker) { invoker->alter(path, "change", "meter", name, value); });
}

void ClientAPI::user_update_label(const std::string& path, const std::string& name, const std::string& value) const {
    try_invoke([&path, &name, &value](const auto& invoker) { invoker->alter(path, "change", "label", name, value); });
}

void ClientAPI::user_clear_event(const std::string& path, const std::string& name) const {
    try_invoke([&path, &name](const auto& invoker) { invoker->alter(path, "change", "event", name, "clear"); });
}
void ClientAPI::user_set_event(const std::string& path, const std::string& name) const {
    try_invoke([&path, &name](const auto& invoker) { invoker->alter(path, "change", "event", name, "set"); });
}

void ClientAPI::child_set_remote_id(const std::string& pid) {
    invoker_->set_child_pid(pid);
}

void ClientAPI::child_set_password(const std::string& password) {
    invoker_->set_child_password(password);
}

void ClientAPI::child_set_try_no(int try_no) {
    invoker_->set_child_try_no(try_no);
}

void ClientAPI::child_update_meter(const std::string& path, const std::string& name, const std::string& value) const {
    invoker_->set_child_path(path);
    try_invoke([name, value](const auto& invoker) { invoker->meterTask(name, value); });
}

void ClientAPI::child_update_label(const std::string& path, const std::string& name, const std::string& value) const {
    invoker_->set_child_path(path);
    try_invoke([name, value](const auto& invoker) { invoker->labelTask(name, std::vector<std::string>{value}); });
}

void ClientAPI::child_clear_event(const std::string& path, const std::string& name) const {
    invoker_->set_child_path(path);
    try_invoke([name](const auto& invoker) { invoker->eventTask(name, "clear"); });
}
void ClientAPI::child_set_event(const std::string& path, const std::string& name) const {
    invoker_->set_child_path(path);
    try_invoke([name](const auto& invoker) { invoker->eventTask(name, "set"); });
}

} // namespace ecf
