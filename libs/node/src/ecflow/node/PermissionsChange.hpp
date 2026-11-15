// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/node/EffectivePermissions.hpp"

class Defs;
class Node;

namespace ecf {

///
/// @brief Describes one change of a Permissions attribute, made by a PermissionsChange.
///
struct PermissionsChangeRecord
{
    std::string path;                  ///< The path of the node, or "/" for the server attribute
    std::optional<Permissions> before; ///< The attribute before the change; std::nullopt when the node had none
    std::optional<Permissions> after;  ///< The attribute after the change; std::nullopt when the node has none
    bool cascade{false};               ///< True for a change made by the removal of a user above the node
};

///
/// @brief Validates and applies changes of Permissions attributes, all or nothing.
///
/// Every operation that changes Permissions attributes in place goes through this pipeline:
///
/// 1. the candidate: the attributes requested, through set() and set_server();
/// 2. the cascade: below every changed attribute, the entries naming users no longer listed above are removed, and
///    the attributes left empty are removed, so that their nodes inherit (D2.17);
/// 3. the checks, which reject the whole change:
///    - node attributes require a server attribute (D2.20);
///    - the entries of a requested attribute name only users listed above its node (D2.15, D2.18);
///    - no node, nor the server, is left without any user holding p, when one held it before (D4.6);
/// 4. the commit, as a modify change of the suites concerned, or of the server (D3.7);
/// 5. one log record per changed attribute, naming the requester, the old and the new values (R4.4).
///
/// The users listed above a suite are those of the server attribute (D2.15); the users listed above a node below a
/// suite are those holding effective roles at its parent (D2.18), sticky server entries included.
///
/// The syntax of entries (D2.19) is guaranteed by the Permissions type.
///
class PermissionsChange {
public:
    ///
    /// @brief Prepares a change of the attributes of definitions.
    ///
    /// @param[in] defs The definitions; they must outlive the change, unchanged until commit().
    ///
    explicit PermissionsChange(Defs& defs);

    ///
    /// @brief Requests a value for the server attribute.
    ///
    /// The server attribute can only be updated: creating and removing it at run time are refused by the commands
    /// (D10.2).
    ///
    /// @param[in] value The new value
    ///
    void set_server(const Permissions& value);

    ///
    /// @brief Requests a value for the attribute of a node.
    ///
    /// @param[in] node A node of the definitions
    /// @param[in] value The new value, or std::nullopt to remove the attribute
    ///
    void set(Node& node, std::optional<Permissions> value);

    ///
    /// @brief Validates the change and computes its cascade, without changing the definitions.
    ///
    /// @throws std::runtime_error naming the reason when the change is rejected.
    ///
    void validate();

    ///
    /// @brief Applies the change, after validating it when validate() was not called.
    ///
    /// @param[in] requester The identity of the requester, written in the log records
    /// @throws std::runtime_error when the change is rejected; the definitions are then unchanged.
    ///
    void commit(const std::string& requester);

    ///
    /// @brief Returns the changes made, or to be made, in the order of a walk of the tree.
    ///
    /// @return The changes; empty before validate(). A requested value equal to the current one is not a change.
    ///
    [[nodiscard]] const std::vector<PermissionsChangeRecord>& changes() const { return records_; }

private:
    [[nodiscard]] const Permissions* server_after() const;
    [[nodiscard]] const Permissions* candidate(const Node& node) const;
    [[nodiscard]] std::vector<EffectivePermissions::State> states_of_parent(const Node& node) const;
    [[nodiscard]] bool
    listed(const std::string& user, const Node& node, const std::vector<EffectivePermissions::State>& parent) const;
    void cascade(Node& node, const std::vector<EffectivePermissions::State>& parent);
    void check_lockout() const;

    Defs& defs_;
    std::optional<Permissions> server_;
    std::vector<std::pair<Node*, std::optional<Permissions>>> requested_;
    std::unordered_set<const Node*> requested_nodes_;
    std::unordered_map<const Node*, std::optional<Permissions>> candidate_;
    std::vector<PermissionsChangeRecord> records_;
    std::vector<Node*> changed_;                // the node of each record; nullptr for the server
    std::vector<EffectivePermissions> walkers_; // one per user of the server attribute after the change
    bool validated_{false};
};

} // namespace ecf
