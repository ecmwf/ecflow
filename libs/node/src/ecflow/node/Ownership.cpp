// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/Ownership.hpp"

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/NodeContainer.hpp"
#include "ecflow/node/Suite.hpp"

namespace ecf::ownership {

bool contains_owner(const Node& node) {
    if (node.get_owner()) {
        return true;
    }
    if (const NodeContainer* container = node.isNodeContainer(); container) {
        for (const auto& child : container->children()) {
            if (contains_owner(*child)) {
                return true;
            }
        }
    }
    return false;
}

bool contains_owner(const Defs& defs) {
    for (const auto& suite : defs.suites()) {
        if (contains_owner(*suite)) {
            return true;
        }
    }
    return false;
}

} // namespace ecf::ownership
