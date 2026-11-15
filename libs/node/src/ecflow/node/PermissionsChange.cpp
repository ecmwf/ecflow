// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/PermissionsChange.hpp"

#include <algorithm>
#include <functional>
#include <set>
#include <stdexcept>
#include <string>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/EffectivePermissions.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Visibility.hpp"

namespace ecf {

namespace {

std::string text_of(const std::optional<Permissions>& value) {
    return value ? value->to_cli() : std::string("(none)");
}

// The roles of every user, before and after the change, at one level of the tree
struct Holders
{
    std::vector<EffectivePermissions::State> before;
    std::vector<EffectivePermissions::State> after;
};

} // namespace

PermissionsChange::PermissionsChange(Defs& defs)
    : defs_{defs} {
}

void PermissionsChange::set_server(const Permissions& value) {
    server_    = value;
    validated_ = false;
}

void PermissionsChange::set(Node& node, std::optional<Permissions> value) {
    requested_.emplace_back(&node, std::move(value));
    requested_nodes_.insert(&node);
    validated_ = false;
}

const Permissions* PermissionsChange::server_after() const {
    return server_ ? &*server_ : defs_.server_state().permissions();
}

const Permissions* PermissionsChange::candidate(const Node& node) const {
    if (const auto it = candidate_.find(&node); it != candidate_.end()) {
        return it->second ? &*it->second : nullptr;
    }
    return node.permissions();
}

std::vector<EffectivePermissions::State> PermissionsChange::states_of_parent(const Node& node) const {
    std::vector<const Node*> ancestors;
    for (const Node* parent = node.parent(); parent != nullptr; parent = parent->parent()) {
        ancestors.push_back(parent);
    }
    std::vector<EffectivePermissions::State> states;
    states.reserve(walkers_.size());
    for (const auto& walker : walkers_) {
        auto state = walker.at_server();
        for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
            state = walker.descend(state, candidate(**it));
        }
        states.push_back(state);
    }
    return states;
}

bool PermissionsChange::listed(const std::string& user,
                               const Node& node,
                               const std::vector<EffectivePermissions::State>& parent) const {
    const auto* server = server_after();
    if (!server) {
        return false;
    }
    if (node.parent() == nullptr) {
        // At a suite, the users of the server attribute (D2.15)
        return server->find(user) != nullptr;
    }
    // Below a suite, the users holding effective roles at the parent (D2.18)
    const auto& entries = server->entries();
    const auto it       = std::lower_bound(
        entries.begin(), entries.end(), user, [](const auto& entry, const auto& name) { return entry.user < name; });
    if (it == entries.end() || it->user != user) {
        return false;
    }
    const auto index = static_cast<std::size_t>(it - entries.begin());
    return !walkers_[index].roles(parent[index]).empty();
}

void PermissionsChange::cascade(Node& node, const std::vector<EffectivePermissions::State>& parent) {
    const auto* attribute = candidate(node);
    if (attribute && server_after()) {
        if (requested_nodes_.count(&node) != 0) {
            // A requested attribute names only users listed above (D2.15, D2.18)
            for (const auto& entry : attribute->entries()) {
                if (!listed(entry.user, node, parent)) {
                    throw std::runtime_error("The Permissions attribute of " + node.absNodePath() +
                                             " names the user '" + entry.user + "', who is not listed above it");
                }
            }
        }
        else {
            // The entries of users no longer listed above are removed, and an emptied attribute with them (D2.17)
            std::vector<Permissions::Entry> kept;
            for (const auto& entry : attribute->entries()) {
                if (listed(entry.user, node, parent)) {
                    kept.push_back(entry);
                }
            }
            if (kept.size() != attribute->entries().size()) {
                std::optional<Permissions> after;
                if (!kept.empty()) {
                    after = Permissions(std::move(kept));
                }
                candidate_[&node] = after;
                attribute         = candidate(node);
            }
        }
    }

    std::vector<EffectivePermissions::State> here;
    here.reserve(walkers_.size());
    for (std::size_t i = 0; i < walkers_.size(); ++i) {
        here.push_back(walkers_[i].descend(parent[i], attribute));
    }
    std::vector<node_ptr> children;
    node.immediateChildren(children);
    for (const auto& child : children) {
        cascade(*child, here);
    }
}

