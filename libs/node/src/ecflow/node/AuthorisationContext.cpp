// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/AuthorisationContext.hpp"

#include "ecflow/node/Node.hpp"

namespace ecf {

bool ServiceAuthorisationContext::allows(const path_t& path, Allowed required) const {
    return service_.allows(identity_, defs_, path, required);
}

bool ServiceAuthorisationContext::content_varies_by_identity() const {
    return service_.content_varies_by_identity();
}

bool ServiceAuthorisationContext::allows(const paths_t& paths, Allowed required) const {
    for (const auto& path : paths) {
        if (!allows(path, required)) {
            return false;
        }
    }
    return true;
}

} // namespace ecf
