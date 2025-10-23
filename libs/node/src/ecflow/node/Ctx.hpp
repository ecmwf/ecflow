/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_Ctx_HPP
#define ecflow_node_Ctx_HPP

#include "ecflow/node/NodeFwd.hpp"
#include "ecflow/server/AuthorisationService.hpp"

namespace ecf {

class Ctx {
public:
    using path_t = std::string;
    using paths_t = std::vector<std::string>;

    Ctx(const Identity& identity, const Defs& defs)
        : identity_{identity},
          defs_{defs},
          service_{AuthorisationService::make_for(defs_)} {}

    [[nodiscard]] bool allows(const path_t& path, Allowed required) const;
    [[nodiscard]] bool allows(const paths_t& paths, Allowed required) const;

private:
    const Identity& identity_;
    const Defs& defs_;
    AuthorisationService service_;
};

} // namespace ecf

#endif /* ecflow_node_Ctx_HPP */
