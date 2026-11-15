// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <iosfwd>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/node/EffectivePermissions.hpp"
#include "ecflow/node/NodeFwd.hpp"

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
/// @brief Reports an invalid Permissions attribute found in definitions, naming the node that carries it.
///
class InvalidPermissions : public std::runtime_error {
public:
    ///
    /// @brief Creates the report.
    ///
    /// @param[in] path The path of the node carrying the attribute
    /// @param[in] what The reason, naming the node
    ///
    InvalidPermissions(std::string path, const std::string& what)
        : std::runtime_error(what),
          path_{std::move(path)} {}

    ///
    /// @brief Returns the path of the node carrying the attribute.
    ///
    /// @return The path.
    ///
    [[nodiscard]] const std::string& path() const { return path_; }

private:
    std::string path_;
};

///
/// @brief Returns the number of the line of definitions, in the defs format, holding the Permissions attribute of a
///        node.
///
/// The lines are read one by one, following the keywords suite, family, task and alias, and their end keywords
/// (endtask included), to know the node of each line.
///
/// @param[in,out] in The definitions, read to their end or to the line found
/// @param[in] path The path of the node
/// @return The number, from 1; 0 when the line is not found.
///
int line_of_permissions(std::istream& in, const std::string& path);

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
    /// @brief Validates a subtree about to be inserted into the definitions, and records the attributes it changes.
    ///
    /// Used by the operations that bring nodes with their attributes: load, replace, plug and move. The subtree is
    /// validated where it will be inserted, under the current server attribute:
    ///
    /// - node attributes require a server attribute (D2.20);
    /// - its attributes name only users listed above their nodes; a user not listed is rejected, never removed
    ///   (D2.15, D2.18);
    /// - no node of the subtree is left without any user holding p, when one held it before: at the node of the
    ///   definitions with the same path, or, for a new node, at the node above it (D4.6); see validate_move() for a
    ///   subtree moved within the definitions.
    ///
    /// The records compare the attributes of the subtree with those of the nodes of the definitions at the same paths,
    /// below the target path: an attribute the subtree no longer carries is recorded as removed. They are added to the
    /// records made before, e.g. by record_removal() for the source of a plug.
    ///
    /// Without a server attribute, no node of the definitions carries one (D2.20): the subtree is only checked to carry
    /// none, the first node carrying one, in the order of a walk of the tree, being named; nothing is recorded, and
    /// neither the definitions nor the archives are read.
    ///
    /// @param[in] subtree The root of the subtree; it may still be part of the definitions, elsewhere (a move)
    /// @param[in] parent The node of the definitions under which the subtree is inserted, or nullptr for a suite
    /// @throws std::runtime_error naming the reason when the subtree is rejected.
    ///
    void validate_transfer(const Node& subtree, const Node* parent);

    ///
    /// @brief Validates a subtree about to be moved within the definitions (a local plug), as validate_transfer() does.
    ///
    /// The lockout check compares each node of the subtree with itself where it was, through the effective permissions
    /// at its former place (D4.6): a node that had a holder of p must keep one.
    ///
    /// @param[in] subtree The subtree moved, or a copy of it holding its archived descendants restored
    /// @param[in] from The node of the definitions the subtree leaves, or nullptr for a suite
    /// @param[in] to The node of the definitions under which the subtree goes, or nullptr for a suite
    /// @throws std::runtime_error naming the reason when the move is rejected.
    ///
    void validate_move(const Node& subtree, const Node* from, const Node* to);

    ///
    /// @brief Records as removed the attributes of a subtree about to leave the definitions (delete, or the source of
    ///        a plug), for log().
    ///
    /// The archived descendants are read from their archive files, without changing anything. Nothing is checked:
    /// removing nodes leaves the attributes of the other nodes, and who holds p there, unchanged. Without a server
    /// attribute, no node carries one (D2.20): nothing is recorded, and no archive is read.
    ///
    /// @param[in] subtree The root of the subtree, part of the definitions
    ///
    void record_removal(const Node& subtree);

    ///
    /// @brief Applies the restoration policy to archived nodes, before they are restored (D3.6).
    ///
    /// Without a server attribute, node attributes are not allowed (D2.20): when the nodes restored carry some, an
    /// explicit restore fails, as a load would, and an automatic one (autorestore, or a restore during begin or
    /// requeue) strips them, with a warning naming the nodes. With a server attribute, nothing is done here (see
    /// complete_restore()).
    ///
    /// @param[in,out] archived The children read from the archive, about to be restored under @p container
    /// @param[in] container The container restored, part of the definitions
    /// @param[in] requested True for an explicit restore (--restore, or of the source of a plug among its siblings)
    /// @throws std::runtime_error for an explicit restore of nodes carrying attributes, without a server attribute.
    ///
    void prepare_restore(const std::vector<node_ptr>& archived, const Node& container, bool requested) const;

    ///
    /// @brief Applies the restoration policy to the nodes just restored under a container (D3.5).
    ///
    /// With a server attribute, the entries naming users no longer listed above are removed, and the attributes left
    /// empty with them, as in the cascade of D2.17; each removal is logged. The restore never fails because of this:
    /// a restored node left without any holder of p, below a node that has one, is accepted, with a warning naming
    /// it.
    ///
    /// @param[in,out] container The container restored, part of the definitions
    ///
    void complete_restore(Node& container);

    ///
    /// @brief Checks every attribute of the definitions, as loaded from a checkpoint (R3.5, D11.4).
    ///
    /// Node attributes require a server attribute (D2.20), and their entries name only users listed above their
    /// nodes (D2.15, D2.18). Self-lockouts are not checked: a restore can leave them (D3.5). Without a server
    /// attribute, the first node carrying one, in the order of a walk of the tree, is at fault.
    ///
    /// @param[in] server The server attribute in force: that of the checkpoint, or of the configuration file; nullptr
    ///            when none
    /// @throws InvalidPermissions naming the first node at fault.
    ///
    void check_definitions(const Permissions* server);

    ///
    /// @brief Writes the log records of the changes validated by validate_transfer(), once the operation is applied.
    ///
    /// @param[in] requester The identity of the requester, written in the log records
    ///
    void log(const std::string& requester) const;

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
    [[nodiscard]] std::vector<EffectivePermissions::State> states_at(const Node* node) const;
    [[nodiscard]] bool holds_p(const std::vector<EffectivePermissions::State>& states) const;
    [[nodiscard]] bool
    listed(const std::string& user, bool suite, const std::vector<EffectivePermissions::State>& parent) const;
    void validate_transfer(const Node& subtree, const Node* parent, const Node* from, bool moved);
    void check_node(const Node& node, const std::vector<EffectivePermissions::State>& parent, bool suite) const;
    void transfer(const Node& node,
                  const std::string& path,
                  const std::vector<EffectivePermissions::State>& parent,
                  const std::vector<EffectivePermissions::State>* old_parent,
                  bool parent_held_p,
                  bool suite);
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
    std::set<std::string> transferred_;         // the paths of the nodes of a subtree transferred
    std::optional<std::vector<std::pair<std::string, Permissions>>> replaced_; // the attributes a transfer replaces
    std::unordered_map<std::string, std::size_t> replaced_index_;              // their positions, by path
    bool validated_{false};
};

} // namespace ecf
