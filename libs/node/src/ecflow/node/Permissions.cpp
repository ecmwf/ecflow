/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/Permissions.hpp"

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/core/Str.hpp"

namespace ecf {

Permissions Permissions::make_from_variable(const std::string& value) {

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
        auto perms    = Allowed::NONE;
        for (auto&& c : user_permissions[1]) {
            switch (c) {
                case 'r':
                    [[fallthrough]];
                case 'R':
                    perms |= Allowed::READ;
                    break;
                case 'w':
                    [[fallthrough]];
                case 'W':
                    perms |= Allowed::WRITE;
                    break;
                case 'x':
                    [[fallthrough]];
                case 'X':
                    perms |= Allowed::EXECUTE;
                    break;
                case 'o':
                    [[fallthrough]];
                case 'O':
                    perms |= Allowed::OWNER;
                    break;
                default:
                    throw std::runtime_error("Invalid permission character: " + std::string(1, c) +
                                             ". Expected one of: [r, w, x, o]");
            }
        }
        allowed.emplace_back(username, perms);
    }

    return Permissions(std::move(allowed));
}

Permissions Permissions::find_in(const std::vector<Variable>& variables) {
    static std::string permissions_var_name = "PERMISSIONS";
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

} // namespace ecf
