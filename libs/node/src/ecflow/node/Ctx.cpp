/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/Ctx.hpp"

#include "ecflow/node/Node.hpp"

namespace ecf {

bool Ctx::allows(const path_t& path, Allowed required) const {
    auto x = service_.allows(identity_, defs_, path, required);
    std::cout << "Ctx :: Checking path=" << path << " required=" << allowed_to_string(required) << " found=" << x << std::endl;
    return x;
}

bool Ctx::allows(const paths_t& paths, Allowed required) const {
    for (const auto& path : paths) {
        if (!allows(path, required)) {
            return false;
        }
    }
    return true;
}

} // namespace ecf
