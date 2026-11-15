// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/RequestContext.hpp"

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/node/Defs.hpp"

namespace ecf {

RequestContext::RequestContext(const Identity& identity, AbstractServer& server)
    : identity_{identity},
      server_{server} {
}

const EffectivePermissions& RequestContext::permissions() const {
    if (!permissions_) {
        permissions_.emplace(*server_.defs(), identity_.username().value());
    }
    return *permissions_;
}

} // namespace ecf
