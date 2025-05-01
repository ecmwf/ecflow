/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_ServerProtocol_HPP
#define ecflow_base_ServerProtocol_HPP

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Enumerate.hpp"

namespace ecf {

enum class Protocol {
    Plain = 0, // custom TCP/IP protocol
    Ssl   = 1, // custom TCP/IP protocol with SSL encryption
    Http  = 2, // HTTP protocol
    Https = 3  // HTTP protocol with SSL encryption
};

namespace detail {

template <>
struct EnumTraits<Protocol>
{
    static constexpr std::array<std::pair<Protocol, const char*>, 4> map = {{std::make_pair(Protocol::Plain, "PLAIN"),
                                                                             std::make_pair(Protocol::Ssl, "SSL"),
                                                                             std::make_pair(Protocol::Http, "HTTP"),
                                                                             std::make_pair(Protocol::Https, "HTTPS")}};

    static constexpr std::array<std::pair<Protocol, const char*>, 4> ui_designation = {
        {std::make_pair(Protocol::Plain, "TCT/IP"),
         std::make_pair(Protocol::Ssl, "TCT/IP with SSL"),
         std::make_pair(Protocol::Http, "HTTP"),
         std::make_pair(Protocol::Https, "HTTPS")}};

    static constexpr size_t size = map.size();
    static_assert(size == 4, "Protocol enum size mismatch");
    static_assert(size == ui_designation.size(), "Protocol/designation enum size mismatch");
};

} // namespace detail

template <typename F>
void for_each_protocol_ui_designation(F&& f) {
    for (const auto& item : ecf::detail::EnumTraits<Protocol>::ui_designation) {
        f(item.first, item.second);
    }
}

static inline const char* to_ui_designation(Protocol protocol) {
    for (const auto& item : ecf::detail::EnumTraits<Protocol>::ui_designation) {
        if (item.first == protocol) {
            return item.second;
        }
    }
    throw std::runtime_error("Invalid protocol");
}

static inline std::string to_configuration_encoding(Protocol protocol) {
    return ecf::convert_to<std::string>(static_cast<std::underlying_type_t<Protocol>>(protocol));
}

static inline Protocol from_configuration_encoding(const std::string& encoding) {
    for (const auto& item : ecf::detail::EnumTraits<Protocol>::map) {
        auto encoded = ecf::convert_to<std::string>(static_cast<std::underlying_type_t<Protocol>>(item.first));
        if (encoded == encoding) {
            return item.first;
        }
    }
    throw std::runtime_error("Invalid encoding");
}

static inline std::string scheme_for(Protocol protocol) {
    switch (protocol) {
        case Protocol::Http:
            return "http";
        case Protocol::Https:
            return "https";
        case Protocol::Plain:
        case Protocol::Ssl:
        default:
            return "";
    }
}

static inline bool is_any_variation_of_http(Protocol protocol) {
    return protocol == Protocol::Http || protocol == Protocol::Https;
}

} // namespace ecf

#endif /* ecflow_base_ServerProtocol_HPP */
