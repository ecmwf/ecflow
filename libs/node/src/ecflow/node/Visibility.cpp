// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/Visibility.hpp"

#include <atomic>
#include <vector>

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/Suite.hpp"

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
