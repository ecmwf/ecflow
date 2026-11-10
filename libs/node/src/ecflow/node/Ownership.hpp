// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>

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
/// @brief Checks whether a node, or any node below it, holds an Owner attribute.
///
/// @param[in] node The root of the subtree to inspect.
/// @return true if @p node or any of its descendants has an Owner; false otherwise.
///
bool contains_owner(const Node& node);

///
/// @brief Checks whether any node of the given definitions holds an Owner attribute.
///
/// @param[in] defs The definitions to inspect.
/// @return true if any node of @p defs has an Owner; false otherwise.
///
bool contains_owner(const Defs& defs);

///
/// @brief Checks whether a node of the server, or any node below it, holds an Owner attribute, including the nodes
///        held in archive files.
///
/// @details The archive files of archived nodes below @p node (at any depth) are read, without restoring them, so
///          that the server tree and its archive files are left unchanged.
///
/// @param[in] node The root of the subtree to inspect, in the server definitions.
/// @return true if @p node or any of its descendants, archived or not, has an Owner; false otherwise.
/// @throws std::runtime_error if an archive file cannot be read.
///
bool contains_owner_including_archives(const Node& node);

} // namespace ecf::ownership
