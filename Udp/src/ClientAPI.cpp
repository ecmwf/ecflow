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

namespace /* __anonymous__ */ {

void update_attribute(const ClientInvoker& invoker,
                      const std::string& path,
                      const std::string& type,
                      const std::string& name,
                      const std::string& value) {
    try {
        invoker.alter(path, "change", type, name, value);
    }
    catch (std::runtime_error& e) {
        throw ClientAPIException(std::string("Client error detected: ") + e.what());
    }
    catch (...) {
        throw ClientAPIException("Unknown client error detected");
    }
}

} // namespace

ClientAPI::ClientAPI() : invoker_(std::make_unique<ClientInvoker>()) {
}

ClientAPI::~ClientAPI() = default;

void ClientAPI::set_authentication(const std::string& username, const std::string& password) {
    invoker_->set_user_name(username);
    invoker_->set_password(password);
}

void ClientAPI::update_meter(const std::string& path, const std::string& name, const std::string& value) const {
    update_attribute(*invoker_, path, "meter", name, value);
}

void ClientAPI::update_label(const std::string& path, const std::string& name, const std::string& value) const {
    update_attribute(*invoker_, path, "label", name, value);
}

void ClientAPI::clear_event(const std::string& path, const std::string& name) const {
    update_attribute(*invoker_, path, "event", name, "clear");
}
void ClientAPI::set_event(const std::string& path, const std::string& name) const {
    update_attribute(*invoker_, path, "event", name, "set");
}

} // namespace ecf
