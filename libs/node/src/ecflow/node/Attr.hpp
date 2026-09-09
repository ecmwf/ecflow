/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_Attr_HPP
#define ecflow_node_Attr_HPP

#include <string>
#include <vector>

namespace ecf {
class Attr {
public:
    enum Type { UNKNOWN = 0, EVENT = 1, METER = 2, LABEL = 3, LIMIT = 4, VARIABLE = 5, ALL = 6 };

    // Disable default construction
    Attr() = delete;

    static const char* to_string(Attr::Type s);
    static Attr::Type to_attr(const std::string& attr);
    static bool is_valid(const std::string& state);
    static std::vector<std::string> all_attrs();
    static std::vector<Attr::Type> attrs();
};

} // namespace ecf

#endif /* ecflow_node_Attr_HPP */
