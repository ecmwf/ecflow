/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_permissions_ActivePermissions_HPP
#define ecflow_node_permissions_ActivePermissions_HPP

#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "ecflow/node/permissions/Permissions.hpp"

class Defs;

namespace ecf {

/**
 * \brief Represents the active permissions, encapsulating the logic for calculating the user permissions as they
 * are defined in the server's hierarchy.
 */
class ActivePermissions {
public:
    static ActivePermissions make_empty() { return ActivePermissions(); }

    [[nodiscard]] bool is_none() const { return permissions_.is_empty(); }

    [[nodiscard]] bool allows(const Username& username, Allowed permission) const {
        return is_none() || permissions_.allows(username, permission);
        // The above means that when there are no active rules, everything is allowed!
        // Only when there are active rules, the user permissions are effectively checked.
        //
        // This design choice is considered dangerous, as a misconfigured server becomes essentially open!
        //   But this is backward-compatible!
    }

    /**
     * \brief Bootstrap the active permissions with the given permissions.
     * This is typically used to set the initial permissions at server level and then begin the permission
     * selection algorithm.
     *
     * @param p the permissions to start selected algorithm
     */
    void bootstrap(const Permissions& p);

    /**
     * \brief Combine the current active permissions with the new permissions using the _supersede_ logic.
     *
     * The _supersede_ logic is applied at root node level (i.e. Suite), and means that the new permissions
     * replace active permissions. This is needed to accommodate the user permission changes done at suite level,
     * and allow new users being added where before there were no permissions defined.
     *
     * All sticky permissions are preserved.
     *
     * @param p the permissions to consider
     */
    void combine_supersede(const Permissions& p);

    /**
     * \brief Combine the current active permissions with the new permissions using the _override_ logic.
     *
     * The _override_ logic is applied at non-root node level (i.e. Family, Task, etc.), and means that the new
     * permissions are combined with active permissions applying a restrictive logic.
     *
     * All sticky permissions are preserved.
     *
     * @param p
     */
    void combine_override(const Permissions& p);

    friend std::ostream& operator<<(std::ostream& os, const ActivePermissions& p) {
        using namespace std::string_literals;
        os << "ActivePermissions: "s << p.permissions_;
        return os;
    }

private:
    Permissions permissions_ = Permissions::make_empty();
};

/**
 * \brief This is a Tag type used to indicate that the permissions follow the nominal rules.
 */
struct Rules
{
};

/**
 * \brief This is a Tag type used to indicate that the permissions are unrestricted.
 */
struct Unrestricted
{
};

ActivePermissions
permissions_at(const Defs& defs, const std::string& path, const std::variant<Unrestricted, Rules>& permissions);

} // namespace ecf

#endif /* ecflow_node_permissions_ActivePermissions_HPP */
