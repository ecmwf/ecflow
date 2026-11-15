// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/Visibility.hpp"

#include <atomic>
#include <stdexcept>
#include <vector>

#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/NodeState.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"

namespace ecf {

namespace {

// The key of the content every user reads when Permissions do not apply, and of the empty content
constexpr const char* everything = "*";
constexpr const char* nothing    = "-";

std::atomic<std::uint64_t> generation{0};

} // namespace

Visibility::Visibility(const Defs& defs, const std::string& user) {
    const EffectivePermissions walker(defs, user);
    applies_ = walker.applies();
    if (!applies_) {
        key_ = everything;
        return;
    }
    admitted_ = walker.admitted();
    if (!admitted_) {
        key_ = nothing;
        return;
    }
    // The key lists the topmost unreadable nodes, in the order of the walk; it is empty when every node is readable
    key_              = "+";
    const auto server = walker.at_server();
    for (const auto& suite : defs.suites()) {
        visit(walker, server, *suite);
    }
}

void Visibility::visit(const EffectivePermissions& walker,
                       const EffectivePermissions::State& parent,
                       const Node& node) {
    const auto state = walker.descend(parent, node);
    if (!walker.roles(state).has(Role::Read)) {
        unreadable_.insert(&node);
        for (const Node* ancestor = node.parent(); ancestor != nullptr; ancestor = ancestor->parent()) {
            if (!partly_hidden_.insert(ancestor).second) {
                break; // its ancestors are recorded already
            }
        }
        key_ += node.absNodePath();
        key_ += '\n';
        return;
    }
    std::vector<node_ptr> children;
    node.immediateChildren(children);
    for (const auto& child : children) {
        visit(walker, state, *child);
    }
}

void Visibility::structure_changed() {
    ++generation;
}

std::uint64_t Visibility::structure_generation() {
    return generation.load();
}

bool Visibility::stops_at(const Node& node) const {
    if (!applies_) {
        return false;
    }
    if (!admitted_) {
        return true;
    }
    return unreadable_.count(&node) != 0;
}

std::vector<node_ptr> Visibility::project(const std::vector<node_ptr>& children) const {
    if (!applies_) {
        return children;
    }
    std::vector<node_ptr> result;
    result.reserve(children.size());
    for (const auto& child : children) {
        if (stops_at(*child)) {
            continue;
        }
        if (partly_hidden_.count(child.get()) == 0) {
            result.push_back(child);
            continue;
        }
        node_ptr copy;
        if (const auto* family = child->isFamily(); family) {
            copy = std::make_shared<Family>(*family);
        }
        else if (const auto* task = child->isTask(); task) {
            copy = std::make_shared<Task>(*task);
        }
        else {
            throw std::logic_error("Visibility::project: unexpected node with hidden nodes below it: " +
                                   child->absNodePath());
        }
        prune(*child, *copy);
        result.push_back(copy);
    }
    return result;
}

std::vector<alias_ptr> Visibility::project(const std::vector<alias_ptr>& aliases) const {
    std::vector<alias_ptr> result;
    result.reserve(aliases.size());
    for (const auto& alias : aliases) {
        if (!stops_at(*alias)) {
            result.push_back(alias);
        }
    }
    return result;
}

void Visibility::prune(const Node& original, Node& copy) const {
    // The copy holds the children of the original, copied in the same order
    if (const auto* task = original.isTask(); task) {
        std::vector<bool> keep;
        for (const auto& alias : task->aliases()) {
            keep.push_back(!stops_at(*alias));
        }
        copy.isTask()->retain_aliases_of_copy(keep);
        return;
    }
    const auto* container = original.isNodeContainer();
    auto* copied          = copy.isNodeContainer();
    if (!container || !copied || container->children().size() != copied->children().size()) {
        throw std::logic_error("Visibility::prune: the copy does not match " + original.absNodePath());
    }
    std::vector<bool> keep;
    for (std::size_t i = 0; i < container->children().size(); ++i) {
        const auto& child = *container->children()[i];
        keep.push_back(!stops_at(child));
        if (keep.back() && partly_hidden_.count(&child) != 0) {
            prune(child, *copied->children()[i]);
        }
    }
    copied->retain_children_of_copy(keep);
}

NState::State Visibility::root_state(const Defs& defs, const std::vector<suite_ptr>& suites) const {
    if (!applies_) {
        return defs.state();
    }
    std::vector<suite_ptr> readable;
    for (const auto& suite : suites) {
        if (!stops_at(*suite)) {
            readable.push_back(suite);
        }
    }
    return ecf::theComputedNodeState(readable, true /* immediate children only */);
}

bool Visibility::readable(const Node& node) const {
    if (!applies_) {
        return true;
    }
    if (!admitted_) {
        return false;
    }
    for (const Node* n = &node; n != nullptr; n = n->parent()) {
        if (unreadable_.count(n) != 0) {
            return false;
        }
    }
    return true;
}

} // namespace ecf
