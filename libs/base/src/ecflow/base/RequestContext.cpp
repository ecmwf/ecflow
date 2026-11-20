// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/RequestContext.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"

namespace ecf {

namespace {

// The visibilities computed for the users of the definitions of a server, valid until the next modify change
struct VisibilityCache
{
    std::weak_ptr<Defs> defs;
    std::optional<Permissions> server_attribute;
    unsigned int modify_change_no{0};
    std::uint64_t structure_generation{0};
    std::unordered_map<std::string, std::shared_ptr<const Visibility>> by_user;
};

VisibilityCache& visibility_cache() {
    static VisibilityCache cache;
    return cache;
}

bool same_owner(const std::weak_ptr<Defs>& a, const std::shared_ptr<Defs>& b) {
    return !a.expired() && !a.owner_before(b) && !b.owner_before(a);
}

} // namespace

RequestContext::RequestContext(const Identity& identity, AbstractServer& server)
    : identity_{identity},
      server_{server} {
}

Roles RequestContext::roles_at(const std::string& path) const {
    const auto defs = server_.defs();
    if (!defs) {
        throw std::logic_error("RequestContext::roles_at: the server holds no definitions");
    }
    const EffectivePermissions walker(*defs, identity_.username().value());
    if (!walker.applies()) {
        throw std::logic_error("RequestContext::roles_at: Permissions do not apply");
    }

    // The node addressed, without an attribute name, or its closest existing ancestor
    std::string candidate = path;
    if (const auto colon = candidate.find(':', candidate.rfind('/') == std::string::npos ? 0 : candidate.rfind('/'));
        colon != std::string::npos) {
        candidate.erase(colon);
    }
    node_ptr node;
    while (!candidate.empty() && candidate != "/" && !(node = defs->findAbsNode(candidate))) {
        const auto slash = candidate.rfind('/');
        candidate        = (slash == std::string::npos || slash == 0) ? std::string{} : candidate.substr(0, slash);
    }
    if (!node) {
        return walker.roles(walker.at_server());
    }

    // Descend from the server to the node
    std::vector<const Node*> chain;
    for (const Node* n = node.get(); n != nullptr; n = n->parent()) {
        chain.push_back(n);
    }
    auto state = walker.at_server();
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        state = walker.descend(state, **it);
    }
    return walker.roles(state);
}

std::shared_ptr<const Visibility> RequestContext::visibility() const {
    const auto defs = server_.defs();
    if (!defs) {
        throw std::logic_error("RequestContext::visibility: the server holds no definitions");
    }
    auto& cache                  = visibility_cache();
    const auto* server_attribute = defs->server_state().permissions();
    const bool same_attribute    = server_attribute
                                       ? (cache.server_attribute && *cache.server_attribute == *server_attribute)
                                       : !cache.server_attribute;
    if (!same_owner(cache.defs, defs) || !same_attribute || cache.modify_change_no != Ecf::modify_change_no() ||
        cache.structure_generation != Visibility::structure_generation()) {
        cache.defs = defs;
        cache.server_attribute =
            server_attribute ? std::optional<Permissions>(*server_attribute) : std::optional<Permissions>();
        cache.modify_change_no     = Ecf::modify_change_no();
        cache.structure_generation = Visibility::structure_generation();
        cache.by_user.clear();
    }
    // Without Permissions every user reads every node: one visibility serves them all
    auto& visibility = cache.by_user[server_attribute ? identity_.username().value() : std::string{}];
    if (!visibility) {
        visibility = std::make_shared<const Visibility>(*defs, identity_.username().value());
    }
    return visibility;
}

const EffectivePermissions& RequestContext::permissions() const {
    if (!permissions_) {
        permissions_.emplace(*server_.defs(), identity_.username().value());
    }
    return *permissions_;
}

} // namespace ecf
