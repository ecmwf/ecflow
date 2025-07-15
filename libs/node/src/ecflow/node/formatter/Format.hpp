/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_formatter_Format_HPP
#define ecflow_node_formatter_Format_HPP

#include "ecflow/node/formatter/AvisoFormatter.hpp"
#include "ecflow/node/formatter/MirrorFormatter.hpp"

namespace ecf {

struct stringstreambuf
{
    std::string& buf;
};

template <typename S, typename = std::enable_if_t<std::is_convertible_v<S, std::string>>>
inline void operator<<(stringstreambuf& sb, S&& s) {
    sb.buf += std::forward<S>(s);
}

template <typename S, typename = std::enable_if_t<std::is_integral_v<S>>>
inline void operator<<(stringstreambuf& sb, S s) {
    sb.buf += std::to_string(s);
}

template <typename T>
void format_as_defs(const T& value, std::string& buffer) {
    buffer.reserve(1024 * 4);
    stringstreambuf output{buffer};
    implementation::Formatter<T, stringstreambuf>::format(value, output);
}

template <typename T, typename Stream>
void format_as_defs(const T& value, Stream& output) {
    implementation::Formatter<T, Stream>::format(value, output);
}

} // namespace ecf

#endif /* ecflow_node_formatter_Format_HPP */
