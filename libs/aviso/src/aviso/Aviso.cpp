/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "aviso/Aviso.hpp"

#include <cassert>
#include <fstream>
#include <httplib.h>
#include <iostream>
#include <memory>
#include <regex>

#include <boost/algorithm/string.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <nlohmann/json.hpp>

namespace aviso {

template <typename... Ts>
struct Overload : Ts...
{
    using Ts::operator()...;
};

template <class... Ts>
Overload(Ts...) -> Overload<Ts...>; // required in C++17, no longer required in C++20

std::string Content::decode_base64(const std::string& val) {
    using namespace boost::archive::iterators;
    using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
    return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(val)), It(std::end(val))),
                                                [](char c) { return c == '\0'; });
}

std::string Content::decode_base64(std::string_view val) {
    std::string v{val};
    return decode_base64(v);
}

std::string Content::encode_base64(const std::string& val) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

std::string Content::encode_base64(std::string_view val) {
    std::string v{val};
    return encode_base64(v);
}

Range::key_t Range::increment_last_byte(key_t val) {
    assert(!val.empty());
    val[val.size() - 1]++;
    return val;
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

ListenerSchema load_listener_schema(const std::string& schema_location) {
    using json = nlohmann::ordered_json;

    std::ifstream is(schema_location);
    json data = json::parse(is);

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

ConfiguredListener create_configured_listener(const std::string& cfg, const ListenerSchema& schema) {
    using json = nlohmann::ordered_json;

    json data = json::parse(cfg);

    std::string address = data["address"];
    std::string path    = data["path"];
    std::string event   = data["event"];

    const auto& listener = schema.get_listener(event);
    if (!listener) {
        throw std::runtime_error("Listener not found");
    }

    ConfiguredListener configured{Address{address}, path, listener->name(), listener->base(), listener->stem()};

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

std::ostream& operator<<(std::ostream& os, const Notification& notification) {
    os << "<Notification> " << std::endl;
    os << "        key->> " << notification.key() << std::endl;
    os << "      value->> " << notification.value() << std::endl;
    for (const auto& [k, v] : notification.parameters()) {
        os << " parameters->> " << k << " = " << v << std::endl;
    }
    return os;
}

struct EtcdClient::Impl
{
    explicit Impl(Address address) : address_(std::move(address)), client_(std::string{address_.address()}) {}

    Address address_;
    httplib::Client client_;
};

EtcdClient::EtcdClient(const Address& address) : impl_(std::make_unique<EtcdClient::Impl>(address)) {
}

EtcdClient::~EtcdClient() = default;

std::vector<std::pair<std::string, std::string>> EtcdClient::poll(std::string_view key_prefix, int64_t revision) {
    using json = nlohmann::ordered_json;

    std::cout << "Polling key prefix: " << key_prefix;
    std::cout << ", from revision: " << revision;
    std::cout << " @ address : " << impl_->address_.address() << std::endl;

    httplib::Headers headers;

    auto range = aviso::Range(key_prefix);

    std::string request_body =
        json::object({{"key", range.base64_begin()}, {"range_end", range.base64_end()}, {"min_mod_revision", revision}})
            .dump();
    std::string content_type = "application/json";

    // std::cout << "Sending: " << request_body << std::endl;

    httplib::Result result = impl_->client_.Post(endpoint_path, headers, request_body, content_type);
    if (!result) {
        std::cout << "Error: " << result.error() << std::endl << std::endl;
        return std::vector<std::pair<std::string, std::string>>{};
    }
    std::cout << "Received: " << result.value().status << " --> " << result.value().reason << std::endl;

    if (result.value().status != 200) {
        // TODO: handle the error (either by throwing an exception or returning an optional)
        std::cout << "Something when wrong!" << std::endl;
        return std::vector<std::pair<std::string, std::string>>{};
    }

    auto response_body = json::parse(std::begin(result.value().body), std::end(result.value().body));
    // std::cout << "Received: " << response_body.dump(2) << std::endl;

    std::vector<std::pair<std::string, std::string>> entries;

    if (response_body.contains("header")) {
        auto latest_revision = response_body["header"]["revision"];
        entries.emplace_back("latest_revision", latest_revision);
    }

    if (response_body.contains("kvs")) {
        for (const auto& kv : response_body["kvs"]) {
            auto k     = kv["key"];
            auto key   = aviso::make_from<aviso::Base64>(k);
            auto v     = kv["value"];
            auto value = aviso::make_from<aviso::Base64>(v);

            if (key.raw() != key_prefix) {
                // std::cout << "Received key+value: " << key.raw() << "+" << value.raw() << std::endl;
                entries.emplace_back(key.raw(), value.raw());
            }
        }
    }
    else {
        std::cout << "No new key+value" << std::endl << std::endl;
    }

    return entries;
}

} // namespace aviso
