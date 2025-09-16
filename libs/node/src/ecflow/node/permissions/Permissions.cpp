/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/permissions/Permissions.hpp"

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Str.hpp"

namespace ecf {

Permissions Permissions::make_from_variable(const std::string& value) {

    if (value.empty()) {
        return Permissions::make_empty();
    }

    // Expecting a comma-separated list of user/permissions, e.g. "USER1:RWXO,USER2:R"

    std::vector<std::string> entries;
    ecf::algorithm::split(entries, value, ",");

    std::vector<Permission> allowed;
    for (auto&& entry : entries) {
        std::vector<std::string> user_permissions;
        ecf::algorithm::split(user_permissions, entry, ":");
        if (user_permissions.size() != 2) {
            throw std::runtime_error("Invalid permission entry: " + entry + ". Expected format: <user>:<rwxo>");
        }

        auto username = Username{user_permissions[0]};
        auto perms    = allowed_from_string(user_permissions[1]);
        allowed.emplace_back(username, perms);
    }

    return Permissions(std::move(allowed));
}

Permissions Permissions::find_in(const std::vector<Variable>& variables) {
    static std::string permissions_var_name = ecf::environment::ECF_PERMISSIONS;
    if (auto found = std::find_if(
            std::begin(variables), std::end(variables), [](auto&& var) { return var.name() == permissions_var_name; });
        found != std::end(variables)) {
        auto var_value = found->theValue();
        return ecf::Permissions::make_from_variable(var_value);
    }
    else {
        return ecf::Permissions::make_empty();
    }
}

Permissions Permissions::combine_supersede(const Permissions& active, const Permissions& current) {
    // `current` is the new permissions
    // `previous` is the previous permissions
    //
    // The superseding rules are to update previous permissions, by keeping all sticky permissions and keeping any
    // non-sticky permissions with the new permissions.
    //

    std::vector<Permission> result;

    // First, we keep all sticky permissions from the active permissions
    for (auto&& permission : active.allowed_) {
        if (contains(permission.allowed(), Allowed::STICKY)) {
            result.push_back(permission);
        }
    }

    // Then, we retain the non-sticky permissions from the current permissions
    for (auto&& permission : current.allowed_) {
        auto found = std::find_if(
            std::begin(result), std::end(result), [&](auto&& p) { return p.username() == permission.username(); });

        if (found == std::end(result)) {
            // If the user is not in listed as sticky, we add the permission
            result.push_back(permission);
        }
    }

    return Permissions{std::move(result)};
}

Permissions Permissions::combine_override(const Permissions& active, const Permissions& current) {
    // `current` is the new permissions
    // `previous` is the previous permissions
    //
    // The overriding rules are to update previous permissions by restricting them based on the new permissions.
    // Sticky permissions are kept as is, while non-sticky permissions are overridden by the current permissions.
    //

    if (current.is_empty()) {
        // When there are no permissions to override (i.e. the current permissions are empty), the active permissions
        // are kept as is.
        return active;
    }

    std::vector<Permission> result;

    for (auto&& permission : active.allowed_) {
        if (contains(permission.allowed(), Allowed::STICKY)) {
            // If the permission is sticky, we keep the permissions as is
            result.push_back(permission);
        }
        else {
            if (auto found = std::find_if(std::begin(current.allowed_),
                                          std::end(current.allowed_),
                                          [&](auto&& p) { return p.username() == permission.username(); });
                found != std::end(current.allowed_)) {
                // When non-sticky permissions are overridden in current, we combine the permissions
                result.push_back(Permission{permission.username(), permission.allowed() & found->allowed()});
            }
        }
    }

    return Permissions{std::move(result)};
}

} // namespace ecf
