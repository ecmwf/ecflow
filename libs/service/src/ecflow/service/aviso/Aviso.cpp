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

#include "ecflow/service/Log.hpp"

namespace ecf::service::aviso {

/* std::variant visitor utility */

template <typename... Ts>
struct Overload : Ts...
{
    using Ts::operator()...;
};

template <class... Ts>
Overload(Ts...) -> Overload<Ts...>; // required in C++17, no longer required in C++20

/* AvisoRequest */

std::ostream& operator<<(std::ostream& os, const AvisoRequest& request) {
    os << "ListenRequest{";
    os << "start: " << request.is_start();
    os << ", path: " << request.path();
    if (request.is_start()) {
        os << ", listener_cfg: " << request.listener_cfg();
        os << ", address: " << request.address();
        os << ", schema: " << request.schema();
        os << ", polling: " << request.polling();
        os << ", revision: " << request.revision();
    }
    os << "}";
    return os;
}

/* Notification */

std::ostream& operator<<(std::ostream& os, const AvisoNotification& notification) {
    os << notification.key() << " = " << notification.value() << " (revision: " << notification.revision() << ")";
    return os;
}

/* Listener */

// N/A

/* ConfiguredListener */

ConfiguredListener ConfiguredListener::make_configured_listener(const AvisoRequest& listen_request) {
    using json = nlohmann::ordered_json;

    json data;
    try {
        data = json::parse(listen_request.listener_cfg());
    }
    catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse listener configuration: " + std::string(e.what()) + " " +
                                 listen_request.listener_cfg());
    }

    ListenerSchema schema;
    try {
        schema = ListenerSchema::load(listen_request.schema());
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to load listener schema: " + std::string(e.what()));
    }

    std::string address = listen_request.address();
    std::string path    = listen_request.path();
    std::string event   = data["event"];
    uint32_t polling    = listen_request.polling();
    uint64_t revision   = listen_request.revision();

    const auto& listener = schema.get_listener(event);
    if (!listener) {
        throw std::runtime_error("Listener not found");
    }

    ConfiguredListener configured{
        aviso::etcd::Address{address}, path, listener->name(), listener->base(), listener->stem(), polling, revision};

    ALOG(I,
         "Listener configured with: " << path << " for " << event << " at " << address << " with revision "
                                      << revision);

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
                    // Unsupported type!
                    std::terminate();
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
            // ALOG(D, "Extracted parameters:" << m.str());
            for (size_t i = 1; i != m.size(); ++i) {
                actual_parameters.emplace_back(placeholders[i - 1], m[i]);
            }
        }
    }

    // D. Check parameters
    bool applicable = true;
    {
        for (const auto& [k, v] : actual_parameters) {
            // ALOG(D, "-> " << k << " = " << v);
            if (auto found = parameters_.find(k); found != std::end(parameters_)) {

                auto actual = v;

                auto validate =
                    Overload{[&actual](const std::string& expected) { return expected == actual; },
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
        ALOG(D, "Match: ✓ --> <Notification> " << key << " = " << value << " (revision: " << revision << ")");
        return notification;
    }
    else {
        ALOG(D, "Match: ✗ --> <Notification> " << key << " = " << value << " (revision: " << revision << ")");
        return std::nullopt;
    }
    return std::nullopt;
}

/* ListenerSchema */

std::optional<Listener> ListenerSchema::get_listener(const std::string& name) const {
    if (auto found = listeners_.find(name); found != listeners_.end()) {
        return found->second;
    }
    return std::nullopt;
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
