/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_Permissions_HPP
#define ecflow_node_Permissions_HPP

#include <algorithm>
#include <string>
#include <vector>

class Node;
class Variable;

namespace ecf {

class Permissions {
public:
    static Permissions make_empty() { return Permissions(); }
    static Permissions make_from_variable(const std::string& var_value);

    static Permissions find_in(const std::vector<Variable>& variables);

    [[nodiscard]] bool is_empty() const { return allowed_.empty(); }

    [[nodiscard]] bool allows(const std::string& user) const {
        auto found =
            std::find_if(std::begin(allowed_), std::end(allowed_), [&user](auto&& current) { return user == current; });
        return found != std::end(allowed_);
    }

private:
    Permissions() : allowed_{} {}
    explicit Permissions(std::vector<std::string> allowed) : allowed_{std::move(allowed)} {}

    std::vector<std::string> allowed_;
};

} // namespace ecf

#endif /* ecflow_node_Permissions_HPP */
