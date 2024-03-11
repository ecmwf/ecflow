/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_formatter_Formatter_HPP
#define ecflow_node_formatter_Formatter_HPP

#include <string>
#include <vector>

namespace ecf {
namespace implementation {

template <typename T, typename Stream>
struct Formatter
{
};

namespace {

/// Some utilities to be used by the formatters

template <typename T, typename Stream>
static void format_vector_as_defs(const std::vector<T>& items, Stream& output) {
    for (const auto& item : items) {
        Formatter<T, Stream>::format(item, output);
    }
}

template <typename T, typename Stream>
static void format_vector_as_defs(const std::vector<std::shared_ptr<T>>& items, Stream& output) {
    for (const auto& item : items) {
        if (item) {
            Formatter<T, Stream>::format(*item, output);
        }
    }
}

} // namespace

} // namespace implementation
} // namespace ecf

#endif /* ecflow_node_formatter_Formatter_HPP */
