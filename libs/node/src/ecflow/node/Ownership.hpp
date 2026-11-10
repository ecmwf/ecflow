// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

class Defs;
class Node;

namespace ecf::ownership {

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

} // namespace ecf::ownership
