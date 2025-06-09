/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

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
    Permission(Username user, Allowed allowed) : username_{std::move(user)}, allowed_{allowed} {}

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
