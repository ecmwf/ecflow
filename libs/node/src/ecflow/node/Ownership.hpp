// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ecflow/attribute/OwnerAttr.hpp"

class Defs;
class Node;

namespace ecf::ownership {

///
/// @brief The effective Owner of a node, and where it comes from.
///
struct ResolvedOwner
{
    ecf::OwnerAttr owner;         ///< The effective Owner.
    const Node* origin = nullptr; ///< The node holding the Owner, or nullptr when it is the server Owner.

    ///
    /// @brief Returns the origin of the effective Owner, as reported to users.
    ///
    /// @return The absolute path of the node holding the Owner, or "server" for the server Owner.
    ///
    [[nodiscard]] std::string origin_path() const;
};

///
/// @brief Resolves the effective Owner of a node.
///
/// @details The effective Owner is the node's own Owner, otherwise the Owner of the closest ancestor (up to the
///          suite), otherwise the server Owner. An alias holds no Owner: the resolution starts at its task.
///
/// @param[in] node The node whose effective Owner is resolved.
/// @return The effective Owner and its origin, or no value when neither the path nor the server has an Owner.
///
std::optional<ResolvedOwner> resolve(const Node& node);

///
/// @brief An Owner attribute, and the path of the node holding it.
///
struct OwnerAtPath
{
    std::string path;     ///< The absolute path of the node, in the server tree.
    ecf::OwnerAttr owner; ///< The Owner.
};

///
/// @brief The Owners a command adds to, and removes from, the server tree.
///
/// @details Computed against the current tree, without changing it, so that the whole set can be checked before
///          the command has any effect. An Owner replaced by an identical one at the same path appears in both.
///
struct ChangeSet
{
    std::vector<OwnerAtPath> added;   ///< The Owners brought in by the command.
    std::vector<OwnerAtPath> removed; ///< The existing Owners taken out by the command.

    ///
    /// @brief Returns the Owners removed, and not replaced by an identical Owner at the same path.
    ///
    /// @return The Owners whose removal takes effect.
    ///
    [[nodiscard]] std::vector<OwnerAtPath> effective_removals() const;
};

///
/// @brief Computes the Owners changed by loading definitions (--load).
///
/// @param[in] server The server definitions.
/// @param[in] incoming The definitions loaded.
/// @param[in] force Whether suites of the same name are replaced (--force); otherwise nothing is removed.
/// @return The Owners of the incoming suites, added, and of the server suites they replace, removed (archived
///         nodes included; an archive file that cannot be read is skipped, with a warning in the server log, since
///         its content is replaced and could not be restored anyway).
///
ChangeSet change_set_for_load(const Defs& server, const Defs& incoming, bool force);

///
/// @brief Computes the Owners changed by replacing a node (--replace).
///
/// @details Follows Defs::replaceChild(): an existing node is replaced, with its subtree; a missing node is
///          added, with the missing parents when @p create_parents is set (those parents bringing their Owners).
///
/// @param[in] server The server definitions.
/// @param[in] path The absolute path of the node replaced.
/// @param[in] incoming The client definitions holding the node.
/// @param[in] create_parents Whether missing parents are created (--replace ... parent).
/// @return The Owners added and removed (archived nodes included, as for change_set_for_load()); empty when the
///         node is absent from @p incoming.
///
ChangeSet
change_set_for_replace(const Defs& server, const std::string& path, const Defs& incoming, bool create_parents);

///
/// @brief Computes the Owners added by moving a node into the server tree (the destination of a remote --plug).
///
/// @param[in] moved The node received.
/// @param[in] parent_path The absolute path of the node receiving it, or empty when a suite is received.
/// @return The Owners of the moved subtree, added.
///
ChangeSet change_set_for_move_in(const Node& moved, const std::string& parent_path);

///
/// @brief Computes the Owners removed by moving a node out of the server tree (the source of a remote --plug).
///
/// @details The archive files of archived nodes below @p moved (at any depth) are read, without restoring them, so
///          that the server tree and its archive files are left unchanged.
///
/// @param[in] moved The node moved, in the server definitions.
/// @return The Owners of the moved subtree, archived nodes included, removed.
/// @throws std::runtime_error if an existing archive file cannot be read.
///
ChangeSet change_set_for_move_out(const Node& moved);

///
/// @brief Records in the server log the Owners whose removal took effect.
///
/// @param[in] change_set The changes made by the command.
/// @param[in] requester The user attached to the command.
/// @param[in] command The command, as named to users (e.g. "load").
///
void log_removals(const ChangeSet& change_set, const std::string& requester, const std::string& command);

} // namespace ecf::ownership
