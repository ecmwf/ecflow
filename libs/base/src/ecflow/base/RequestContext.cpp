// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/RequestContext.hpp"

#include <stdexcept>
#include <vector>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"

namespace ecf {

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

const EffectivePermissions& RequestContext::permissions() const {
    if (!permissions_) {
        permissions_.emplace(*server_.defs(), identity_.username().value());
    }
    return *permissions_;
}

} // namespace ecf
