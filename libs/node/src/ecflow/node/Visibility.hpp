// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>

#include "ecflow/node/EffectivePermissions.hpp"

class Defs;
class Node;

namespace ecf {

///
/// @brief Tells which nodes of the definitions a user can read, i.e. where the user holds the role r.
///
/// The visibility is computed once, from the definitions as they are when it is created, by walking the tree from the
/// server down with the effective permissions of the user; the walk stops at each node the user cannot read, since
/// no node below it is readable (from the suite down, permissions only restrict, and a sticky entry only keeps what
/// was reached above it).
///
/// Without a server attribute, Permissions do not apply and every node is readable. A user not admitted by the
/// server attribute reads no node.
///
/// @invariant A node is readable only when all its ancestors are.
///
class Visibility {
public:
    ///
    /// @brief Computes the nodes the user can read.
    ///
    /// @param[in] defs The definitions; their nodes must outlive the visibility, unchanged in structure and in
    ///            Permissions attributes
    /// @param[in] user The user
    ///
    Visibility(const Defs& defs, const std::string& user);

    ///
    /// @brief Returns whether Permissions apply, i.e. whether the server has a Permissions attribute.
    ///
    /// @return True when the server has a Permissions attribute.
    ///
    [[nodiscard]] bool applies() const { return applies_; }

    ///
    /// @brief Returns whether the user can read a node of the definitions.
    ///
    /// @param[in] node The node, of the definitions the visibility was computed from
    /// @return True when the user holds r at the node.
    ///
    [[nodiscard]] bool readable(const Node& node) const;

    ///
    /// @brief Returns whether a node is hidden while its parent is readable, i.e. where a walk down the tree stops.
    ///
    /// A writer going down from readable nodes checks each node it meets with this function only, which avoids
    /// checking the ancestors again.
    ///
    /// @param[in] node The node, of the definitions the visibility was computed from, whose parent is readable
    /// @return True when the user cannot read the node.
    ///
    [[nodiscard]] bool stops_at(const Node& node) const;

    ///
    /// @brief Returns a key identifying the set of readable nodes.
    ///
    /// Two users with the same readable nodes have the same key, and two users with different readable nodes have
    /// different keys, so that content built for one can be served to the other only when they read the same nodes.
    ///
    /// @return The key.
    ///
    [[nodiscard]] const std::string& key() const { return key_; }

    ///
    /// @brief Records a change that can alter what users read without a modify change: a node added below a suite
    ///        or removed from it, or a Permissions attribute added to a node or removed from it.
    ///
    /// Visibilities kept for later requests are valid only while the generation is unchanged.
    ///
    static void structure_changed();

    ///
    /// @brief Returns the number of changes recorded by structure_changed().
    ///
    /// @return The generation; it only increases.
    ///
    [[nodiscard]] static std::uint64_t structure_generation();

private:
    void visit(const EffectivePermissions& walker, const EffectivePermissions::State& parent, const Node& node);

    bool applies_{false};
    bool admitted_{true};
    std::unordered_set<const Node*> unreadable_; // the topmost unreadable nodes; nothing below them is visited
    std::string key_;
};

} // namespace ecf
