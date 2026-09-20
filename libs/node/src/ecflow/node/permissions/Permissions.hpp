// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#ifndef ecflow_node_permissions_Permissions_HPP
#define ecflow_node_permissions_Permissions_HPP

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "ecflow/core/Identity.hpp"
#include "ecflow/core/Result.hpp"
#include "ecflow/node/permissions/Permission.hpp"

class Variable;

namespace ecf {

/**
 * \brief Represents a collection of permissions for multiple users.
 *
 * This class allows checking if a specific user has a certain permission and provides methods to combine
 * permissions from different sources.
 */
class Permissions {
public:
    static Permissions make_empty() { return Permissions(); }
    static Result<Permissions> make_from_variable(const std::string& var_value);

    static Permissions find_in(const std::vector<Variable>& variables);

    [[nodiscard]] bool is_empty() const { return allowed_.empty(); }

    [[nodiscard]] bool allows(const Username& username, Allowed permission) const {
        auto found = std::find_if(std::begin(allowed_), std::end(allowed_), [&](auto&& current) {
            return current.allows(username, permission);
        });
        return found != std::end(allowed_);
    }

    static Permissions combine_supersede(const Permissions& active, const Permissions& current);
    static Permissions combine_override(const Permissions& active, const Permissions& current);

    friend std::ostream& operator<<(std::ostream& os, const Permissions& p) {
        using namespace std::string_literals;
        os << "Permissions: "s;
        for (auto&& permission : p.allowed_) {
            os << "("s << permission.username().value() << ":"s << allowed_to_string(permission.allowed()) << ") "s;
        }
        return os;
    }

private:
    Permissions()
        : allowed_{} {}
    explicit Permissions(std::vector<Permission> allowed)
        : allowed_{std::move(allowed)} {}

    std::vector<Permission> allowed_;
};

} // namespace ecf

#endif /* ecflow_node_permissions_Permissions_HPP */
