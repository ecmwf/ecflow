// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/EffectivePermissions.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"

namespace ecf {

EffectivePermissions::EffectivePermissions(const Defs& defs, std::string user)
    : EffectivePermissions(defs.server_state().permissions(), std::move(user)) {
}

EffectivePermissions::EffectivePermissions(const Permissions* server, std::string user)
    : user_{std::move(user)} {
    applies_ = server != nullptr;
    if (!applies_) {
        return;
    }
    const auto* entry = server->find(user_);
    admitted_         = entry != nullptr && entry->roles.has(Role::Read);
    if (admitted_) {
        server_roles_ = entry->roles;
        if (entry->sticky) {
            sticky_server_roles_ = entry->roles;
        }
    }
}

EffectivePermissions::State EffectivePermissions::at_server() const {
    return State{server_roles_, false, true};
}

EffectivePermissions::State EffectivePermissions::descend(const State& parent, const Node& node) const {
    return descend(parent, node.permissions());
}

EffectivePermissions::State EffectivePermissions::descend(const State& parent, const Permissions* attribute) const {
    if (!admitted_ || parent.locked) {
        return State{parent.roles, parent.locked, false};
    }
    if (!attribute) {
        // A suite without attribute takes the server entry; any other node inherits
        return State{parent.roles, false, false};
    }
    const auto* entry = attribute->find(user_);
    if (!entry) {
        return State{Roles{}, false, false};
    }
    // The suite replaces the server entry; below it, rights are intersected
    const Roles roles = parent.server ? entry->roles : (parent.roles & entry->roles);
    return State{roles, entry->sticky, false};
}

Roles EffectivePermissions::roles(const State& state) const {
    if (!admitted_) {
        return Roles{};
    }
    return state.roles | sticky_server_roles_;
}

std::optional<Roles> effective_permissions(const Defs& defs, const std::string& path, const std::string& user) {
    const EffectivePermissions walker(defs, user);
    if (!walker.applies()) {
        return std::nullopt;
    }
    auto state = walker.at_server();
    if (path == "/") {
        return walker.roles(state);
    }

    const auto node = defs.findAbsNode(path);
    if (!node) {
        throw std::invalid_argument("effective_permissions: no node at " + path);
    }
    std::vector<const Node*> chain;
    for (const Node* n = node.get(); n; n = n->parent()) {
        chain.push_back(n);
    }
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        state = walker.descend(state, **it);
    }
    return walker.roles(state);
}

std::optional<std::vector<std::pair<std::string, Roles>>> effective_permissions_of_every_user(const Defs& defs,
                                                                                              const std::string& path) {
    const auto* server = defs.server_state().permissions();
    if (!server) {
        return std::nullopt;
    }
    std::vector<std::pair<std::string, Roles>> result;
    for (const auto& entry : server->entries()) {
        if (auto roles = effective_permissions(defs, path, entry.user); roles && !roles->empty()) {
            result.emplace_back(entry.user, *roles);
        }
    }
    return result;
}

} // namespace ecf
