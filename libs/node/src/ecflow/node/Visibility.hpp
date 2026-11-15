// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

#include "ecflow/core/NState.hpp"
#include "ecflow/node/EffectivePermissions.hpp"
#include "ecflow/node/NodeFwd.hpp"

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
    /// @brief Returns the children of a readable node as the user reads them.
    ///
    /// The hidden children are left out; a readable child with hidden nodes below it is replaced by a copy without
    /// them, so that the nodes of the server are never changed; the other children are returned as they are.
    ///
    /// @param[in] children The children of a readable node, of the definitions the visibility was computed from
    /// @return The children to send.
    ///
    [[nodiscard]] std::vector<node_ptr> project(const std::vector<node_ptr>& children) const;

    ///
    /// @brief Returns the aliases of a readable task that the user can read.
    ///
    /// @param[in] aliases The aliases of a readable task
    /// @return The readable aliases.
    ///
    [[nodiscard]] std::vector<alias_ptr> project(const std::vector<alias_ptr>& aliases) const;

    ///
    /// @brief Returns the names of the readable nodes among the children of a readable node, in their order.
    ///
    /// @tparam Pointer The type of pointer to the children (node, suite or alias)
    /// @param[in] children The children
    /// @return The names.
    ///
    template <typename Pointer>
    [[nodiscard]] std::vector<std::string> names(const std::vector<Pointer>& children) const;

    ///
    /// @brief Returns the state of the root as the user reads it: the most significant state of the readable suites
    ///        among those given.
    ///
    /// The suites are those a client receives: every suite without a client handle, the suites of the handle
    /// otherwise; full and incremental synchronisations pass the same ones, so that they agree, and the news of a
    /// handle, which follows its suites, reports every change of that state.
    ///
    /// @param[in] defs The definitions, whose state is returned when Permissions do not apply
    /// @param[in] suites The suites the client receives
    /// @return The state.
    ///
    [[nodiscard]] NState::State root_state(const Defs& defs, const std::vector<suite_ptr>& suites) const;

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
    void prune(const Node& original, Node& copy) const;

    bool applies_{false};
    bool admitted_{true};
    std::unordered_set<const Node*> unreadable_;    // the topmost unreadable nodes; nothing below them is visited
    std::unordered_set<const Node*> partly_hidden_; // the readable nodes with unreadable nodes below them
    std::string key_;
};

template <typename Pointer>
std::vector<std::string> Visibility::names(const std::vector<Pointer>& children) const {
    std::vector<std::string> result;
    result.reserve(children.size());
    for (const auto& child : children) {
        if (!stops_at(*child)) {
            result.push_back(child->name());
        }
    }
    return result;
}

} // namespace ecf
