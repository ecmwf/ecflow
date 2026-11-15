// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>

#include "ecflow/attribute/Permissions.hpp"

class Defs;
class Node;

namespace ecf {

///
/// @brief Computes the effective permissions of a user, walking the tree from the server down.
///
/// The rules are those of the decisions on effective permissions:
///
/// - admission: a user without an entry granting r in the server attribute has no roles (only --ping remains);
/// - at the server, the user has the roles of its server entry;
/// - a sticky server entry is kept aside, and united with the result at every node;
/// - a suite replaces the non-sticky server entry with its own entry, or takes the server entry when the suite has no
///   attribute; it may broaden the roles;
/// - below the suite, each attribute intersects the inherited roles with those of its entry for the user, and leaves
///   no roles when it does not list the user;
/// - from the suite down, the first sticky entry of the user locks the set reached there: the attributes below no
///   longer change it.
///
/// Without a server attribute, Permissions do not apply (applies() is false).
///
/// The walker takes a snapshot of the server attribute when it is created; descend() reads the attribute of each node
/// it is given. A walker must therefore be created again after the server attribute changes.
///
class EffectivePermissions {
public:
    ///
    /// @brief Holds the state carried from a node to its children.
    ///
    struct State
    {
        Roles roles;        ///< The roles reached, before the sticky server entry is united with them
        bool locked{false}; ///< True once a sticky entry below the server has locked the roles
        bool server{true};  ///< True at the server; false from the suites down
    };

    ///
    /// @brief Prepares the walk for a user.
    ///
    /// @param[in] defs The definitions, holding the server attribute
    /// @param[in] user The user
    ///
    EffectivePermissions(const Defs& defs, std::string user);

    ///
    /// @brief Prepares the walk for a user, under a given server attribute.
    ///
    /// Used to evaluate a candidate server attribute before it is applied.
    ///
    /// @param[in] server The server attribute, or nullptr when the server has none
    /// @param[in] user The user
    ///
    EffectivePermissions(const Permissions* server, std::string user);

    ///
    /// @brief Returns whether Permissions apply, i.e. whether the server has a Permissions attribute.
    ///
    /// @return True when the server has a Permissions attribute.
    ///
    [[nodiscard]] bool applies() const { return applies_; }

    ///
    /// @brief Returns whether the user is admitted: listed by the server attribute with r.
    ///
    /// @return True when the user is admitted.
    ///
    [[nodiscard]] bool admitted() const { return admitted_; }

    ///
    /// @brief Returns the state at the server.
    ///
    /// @return The state; its roles are those of the server entry of an admitted user, and empty otherwise.
    ///
    [[nodiscard]] State at_server() const;

    ///
    /// @brief Returns the state of a node, from the state of its parent.
    ///
    /// @param[in] parent The state of the parent: at_server() for a suite
    /// @param[in] node The node
    /// @return The state of the node.
    ///
    [[nodiscard]] State descend(const State& parent, const Node& node) const;

    ///
    /// @brief Returns the state of a node carrying a given attribute, from the state of its parent.
    ///
    /// Used to evaluate a candidate attribute before it is applied.
    ///
    /// @param[in] parent The state of the parent: at_server() for a suite
    /// @param[in] attribute The attribute of the node, or nullptr when the node has none
    /// @return The state of the node.
    ///
    [[nodiscard]] State descend(const State& parent, const Permissions* attribute) const;

    ///
    /// @brief Returns the effective roles of a state: its roles, united with the sticky server entry.
    ///
    /// @param[in] state The state
    /// @return The effective roles; empty for a user not admitted.
    ///
    [[nodiscard]] Roles roles(const State& state) const;

private:
    std::string user_;
    bool applies_{false};
    bool admitted_{false};
    Roles server_roles_;
    Roles sticky_server_roles_;
};

///
/// @brief Computes the effective roles of a user at a path.
///
/// @param[in] defs The definitions, holding the server attribute
/// @param[in] path The path of a node, or "/" for the server
/// @param[in] user The user
/// @return The effective roles, or std::nullopt when Permissions do not apply (the server has no attribute).
/// @throws std::invalid_argument when no node has the path.
///
std::optional<Roles> effective_permissions(const Defs& defs, const std::string& path, const std::string& user);

} // namespace ecf
