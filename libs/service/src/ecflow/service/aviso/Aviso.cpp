/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/service/aviso/Aviso.hpp"

#include <cassert>
#include <fstream>
#include <httplib.h>
#include <iostream>
#include <memory>
#include <regex>

#include <nlohmann/json.hpp>

#include "ecflow/core/Overload.hpp"
#include "ecflow/service/Log.hpp"

namespace ecf::service::aviso {

/* AvisoSubscribe */

std::ostream& operator<<(std::ostream& os, const AvisoSubscribe& request) {
    os << "AvisoSubscribe{";
    os << "path: " << request.path();
    os << ", listener_cfg: " << request.listener_cfg();
    os << ", address: " << request.address();
    os << ", schema: " << request.schema();
    os << ", polling: " << request.polling();
    os << ", revision: " << request.revision();
    os << "}";
    return os;
}

/* AvisoUnsubscribe */

std::ostream& operator<<(std::ostream& os, const AvisoUnsubscribe& request) {
    os << "AvisoUnsubscribe{";
    os << "path: " << request.path();
    os << "}";
    return os;
}

/* AvisoRequest */

std::ostream& operator<<(std::ostream& os, const AvisoRequest& request) {
    os << "AvisoRequest{";
    std::visit(
        ecf::overload{[&os](const AvisoSubscribe& r) { os << r; }, [&os](const AvisoUnsubscribe& r) { os << r; }},
        request);
    os << "}";
    return os;
}

/* AvisoNotification */

std::ostream& operator<<(std::ostream& os, const AvisoNotification& notification) {
    os << "AvisoNotification{";
    os << "key: " << notification.key();
    os << ", value: " << notification.value();
    os << ", revision: " << notification.revision();
    os << "}";
    return os;
}

/* AvisoNoMatch */

std::ostream& operator<<(std::ostream& os, const AvisoNoMatch& no_match) {
    os << "AvisoNoMatch{}";
    return os;
}

/* AvisoError */

std::ostream& operator<<(std::ostream& os, const AvisoError& error) {
    os << "AvisoError{";
    os << "reason: " << error.reason();
    os << "}";
    return os;
}

/* Listener */

// N/A

/* ConfiguredListener */

ConfiguredListener ConfiguredListener::make_configured_listener(const AvisoSubscribe& listen_request) {

    std::ifstream schema_stream(listen_request.schema());

    return make_configured_listener(listen_request.path(),
                                    listen_request.listener_cfg(),
                                    listen_request.address(),
                                    schema_stream,
                                    listen_request.polling(),
                                    listen_request.revision());
}

ConfiguredListener ConfiguredListener::make_configured_listener(const std::string& path,
                                                                const std::string& listener_cfg,
                                                                const std::string& address,
                                                                const std::string& schema_content,
                                                                uint32_t polling,
                                                                uint64_t revision) {

    std::istringstream schema_stream(schema_content);

    return make_configured_listener(path, listener_cfg, address, schema_stream, polling, revision);
}

ConfiguredListener ConfiguredListener::make_configured_listener(const std::string& path,
                                                                const std::string& listener_cfg,
                                                                const std::string& address,
                                                                std::istream& schema_stream,
                                                                uint32_t polling,
                                                                uint64_t revision) {
    using json = nlohmann::ordered_json;

    json data;
    try {
        data = json::parse(listener_cfg);
    }
    catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse listener configuration: " + std::string(e.what()) + " " +
                                 listener_cfg);
    }

    ListenerSchema schema;
    try {
        schema = ListenerSchema::load(schema_stream);
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to load listener schema: " + std::string(e.what()));
    }

    if (!data.contains("event")) {
        throw std::runtime_error("Listener configuration does not specify 'event'");
    }
    if (!data.contains("request")) {
        throw std::runtime_error("Listener configuration does not specify 'request'");
    }

    std::string event    = data["event"];
    const auto& listener = schema.get_listener(event);
    if (!listener) {
        throw std::runtime_error("Listener could not be found in schema");
    }

    ConfiguredListener configured{
        aviso::etcd::Address{address}, path, listener->name(), listener->base(), listener->stem(), polling, revision};

    SLOG(I,
         "Aviso: configured with: " << path << " for " << event << " at " << address << " with revision " << revision);

    auto request = data["request"];
    for (json::iterator entry = request.begin(); entry != request.end(); ++entry) {

        auto value = entry.value();
        if (value.is_string()) {
            configured.with_parameter(entry.key(), value.template get<std::string>());
        }
        else if (value.is_number_integer()) {
            configured.with_parameter(entry.key(), std::to_string(value.template get<int64_t>()));
        }
        else if (value.is_array()) {
            std::vector<std::string> values_as_atrings;
            for (const auto& v : value) {
                if (v.is_string()) {
                    values_as_atrings.push_back(v.template get<std::string>());
                }
                else if (v.is_number_integer()) {
                    values_as_atrings.push_back(std::to_string(v.template get<int64_t>()));
                }
                else {
                    std::terminate(); // Unsupported type!
                }
            }

            configured.with_parameter(entry.key(), values_as_atrings);
        }
    }

    return configured;
}

ConfiguredListener::ConfiguredListener(ecf::service::aviso::etcd::Address address,
                                       std::string_view path,
                                       std::string_view name,
                                       std::string_view base,
                                       std::string_view stem,
                                       uint32_t polling,
                                       uint64_t revision)
    : Listener(name, base, stem),
      path_{path},
      address_(std::move(address)),
      resolved_base_(base),
      polling_{polling},
      revision_{revision} {
}

