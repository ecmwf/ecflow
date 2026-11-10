// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/Ownership.hpp"

#include <stdexcept>
#include <string>

#include "ecflow/core/Environment.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/NodeContainer.hpp"
#include "ecflow/node/Suite.hpp"

namespace ecf::ownership {

std::string ResolvedOwner::origin_path() const {
    return origin ? origin->absNodePath() : std::string("server");
}

std::optional<ResolvedOwner> resolve(const Node& node) {
    for (const Node* current = &node; current; current = current->parent()) {
        if (const auto* owner = current->get_owner(); owner) {
            return ResolvedOwner{*owner, current};
        }
    }
    if (const Defs* defs = node.defs(); defs) {
        if (const auto* server_owner = defs->server_state().server_owner(); server_owner) {
            return ResolvedOwner{*server_owner, nullptr};
        }
    }
    return std::nullopt;
}

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

namespace {

/// Finds ECF_HOME among the variables of a node read from an archive and of its ancestors, ignoring the
/// definitions of the archive (whose server variables are defaults)
bool find_archived_ecf_home(const Node& node, std::string& ecf_home) {
    for (const Node* current = &node; current; current = current->parent()) {
        if (current->findVariableValue(ecf::environment::ECF_HOME, ecf_home)) {
            return true;
        }
    }
    return false;
}

bool contains_owner_including_archives(const Node& node,
                                       const std::string& inherited_ecf_home,
                                       const Defs* server_defs,
                                       bool from_archive) {
    if (node.get_owner()) {
        return true;
    }
    const NodeContainer* container = node.isNodeContainer();
    if (!container) {
        return false;
    }
    if (!container->get_flag().is_set(ecf::Flag::ARCHIVED) || !container->children().empty()) {
        for (const auto& child : container->children()) {
            if (contains_owner_including_archives(*child, inherited_ecf_home, server_defs, from_archive)) {
                return true;
            }
        }
        return false;
    }

    // An archived node keeps its own attributes, while its children are held in its archive file. A node read
    // from an archive only finds the variables defined within the archived subtree: otherwise, its ECF_HOME is that
    // of the archived node it was read from.
    std::string ecf_home = inherited_ecf_home;
    if (from_archive) {
        find_archived_ecf_home(*container, ecf_home);
    }
    else {
        container->findParentUserVariableValue(ecf::environment::ECF_HOME, ecf_home);
    }
    const std::string path = NodeContainer::make_archive_path(ecf_home, container->absNodePath(), server_defs);
    defs_ptr archive       = Defs::create();
    try {
        archive->restore(path);
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Could not read the archive of " + container->absNodePath() + " at " + path + ": " +
                                 e.what());
    }
    node_ptr archived = archive->findAbsNode(container->absNodePath());
    if (!archived || !archived->isNodeContainer()) {
        throw std::runtime_error("Could not find " + container->absNodePath() + " in its archive at " + path);
    }
    for (const auto& child : archived->isNodeContainer()->children()) {
        if (contains_owner_including_archives(*child, ecf_home, server_defs, true)) {
            return true;
        }
    }
    return false;
}

} // namespace

bool contains_owner_including_archives(const Node& node) {
    std::string server_ecf_home;
    node.findParentUserVariableValue(ecf::environment::ECF_HOME, server_ecf_home);
    return contains_owner_including_archives(node, server_ecf_home, node.defs(), false);
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