void PermissionsChange::check_lockout() const {
    // The users who can hold roles are those of the server attribute, before or after the change
    std::set<std::string> users;
    for (const auto* server : {defs_.server_state().permissions(), server_after()}) {
        if (server) {
            for (const auto& entry : server->entries()) {
                users.insert(entry.user);
            }
        }
    }
    std::vector<EffectivePermissions> before;
    std::vector<EffectivePermissions> after;
    for (const auto& user : users) {
        before.emplace_back(defs_.server_state().permissions(), user);
        after.emplace_back(server_after(), user);
    }

    const auto holds_p = [](const std::vector<EffectivePermissions>& walkers,
                            const std::vector<EffectivePermissions::State>& states) {
        for (std::size_t i = 0; i < walkers.size(); ++i) {
            if (walkers[i].roles(states[i]).has(Role::Permissions)) {
                return true;
            }
        }
        return false;
    };
    const auto check = [&](const std::string& path, const Holders& holders) {
        if (holds_p(before, holders.before) && !holds_p(after, holders.after)) {
            throw std::runtime_error("The change would leave nobody able to change the Permissions attribute of " +
                                     path);
        }
    };

    Holders server;
    for (std::size_t i = 0; i < users.size(); ++i) {
        server.before.push_back(before[i].at_server());
        server.after.push_back(after[i].at_server());
    }

    // Walks the subtree of a node, from the holders of its parent
    std::function<void(const Node&, const Holders&)> walk = [&](const Node& node, const Holders& parent) {
        Holders here;
        for (std::size_t i = 0; i < users.size(); ++i) {
            here.before.push_back(before[i].descend(parent.before[i], node.permissions()));
            here.after.push_back(after[i].descend(parent.after[i], candidate(node)));
        }
        check(node.absNodePath(), here);
        std::vector<node_ptr> children;
        node.immediateChildren(children);
        for (const auto& child : children) {
            walk(*child, here);
        }
    };

    if (server_) {
        // The server attribute applies to the whole tree
        check("/", server);
        for (const auto& suite : defs_.suites()) {
            walk(*suite, server);
        }
        return;
    }

    // Otherwise, only the subtrees of the changed nodes are affected
    for (const auto* node : changed_) {
        if (!node) {
            continue; // the server, not changed here
        }
        bool below_changed = false;
        for (const Node* parent = node->parent(); parent != nullptr && !below_changed; parent = parent->parent()) {
            below_changed = std::find(changed_.begin(), changed_.end(), parent) != changed_.end();
        }
        if (below_changed) {
            continue; // walked from the changed ancestor
        }
        std::vector<const Node*> ancestors;
        for (const Node* parent = node->parent(); parent != nullptr; parent = parent->parent()) {
            ancestors.push_back(parent);
        }
        Holders holders = server;
        for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
            Holders next;
            for (std::size_t i = 0; i < users.size(); ++i) {
                next.before.push_back(before[i].descend(holders.before[i], (*it)->permissions()));
                next.after.push_back(after[i].descend(holders.after[i], candidate(**it)));
            }
            holders = std::move(next);
        }
        walk(*node, holders);
    }
}

