/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
    // Disable copy (and move) semantics
    Attr(const Attr&)                  = delete;
    const Attr& operator=(const Attr&) = delete;

    static const char* to_string(Attr::Type s);
    static Attr::Type to_attr(const std::string& attr);
    static bool is_valid(const std::string& state);
    static std::vector<std::string> all_attrs();
    static std::vector<Attr::Type> attrs();
};

} // namespace ecf

#endif /* ecflow_node_Attr_HPP */
