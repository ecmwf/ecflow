// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/Ownership.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Log.hpp"
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

std::string child_path(const std::string& parent_path, const Node& child) {
    return parent_path + "/" + child.name();
}

/// Collects the Owners of a subtree, giving them the paths they have (or will have) in the server tree
void collect(const Node& node, const std::string& path, std::vector<OwnerAtPath>& owners) {
    if (const auto* owner = node.get_owner(); owner) {
        owners.push_back(OwnerAtPath{path, *owner});
    }
    if (const NodeContainer* container = node.isNodeContainer(); container) {
        for (const auto& child : container->children()) {
            collect(*child, child_path(path, *child), owners);
        }
    }
}

/// How an archive file that cannot be read is handled
enum class Unreadable { FAIL, SKIP };

///
/// Collects the Owners of a subtree of the server, including those held in the archive files of its archived
/// nodes. A missing archive file holds nothing that could be restored, and is skipped; an unreadable one either
/// fails the collection, or is skipped with a warning in the server log.
///
/// @param inherited_ecf_home The ECF_HOME of the archived node a node was read from (for nodes read from archives)
///
void collect_including_archives(const Node& node,
                                const std::string& inherited_ecf_home,
                                const Defs* server_defs,
                                bool from_archive,
                                Unreadable unreadable,
                                std::vector<OwnerAtPath>& owners) {
    if (const auto* owner = node.get_owner(); owner) {
        owners.push_back(OwnerAtPath{node.absNodePath(), *owner});
    }
    const NodeContainer* container = node.isNodeContainer();
    if (!container) {
        return;
    }
    if (!container->get_flag().is_set(ecf::Flag::ARCHIVED) || !container->children().empty()) {
        for (const auto& child : container->children()) {
            collect_including_archives(*child, inherited_ecf_home, server_defs, from_archive, unreadable, owners);
        }
        return;
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
    std::string error;
    std::error_code access;
    const bool exists = fs::exists(path, access);
    if (access) {
        error = access.message(); // e.g. a directory that cannot be traversed: handled as an unreadable archive
    }
    else if (!exists) {
        return;
    }
    defs_ptr archive = Defs::create();
    node_ptr archived;
    try {
        if (!error.empty()) {
            throw std::runtime_error(error);
        }
        archive->restore(path);
        archived = archive->findAbsNode(container->absNodePath());
        if (!archived || !archived->isNodeContainer()) {
            error = "the node is not in the archive";
        }
    }
    catch (const std::exception& e) {
        error = e.what();
    }
    if (!error.empty()) {
        const std::string message =
            "Could not read the archive of " + container->absNodePath() + " at " + path + ": " + error;
        if (unreadable == Unreadable::FAIL) {
            throw std::runtime_error(message);
        }
        ecf::log(Log::WAR, message + " (its Owners, if any, are not checked)");
        return;
    }
    for (const auto& child : archived->isNodeContainer()->children()) {
        collect_including_archives(*child, ecf_home, server_defs, true, unreadable, owners);
    }
}

std::vector<OwnerAtPath> collect_from_server(const Node& node, Unreadable unreadable) {
    std::vector<OwnerAtPath> owners;
    std::string ecf_home;
    node.findParentUserVariableValue(ecf::environment::ECF_HOME, ecf_home);
    collect_including_archives(node, ecf_home, node.defs(), false, unreadable, owners);
    return owners;
}

void append(std::vector<OwnerAtPath>& to, const std::vector<OwnerAtPath>& from) {
    to.insert(to.end(), from.begin(), from.end());
}

} // namespace

std::vector<OwnerAtPath> ChangeSet::effective_removals() const {
    std::vector<OwnerAtPath> effective;
    for (const auto& r : removed) {
        auto replaced = std::find_if(
            added.begin(), added.end(), [&r](const OwnerAtPath& a) { return a.path == r.path && a.owner == r.owner; });
        if (replaced == added.end()) {
            effective.push_back(r);
        }
    }
    return effective;
}

ChangeSet change_set_for_load(const Defs& server, const Defs& incoming, bool force) {
    ChangeSet changes;
    for (const auto& suite : incoming.suites()) {
        collect(*suite, "/" + suite->name(), changes.added);
        if (force) {
            if (auto existing = server.findSuite(suite->name()); existing) {
                append(changes.removed, collect_from_server(*existing, Unreadable::SKIP));
            }
        }
    }
    return changes;
}

ChangeSet
change_set_for_replace(const Defs& server, const std::string& path, const Defs& incoming, bool create_parents) {
    ChangeSet changes;
    node_ptr client_node = incoming.findAbsNode(path);
    if (!client_node) {
        return changes; // the command fails without any change
    }

    if (node_ptr server_node = server.findAbsNode(path); server_node || !create_parents) {
        collect(*client_node, path, changes.added);
        if (server_node) {
            changes.removed = collect_from_server(*server_node, Unreadable::SKIP);
        }
        return changes;
    }

    // The node is added, with the missing parents, from the closest parent existing on the server
    const Node* added_root = client_node.get();
    for (const Node* parent = client_node->parent(); parent; parent = parent->parent()) {
        if (server.findAbsNode(parent->absNodePath())) {
            break;
        }
        added_root = parent;
    }
    collect(*added_root, added_root->absNodePath(), changes.added);
    return changes;
}

ChangeSet change_set_for_move_in(const Node& moved, const std::string& parent_path) {
    ChangeSet changes;
    collect(moved, parent_path + "/" + moved.name(), changes.added);
    return changes;
}

ChangeSet change_set_for_move_out(const Node& moved) {
    ChangeSet changes;
    changes.removed = collect_from_server(moved, Unreadable::FAIL);
    return changes;
}

std::vector<OwnerAtPath> owners_including_archives(const Node& node) {
    return collect_from_server(node, Unreadable::SKIP);
}

void log_removals(const ChangeSet& change_set, const std::string& requester, const std::string& command) {
    for (const auto& removal : change_set.effective_removals()) {
        ecf::log(Log::LOG,
                 "Owner " + removal.owner.value() + " of " + removal.path + " removed by " + requester + " (" +
                     command + ")");
    }
}

} // namespace ecf::ownership