void PermissionsChange::validate() {
    validated_ = false;
    candidate_.clear();
    records_.clear();
    changed_.clear();

    // Node attributes require a server attribute (D2.20)
    if (!server_after()) {
        for (const auto& [node, value] : requested_) {
            if (value) {
                throw std::runtime_error("The Permissions attribute of " + node->absNodePath() +
                                         " requires a server Permissions attribute");
            }
        }
    }

    for (const auto& [node, value] : requested_) {
        candidate_[node] = value;
    }

    // One walker per user of the server attribute, in its order (sorted by user name)
    walkers_.clear();
    if (const auto* server = server_after(); server) {
        for (const auto& entry : server->entries()) {
            walkers_.emplace_back(server, entry.user);
        }
    }

    // The cascade, from the topmost requested nodes, or from every suite when the server attribute changes
    if (server_) {
        for (const auto& suite : defs_.suites()) {
            cascade(*suite, states_of_parent(*suite));
        }
    }
    else {
        for (const auto& [node, value] : requested_) {
            bool below_requested = false;
            for (const Node* parent = node->parent(); parent != nullptr && !below_requested;
                 parent             = parent->parent()) {
                below_requested = requested_nodes_.count(parent) != 0;
            }
            if (!below_requested) {
                cascade(*node, states_of_parent(*node));
            }
        }
    }

    // The records, in the order of a walk of the tree
    if (server_ && (!defs_.server_state().permissions() || *defs_.server_state().permissions() != *server_)) {
        records_.push_back(PermissionsChangeRecord{"/",
                                                   defs_.server_state().permissions()
                                                       ? std::optional<Permissions>(*defs_.server_state().permissions())
                                                       : std::nullopt,
                                                   server_,
                                                   false});
        changed_.push_back(nullptr);
    }
    // Only the candidate nodes and their ancestors are visited, in the order of the tree, so that a small change does
    // not walk the whole tree
    std::unordered_set<const Node*> on_path;
    for (const auto& [node, value] : candidate_) {
        for (const Node* n = node; n != nullptr && on_path.insert(n).second; n = n->parent()) {}
    }
    std::function<void(Node&)> collect = [&](Node& node) {
        if (const auto it = candidate_.find(&node); it != candidate_.end()) {
            const auto* current = node.permissions();
            const bool same     = current ? (it->second && *current == *it->second) : !it->second;
            if (!same) {
                records_.push_back(
                    PermissionsChangeRecord{node.absNodePath(),
                                            current ? std::optional<Permissions>(*current) : std::nullopt,
                                            it->second,
                                            requested_nodes_.count(&node) == 0});
                changed_.push_back(&node);
            }
        }
        std::vector<node_ptr> children;
        node.immediateChildren(children);
        for (const auto& child : children) {
            if (on_path.count(child.get()) != 0) {
                collect(*child);
            }
        }
    };
    for (const auto& suite : defs_.suites()) {
        if (on_path.count(suite.get()) != 0) {
            collect(*suite);
        }
    }

    // No node, nor the server, is left without any user able to change its attribute (D4.6)
    if (!records_.empty()) {
        check_lockout();
    }
    validated_ = true;
}

void PermissionsChange::commit(const std::string& requester) {
    if (!validated_) {
        validate();
    }

    std::set<Suite*> suites;
    for (std::size_t i = 0; i < records_.size(); ++i) {
        const auto& record = records_[i];
        if (Node* node = changed_[i]; node) {
            node->set_permissions(record.after ? &*record.after : nullptr);
            suites.insert(node->suite());
        }
        else {
            defs_.server_state().set_permissions(*record.after);
            Visibility::structure_changed();
            defs_.server_modified();
        }
        ecf::log(Log::LOG,
                 " permissions " + record.path + " " + text_of(record.before) + " -> " + text_of(record.after) +
                     (record.cascade ? " (removal of users above)" : "") + " by " + requester);
    }

    // A change of permissions can change what clients may read: clients of the suites concerned perform a full
    // synchronisation (D3.7)
    for (auto* suite : suites) {
        suite->set_modify_change_no(Ecf::incr_modify_change_no());
    }

    requested_.clear();
    requested_nodes_.clear();
    server_.reset();
    validated_ = false;
}

} // namespace ecf
