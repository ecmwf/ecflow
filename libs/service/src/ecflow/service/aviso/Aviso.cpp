// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/service/aviso/Aviso.hpp"

#include <algorithm>
#include <ostream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "ecflow/core/Overload.hpp"
#include "ecflow/service/auth/Credentials.hpp"

namespace ecf::service::aviso {

std::ostream& operator<<(std::ostream& os, const AvisoSubscribe& request) {
    os << "AvisoSubscribe{";
    os << "path: " << request.path();
    os << ", listener: " << request.listener();
    os << ", url: " << request.url();
    os << ", revision: " << request.revision();
    os << ", auth: " << request.auth();
    os << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const AvisoNotification& notification) {
    os << "AvisoNotification{";
    os << "event_type: " << notification.event_type();
    os << ", sequence: " << notification.sequence();
    os << ", identifier: " << notification.identifier_json();
    os << ", payload: " << notification.payload_json();
    os << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const AvisoError& error) {
    os << "AvisoError{";
    os << "reason: " << error.reason();
    os << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const AvisoWatchStarted&) {
    os << "AvisoWatchStarted{}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const AvisoResponse& response) {
    std::visit([&os](const auto& r) { os << r; }, response);
    return os;
}

Listener parse_listener(const std::string& listener) {
    using json = nlohmann::ordered_json;

    json content;
    try {
        content = json::parse(listener);
    }
    catch (const json::parse_error& e) {
        throw std::runtime_error(std::string("Aviso listener is not valid JSON: ") + e.what());
    }

    if (!content.is_object()) {
        throw std::runtime_error("Aviso listener must be a JSON object");
    }

    auto event = content.find("event");
    if (event == content.end() || !event->is_string() || event->get<std::string>().empty()) {
        throw std::runtime_error("Aviso listener must define the event, as a non-empty string");
    }

    Listener result{event->get<std::string>(), ""};

    if (auto request = content.find("request"); request != content.end()) {
        if (!request->is_object()) {
            throw std::runtime_error("Aviso listener request must be a JSON object");
        }
        json filter = json::object();
        for (const auto& [name, value] : request->items()) {
            if (value.is_array()) {
                filter[name] = json{{"in", value}};
            }
            else {
                filter[name] = value;
            }
        }
        if (!filter.empty()) {
            result.filter_json = filter.dump();
        }
    }

    return result;
}

Auth load_auth(const std::string& path) {
    if (path.empty()) {
        throw std::runtime_error("no Aviso credentials file given (see option --auth, or variable ECF_AVISO_AUTH)");
    }

    auto loaded = ecf::service::auth::Credentials::load(path);

    return std::visit(ecf::overload{[](const ecf::service::auth::Credentials& credentials) -> Auth {
                                        // The key of an ECMWF API credentials file is the bearer token; otherwise,
                                        // the credentials necessarily hold a user (see Credentials::load)
                                        if (auto key = credentials.key(); key) {
                                            return BearerAuth{key->key};
                                        }
                                        auto user = credentials.user().value();
                                        return BasicAuth{user.username, user.password};
                                    },
                                    [&path](const ecf::service::auth::Credentials::Error& error) -> Auth {
                                        throw std::runtime_error("Aviso credentials file " + path + ": " +
                                                                 error.message);
                                    }},
                      loaded);
}

std::string describe_error(std::string_view kind,
                           std::uint16_t http_status,
                           std::string_view message,
                           const std::optional<std::string>& request_id) {
    std::string description = "Aviso error (";
    description += kind;
    if (http_status != 0) {
        description += ", HTTP " + std::to_string(http_status);
    }
    description += "): ";
    description += message;
    if (request_id) {
        description += " [request " + *request_id + "]";
    }
    description += "; ";
    description += unsupported_v1;

    std::replace(description.begin(), description.end(), '\'', '`');
    std::replace(description.begin(), description.end(), '\n', ' ');
    std::replace(description.begin(), description.end(), '\r', ' ');
    return description;
}

} // namespace ecf::service::aviso
