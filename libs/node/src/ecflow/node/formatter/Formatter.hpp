/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
        output << "\n"; // Add a newline after each item
    }
}

template <typename T, typename Stream>
static void format_vector_as_defs(const std::vector<std::shared_ptr<T>>& items, Stream& output) {
    for (const auto& item : items) {
        if (item) {
            Formatter<T, Stream>::format(*item, output);
            output << "\n"; // Add a newline after each item
        }
    }
}

} // namespace

} // namespace implementation
} // namespace ecf

#endif /* ecflow_node_formatter_Formatter_HPP */
