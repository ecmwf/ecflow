/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "aviso/ConfiguredListener.hpp"

#include <cassert>
#include <fstream>
#include <httplib.h>
#include <iostream>
#include <memory>
#include <regex>

#include <nlohmann/json.hpp>

namespace aviso {

std::ostream& operator<<(std::ostream& os, const Notification& notification) {
    os << "<Notification> " << std::endl;
    os << "        key->> " << notification.key() << std::endl;
    os << "      value->> " << notification.value() << std::endl;
    for (const auto& [k, v] : notification.parameters()) {
        os << " parameters->> " << k << " = " << v << std::endl;
    }
    return os;
}

template <typename... Ts>
struct Overload : Ts...
{
    using Ts::operator()...;
};

template <class... Ts>
Overload(Ts...) -> Overload<Ts...>; // required in C++17, no longer required in C++20

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

std::optional<Notification> ConfiguredListener::accepts(const std::string& key, const std::string& value) const {

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
            // std::cout << "Listened to:" << m.str() << std::endl;
            for (size_t i = 1; i != m.size(); ++i) {
                actual_parameters.emplace_back(placeholders[i - 1], m[i]);
            }
        }
    }

    // D. Check parameters
    bool applicable = true;
    {
        for (const auto& [k, v] : actual_parameters) {
            //            std::cout << "-> " << k << " = " << v << std::endl;
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

    std::cout << "<Notification> " << key << std::endl;
    if (applicable) {
        Notification notification{key, value};
        for (const auto& [k, v] : actual_parameters) {
            notification.add_parameter(k, v);
        }
        std::cout << "Match: ✓" << std::endl << std::endl;
        return notification;
    }
    else {
        std::cout << "Match: ✗" << std::endl << std::endl;
        return std::nullopt;
    }
}

ConfiguredListener create_configured_listener(const ListenRequest& listen_request, const ListenerSchema& schema) {
    using json = nlohmann::ordered_json;

    json data = json::parse(listen_request.listener_cfg());

    std::string address = listen_request.address();
    std::string path    = listen_request.path();
    std::string event   = data["event"];

    const auto& listener = schema.get_listener(event);
    if (!listener) {
        throw std::runtime_error("Listener not found");
    }

    ConfiguredListener configured{etcd::Address{address}, path, listener->name(), listener->base(), listener->stem()};

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

} // namespace aviso
