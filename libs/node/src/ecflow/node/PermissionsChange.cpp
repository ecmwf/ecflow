// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/PermissionsChange.hpp"

#include <algorithm>
#include <functional>
#include <istream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/ArchivedNodes.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/EffectivePermissions.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/NodeContainer.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/Visibility.hpp"

namespace ecf {

namespace {

std::string text_of(const std::optional<Permissions>& value) {
    return value ? value->to_cli() : std::string("(none)");
}

void write_record(const PermissionsChangeRecord& record, const std::string& requester) {
    ecf::log(Log::LOG,
             " permissions " + record.path + " " + text_of(record.before) + " -> " + text_of(record.after) +
                 (record.cascade ? " (removal of users above)" : "") + " by " + requester);
}

std::optional<Permissions> value_of(const Permissions* attribute) {
    return attribute ? std::optional<Permissions>(*attribute) : std::nullopt;
}

// Returns the first node of a subtree carrying a Permissions attribute, in the order of a walk of the tree (that of
// immediateChildren(), aliases included), or nullptr; the children are visited in place, without copies
const Node* first_carrying(const Node& node) {
    if (node.permissions()) {
        return &node;
    }
    if (const auto* container = node.isNodeContainer(); container) {
        for (const auto& child : container->children()) {
            if (const auto* found = first_carrying(*child); found) {
                return found;
            }
        }
    }
    else if (const auto* task = node.isTask(); task) {
        for (const auto& alias : task->aliases()) {
            if (alias->permissions()) {
                return alias.get();
            }
        }
    }
    return nullptr;
}

// Returns the path that a node of a subtree takes once the subtree is inserted under a parent (nullptr for a suite)
std::string destination_of(const Node& node, const Node& subtree, const Node* parent) {
    std::vector<const Node*> names;
    for (const Node* n = &node; n != &subtree; n = n->parent()) {
        names.push_back(n);
    }
    names.push_back(&subtree);
    std::string path = parent ? parent->absNodePath() : std::string();
    for (auto it = names.rbegin(); it != names.rend(); ++it) {
        path += "/" + (*it)->name();
    }
    return path;
}

// The roles of every user, before and after the change, at one level of the tree
struct Holders
{
    std::vector<EffectivePermissions::State> before;
    std::vector<EffectivePermissions::State> after;
};

} // namespace

int line_of_permissions(std::istream& in, const std::string& path) {
    std::vector<std::string> nodes; // the names, from the suite down
    bool task        = false;       // true when the last name is a task, whose end keyword is optional
    bool alias       = false;       // true when the last name is an alias: successive aliases are siblings
    int number       = 0;
    auto close_alias = [&]() {
        if (alias && !nodes.empty()) {
            nodes.pop_back();
        }
        alias = false;
    };
    for (std::string line; std::getline(in, line);) {
        ++number;
        std::istringstream words(line);
        std::string keyword, name;
        words >> keyword >> name;
        if (keyword == "suite") {
            nodes.assign(1, name);
            task  = false;
            alias = false;
        }
        else if (keyword == "family" || keyword == "task") {
            close_alias();
            if (task) {
                nodes.pop_back();
            }
            nodes.push_back(name);
            task = keyword == "task";
        }
        else if (keyword == "endtask") {
            close_alias();
            if (task) {
                nodes.pop_back();
            }
            task = false;
        }
        else if (keyword == "alias") {
            close_alias(); // the previous alias, a sibling, ends here (one endalias closes the last)
            nodes.push_back(name);
            alias = true;
        }
        else if (keyword == "endalias") {
            close_alias();
        }
        else if (keyword == "endfamily") {
            close_alias();
            if (task && !nodes.empty()) {
                nodes.pop_back();
            }
            if (!nodes.empty()) {
                nodes.pop_back();
            }
            task = false;
        }
        else if (keyword == "endsuite") {
            nodes.clear();
            task  = false;
            alias = false;
        }
        else if (keyword == "permissions") {
            std::string current;
            for (const auto& node : nodes) {
                current += "/" + node;
            }
            if (current == path) {
                return number;
            }
        }
    }
    return 0;
}

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
                               bool suite,
                               const std::vector<EffectivePermissions::State>& parent) const {
    const auto* server = server_after();
    if (!server) {
        return false;
    }
    if (suite) {
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
                if (!listed(entry.user, node.parent() == nullptr, parent)) {
                    throw std::runtime_error("The Permissions attribute of " + node.absNodePath() +
                                             " names the user '" + entry.user + "', who is not listed above it");
                }
            }
        }
        else {
            // The entries of users no longer listed above are removed, and an emptied attribute with them (D2.17)
            std::vector<Permissions::Entry> kept;
            for (const auto& entry : attribute->entries()) {
                if (listed(entry.user, node.parent() == nullptr, parent)) {
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
        write_record(record, requester);
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

std::vector<EffectivePermissions::State> PermissionsChange::states_at(const Node* node) const {
    if (!node) {
        std::vector<EffectivePermissions::State> states;
        states.reserve(walkers_.size());
        for (const auto& walker : walkers_) {
            states.push_back(walker.at_server());
        }
        return states;
    }
    auto states = states_of_parent(*node);
    for (std::size_t i = 0; i < walkers_.size(); ++i) {
        states[i] = walkers_[i].descend(states[i], candidate(*node));
    }
    return states;
}

bool PermissionsChange::holds_p(const std::vector<EffectivePermissions::State>& states) const {
    for (std::size_t i = 0; i < walkers_.size(); ++i) {
        if (walkers_[i].roles(states[i]).has(Role::Permissions)) {
            return true;
        }
    }
    return false;
}

void PermissionsChange::transfer(const Node& node,
                                 const std::string& path,
                                 const std::vector<EffectivePermissions::State>& parent,
                                 const std::vector<EffectivePermissions::State>* old_parent,
                                 bool parent_held_p,
                                 bool suite) {
    transferred_.insert(path);
    const auto* attribute = node.permissions();
    if (attribute) {
        if (!server_after()) {
            // Node attributes require a server attribute (D2.20)
            throw std::runtime_error("The Permissions attribute of " + path +
                                     " requires a server Permissions attribute");
        }
        // Rejected, never removed: the definitions brought are taken as they are (D2.18)
        for (const auto& entry : attribute->entries()) {
            if (!listed(entry.user, suite, parent)) {
                throw std::runtime_error("The Permissions attribute of " + path + " names the user '" + entry.user +
                                         "', who is not listed above it");
            }
        }
    }

    std::vector<EffectivePermissions::State> here;
    here.reserve(walkers_.size());
    for (std::size_t i = 0; i < walkers_.size(); ++i) {
        here.push_back(walkers_[i].descend(parent[i], attribute));
    }

    // No lockout (D4.6): compared with the node itself where it was (a move: its effective permissions there, its
    // archived descendants included), or with the node at the same path, or, for a new node, with the node above it
    const auto existing = defs_.findAbsNode(path);
    std::vector<EffectivePermissions::State> old_here;
    bool held_p = parent_held_p;
    if (old_parent) {
        old_here.reserve(walkers_.size());
        for (std::size_t i = 0; i < walkers_.size(); ++i) {
            old_here.push_back(walkers_[i].descend((*old_parent)[i], attribute));
        }
        held_p = holds_p(old_here);
    }
    else if (existing) {
        held_p = holds_p(states_at(existing.get()));
    }
    if (held_p && !holds_p(here)) {
        throw std::runtime_error("The change would leave nobody able to change the Permissions attribute of " + path);
    }

    // The attribute replaced: that of the node at the same path, archived or not, below the target path
    const Permissions* before = existing ? existing->permissions() : nullptr;
    if (replaced_) {
        const auto it = replaced_index_.find(path);
        before        = it != replaced_index_.end() ? &(*replaced_)[it->second].second : nullptr;
    }
    if (before ? !(attribute && *attribute == *before) : attribute != nullptr) {
        records_.push_back(PermissionsChangeRecord{path, value_of(before), value_of(attribute), false});
    }

    std::vector<node_ptr> children;
    node.immediateChildren(children);
    for (const auto& child : children) {
        transfer(*child, path + "/" + child->name(), here, old_parent ? &old_here : nullptr, held_p, false);
    }
}

void PermissionsChange::validate_transfer(const Node& subtree, const Node* parent) {
    validate_transfer(subtree, parent, nullptr, false);
}

void PermissionsChange::validate_move(const Node& subtree, const Node* from, const Node* to) {
    validate_transfer(subtree, to, from, true);
}

void PermissionsChange::validate_transfer(const Node& subtree, const Node* parent, const Node* from, bool moved) {
    transferred_.clear();
    changed_.clear();
    candidate_.clear();
    walkers_.clear();
    if (const auto* server = server_after(); server) {
        for (const auto& entry : server->entries()) {
            walkers_.emplace_back(server, entry.user);
        }
    }

    if (!server_after()) {
        // Without a server attribute, no node of the definitions carries one (D2.20): nothing is replaced, nobody can
        // be locked out, and the only possible outcome is the refusal of an attribute brought
        if (const auto* found = first_carrying(subtree); found) {
            throw std::runtime_error("The Permissions attribute of " + destination_of(*found, subtree, parent) +
                                     " requires a server Permissions attribute");
        }
        return;
    }

    const std::string root = (parent ? parent->absNodePath() : std::string()) + "/" + subtree.name();

    // The attributes of the nodes the subtree replaces, archived ones included, by path
    replaced_.reset();
    replaced_index_.clear();
    if (const auto replaced = defs_.findAbsNode(root); replaced) {
        replaced_.emplace();
        (void)for_each_node(*replaced, [this](const Node& node) {
            if (node.permissions()) {
                replaced_index_.emplace(node.absNodePath(), replaced_->size());
                replaced_->emplace_back(node.absNodePath(), *node.permissions());
            }
        });
    }
    const auto states = states_at(parent);
    if (moved) {
        const auto old_states = states_at(from);
        transfer(subtree, root, states, &old_states, holds_p(states), parent == nullptr);
    }
    else {
        transfer(subtree, root, states, nullptr, holds_p(states), parent == nullptr);
    }

    // The attributes of the nodes replaced, archived ones included, that are absent from the subtree
    if (replaced_) {
        for (const auto& [path, attribute] : *replaced_) {
            if (transferred_.count(path) == 0) {
                records_.push_back(PermissionsChangeRecord{path, attribute, std::nullopt, false});
            }
        }
    }
    replaced_.reset();
    replaced_index_.clear();
}

void PermissionsChange::record_removal(const Node& subtree) {
    if (!server_after()) {
        // Without a server attribute, no node carries one (D2.20); the archives are not read
        return;
    }
    for_each_node(subtree, [this](const Node& node) {
        if (node.permissions()) {
            records_.push_back(
                PermissionsChangeRecord{node.absNodePath(), value_of(node.permissions()), std::nullopt, false});
        }
    });
}

void PermissionsChange::prepare_restore(const std::vector<node_ptr>& archived,
                                        const Node& container,
                                        bool requested) const {
    if (server_after()) {
        return;
    }
    // Without a server attribute, node attributes are not allowed (D2.20, D3.6)
    std::vector<Node*> carrying;
    std::function<void(Node&)> collect = [&](Node& node) {
        if (node.permissions()) {
            carrying.push_back(&node);
        }
        std::vector<node_ptr> children;
        node.immediateChildren(children);
        for (const auto& child : children) {
            collect(*child);
        }
    };
    for (const auto& node : archived) {
        collect(*node);
    }
    if (carrying.empty()) {
        return;
    }
    std::string named;
    for (const auto* node : carrying) {
        named += (named.empty() ? "" : ", ") + node->absNodePath();
    }
    if (requested) {
        throw std::runtime_error(
            "The restore of " + container.absNodePath() +
            " brings Permissions attributes, which require a server Permissions attribute: " + named);
    }
    for (auto* node : carrying) {
        node->set_permissions(nullptr);
    }
    ecf::log(Log::WAR,
             "restore of " + container.absNodePath() +
                 ": Permissions attributes removed, the server having no server Permissions attribute: " + named);
}

void PermissionsChange::complete_restore(Node& container) {
    const auto* server = server_after();
    if (!server) {
        return;
    }
    records_.clear();
    candidate_.clear();
    walkers_.clear();
    for (const auto& entry : server->entries()) {
        walkers_.emplace_back(server, entry.user);
    }

    // The entries of users not listed above are removed, as in the cascade (D3.5, D2.17)
    const auto states = states_at(&container);
    std::vector<node_ptr> children;
    container.immediateChildren(children);
    for (const auto& child : children) {
        cascade(*child, states);
    }
    std::function<void(Node&)> apply = [&](Node& node) {
        if (const auto it = candidate_.find(&node); it != candidate_.end()) {
            const auto before = value_of(node.permissions());
            node.set_permissions(it->second ? &*it->second : nullptr);
            write_record(PermissionsChangeRecord{node.absNodePath(), before, it->second, true},
                         "restore of " + container.absNodePath());
        }
        std::vector<node_ptr> below;
        node.immediateChildren(below);
        for (const auto& child : below) {
            apply(*child);
        }
    };
    for (const auto& child : children) {
        apply(*child);
    }
    candidate_.clear();

    // A lockout is accepted, with a warning naming the topmost nodes concerned (D3.5)
    std::function<void(const Node&, const std::vector<EffectivePermissions::State>&, bool)> warn =
        [&](const Node& node, const std::vector<EffectivePermissions::State>& parent, bool parent_held_p) {
            std::vector<EffectivePermissions::State> here;
            here.reserve(walkers_.size());
            for (std::size_t i = 0; i < walkers_.size(); ++i) {
                here.push_back(walkers_[i].descend(parent[i], node.permissions()));
            }
            const bool held_p = holds_p(here);
            if (parent_held_p && !held_p) {
                ecf::log(Log::WAR,
                         "restore of " + container.absNodePath() +
                             ": nobody is able to change the Permissions attribute of " + node.absNodePath() +
                             "; a sticky server entry holding p can change it");
            }
            std::vector<node_ptr> below;
            node.immediateChildren(below);
            for (const auto& child : below) {
                warn(*child, here, held_p);
            }
        };
    const bool container_held_p = holds_p(states);
    for (const auto& child : children) {
        warn(*child, states, container_held_p);
    }
}

void PermissionsChange::check_node(const Node& node,
                                   const std::vector<EffectivePermissions::State>& parent,
                                   bool suite) const {
    const auto* attribute = node.permissions();
    if (attribute) {
        if (!server_after()) {
            throw InvalidPermissions(node.absNodePath(),
                                     "The Permissions attribute of " + node.absNodePath() +
                                         " requires a server Permissions attribute");
        }
        for (const auto& entry : attribute->entries()) {
            if (!listed(entry.user, suite, parent)) {
                throw InvalidPermissions(node.absNodePath(),
                                         "The Permissions attribute of " + node.absNodePath() + " names the user '" +
                                             entry.user + "', who is not listed above it");
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
        check_node(*child, here, false);
    }
}

void PermissionsChange::check_definitions(const Permissions* server) {
    if (server && !defs_.server_state().permissions()) {
        server_ = *server;
    }
    if (!server_after()) {
        // Node attributes require a server attribute (D2.20): the first node carrying one is at fault
        for (const auto& suite : defs_.suites()) {
            if (const auto* found = first_carrying(*suite); found) {
                throw InvalidPermissions(found->absNodePath(),
                                         "The Permissions attribute of " + found->absNodePath() +
                                             " requires a server Permissions attribute");
            }
        }
        return;
    }
    walkers_.clear();
    if (const auto* in_force = server_after(); in_force) {
        for (const auto& entry : in_force->entries()) {
            walkers_.emplace_back(in_force, entry.user);
        }
    }
    const auto states = states_at(nullptr);
    for (const auto& suite : defs_.suites()) {
        check_node(*suite, states, true);
    }
    server_.reset();
}

void PermissionsChange::log(const std::string& requester) const {
    for (const auto& record : records_) {
        write_record(record, requester);
    }
}

} // namespace ecf
