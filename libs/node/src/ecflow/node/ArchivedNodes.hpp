// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>

class Node;

namespace ecf {

///
/// @brief Calls a function for every node of a subtree, the archived descendants included.
///
/// The children of an archived container are read from its archive file, located as NodeContainer::archive_path()
/// does (with the ECF_HOME of the live nodes and the port of their definitions), without changing anything, the change
/// numbers of the server included; nested archives are read in the same way. An archive that cannot be read (missing,
/// unreadable, invalid, or not holding the container as a container) is skipped, with a warning in the log, and
/// reported by the result.
///
/// @param[in] subtree The root of the subtree, visited first
/// @param[in] visit The function, called for each node, in the order of a walk of the tree; the nodes read from an
///            archive live only for the duration of the call
/// @return True when every archive was read; false when one was skipped.
///
bool for_each_node(const Node& subtree, const std::function<void(const Node&)>& visit);

///
/// @brief Returns whether a node, or any node below it, carries a Permissions attribute, archived nodes included.
///
/// Used to decide whether p is required: an archive that cannot be read counts as carrying attributes, so that the
/// decision fails closed.
///
/// @param[in] node The node
/// @return True when an attribute is found, or when an archive could not be read.
///
bool carries_permissions(const Node& node);

} // namespace ecf