void ConfiguredListener::with_parameter(const std::string& parameter, const std::string& value) {
    const std::string LBRACE  = R"(\{)";
    const std::string RBRACE  = R"(\})";
    const std::string pattern = LBRACE + parameter + RBRACE;

    const auto re = std::regex(pattern);

    resolved_base_         = std::regex_replace(resolved_base_, re, value);
    parameters_[parameter] = value;
}

void ConfiguredListener::with_parameter(const std::string& parameter, int64_t value) {
    with_parameter(parameter, std::to_string(value));
}

void ConfiguredListener::with_parameter(const std::string& parameter, const std::vector<std::string>& values) {
    parameters_[parameter] = values;
}

std::optional<AvisoNotification>
ConfiguredListener::accepts(const std::string& key, const std::string& value, uint64_t revision) const {
    // 1. turn `full` key into regex
    auto original_full = full();

    // A. Find placeholders
    // TODO can be done only once
    std::vector<std::string> placeholders;
    {
        std::regex re(R"(\{([^}]+)\})");

        auto m_bgn = std::sregex_iterator(std::begin(original_full), std::end(original_full), re);
        auto m_end = std::sregex_iterator();

        for (auto i = m_bgn; i != m_end; ++i) {
            std::smatch m = *i;
            placeholders.push_back(m[1]);
        }
    }

    // B. Create matching regex, by replacing placeholders
    // TODO can be done only once
    auto replaced_full = full();

    for (const auto& placeholder : placeholders) {
        const std::string LBRACE  = R"(\{)";
        const std::string RBRACE  = R"(\})";
        const std::string pattern = LBRACE + placeholder + RBRACE;

        const auto re = std::regex(pattern);

        const std::string value = R"(([\d\w]*))";

        replaced_full = std::regex_replace(replaced_full, re, value);
    }

    // C. Extract result from match
    auto actual_parameters = std::vector<std::pair<std::string, std::string>>{};

    {
        const auto re = std::regex(replaced_full);

        auto m_bgn = std::sregex_iterator(std::begin(key), std::end(key), re);
        auto m_end = std::sregex_iterator();

        for (auto i = m_bgn; i != m_end; ++i) {
            std::smatch m = *i;
            // SLOG(D, "Extracted parameters:" << m.str());
            for (size_t i = 1; i != m.size(); ++i) {
                actual_parameters.emplace_back(placeholders[i - 1], m[i]);
            }
        }
    }

    if (placeholders.size() != actual_parameters.size()) {
        SLOG(D, "Aviso: Match [✗] --> <Notification> " << key << " = " << value << " (revision: " << revision << ")");
        return std::nullopt;
    }

    // D. Check parameters
    bool applicable = true;
    {
        for (const auto& [k, v] : actual_parameters) {
            // SLOG(D, "-> " << k << " = " << v);
            if (auto found = parameters_.find(k); found != std::end(parameters_)) {

                auto actual = v;

                auto validate =
                    ecf::overload{[&actual](const std::string& expected) { return expected == actual; },
                                  [&actual](std::int64_t expected) { return std::to_string(expected) == actual; },
                                  [&actual](const std::vector<std::string>& valid) {
                                      return std::any_of(valid.begin(), valid.end(), [&actual](const auto& expected) {
                                          return expected == actual;
                                      });
                                  }};

                applicable = std::visit(validate, found->second);
                if (!applicable) {
                    break;
                }
            }
        }
    }

    if (applicable) {
        AvisoNotification notification{key, value, revision};
        for (const auto& [k, v] : actual_parameters) {
            notification.add_parameter(k, v);
        }
        SLOG(D, "Aviso: Match [✓] --> <Notification> " << key << " = " << value << " (revision: " << revision << ")");
        return notification;
    }
    else {
        SLOG(D, "Aviso: Match [✗] --> <Notification> " << key << " = " << value << " (revision: " << revision << ")");
        return std::nullopt;
    }
}

std::ostream& operator<<(std::ostream& os, const ConfiguredListener& listener) {
    os << "ConfiguredListener{";
    os << "name: " << listener.name();
    os << ", full: " << listener.full();
    os << ", path: " << listener.path();
    os << ", address: " << listener.address().address();
    os << ", polling: " << listener.polling();
    os << ", revision: " << listener.revision() << "}";
    return os;
}

/* ListenerSchema */

std::optional<Listener> ListenerSchema::get_listener(const std::string& name) const {
    if (auto found = listeners_.find(name); found != listeners_.end()) {
        return found->second;
    }
    return std::nullopt;
}

ListenerSchema ListenerSchema::load_from_string(const std::string& schema_content) {
    std::istringstream schema_stream(schema_content);
    return load(schema_stream);
}

ListenerSchema ListenerSchema::load(const std::string& schema_path) {
    std::ifstream schema_stream(schema_path);
    return load(schema_stream);
}

ListenerSchema ListenerSchema::load(std::istream& schema_stream) {
    using json = nlohmann::ordered_json;

    json data;
    try {
        data = json::parse(schema_stream);
    }
    catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse listener schema: " + std::string(e.what()));
    }

    ListenerSchema schema{};

    for (json::iterator entry = data.begin(); entry != data.end(); ++entry) {
        if (auto& value = entry.value(); value.is_object() && value.contains("endpoint")) {
            for (const auto& endpoint : value["endpoint"]) {
                for (const auto& engine : endpoint["engine"]) {
                    if (engine == "etcd_rest") {
                        std::string name = entry.key();
                        std::string base = endpoint["base"];
                        std::string stem = endpoint["stem"];
                        schema.add_listener(Listener(name, base, stem));
                    }
                }
            }
        }
    }

    return schema;
}

void ListenerSchema::add_listener(const Listener& listener) {
    auto name        = std::string(listener.name());
    listeners_[name] = listener;
}

} // namespace ecf::service::aviso
