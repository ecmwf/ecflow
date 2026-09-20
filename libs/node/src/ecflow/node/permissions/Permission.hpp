// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#ifndef ecflow_node_permissions_Permission_HPP
#define ecflow_node_permissions_Permission_HPP

#include "ecflow/core/Identity.hpp"
#include "ecflow/node/permissions/Allowed.hpp"

namespace ecf {

/**
 * \brief Represents a permission for a specific user by linking a 'username' with a set of allowed permissions.
 */
class Permission {
public:
    Permission(Username user, Allowed allowed)
        : username_{std::move(user)},
          allowed_{allowed} {}

    [[nodiscard]] bool allows(const Username& user, Allowed requested) const {
        return user == username_ && (contains(allowed_, requested));
    }

    Username username() const { return username_; }
    Allowed allowed() const { return allowed_; }

private:
    Username username_;
    Allowed allowed_;
};

} // namespace ecf

#endif /* ecflow_node_permissions_Permission_HPP */
