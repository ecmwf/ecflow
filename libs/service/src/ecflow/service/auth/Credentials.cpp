/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/service/auth/Credentials.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

#include "ecflow/core/Message.hpp"

namespace ecf::service::auth {

void Credentials::add(std::string key, std::string value) {
    entries_.push_back({std::move(key), std::move(value)});
}

std::optional<std::string> Credentials::value(std::string_view key) const {
    for (const auto& entry : entries_) {
        if (entry.key == key) {
            return entry.value;
        }
    }
    return std::nullopt;
}

std::optional<Credentials::UserCredentials> Credentials::user() const {
    if (auto username = value("username"); username) {
        if (auto password = value("password"); password) {
            return UserCredentials{std::move(*username), std::move(*password)};
        }
    }
    return std::nullopt;
}

std::optional<Credentials::KeyCredentials> Credentials::key() const {
    if (auto key = value("key"); key) {
        if (auto email = value("email"); email) {
            return KeyCredentials{std::move(*email), std::move(*key)};
        }
    }
    return std::nullopt;
}

namespace {

Credentials load_from_stream(std::istream& input) {
    using json = nlohmann::ordered_json;

    json content;
    try {
        content = json::parse(input);
    }
    catch (const json::parse_error& e) {
        throw std::runtime_error(Message("Credentials: Unable to parse content, due to ", e.what()).str());
    }

    Credentials credentials;
    for (auto field : content.items()) {
        try {
            credentials.add(field.key(), field.value());
        }
        catch (const json::type_error& e) {
            throw std::runtime_error(Message("Credentials: Unable to retrieve content, due to ", e.what()).str());
        }
    }

    if (!credentials.user() && !credentials.key()) {
        throw std::runtime_error("Credentials: Invalid content found (neither user nor key credentials provided)");
    }

    return credentials;
}

} // namespace

Credentials Credentials::load(const std::string& filepath) {
    std::ifstream stream(filepath);
    return load_from_stream(stream);
}

Credentials Credentials::load_content(const std::string& content) {
    std::istringstream stream(content);
    return load_from_stream(stream);
}

} // namespace ecf::service::auth
