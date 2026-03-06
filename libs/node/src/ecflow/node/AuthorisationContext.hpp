// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#ifndef ecflow_node_Ctx_HPP
#define ecflow_node_Ctx_HPP

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/node/NodeFwd.hpp"
#include "ecflow/server/AuthorisationService.hpp"

namespace ecf {

class AuthorisationContext {
public:
    using path_t  = std::string;
    using paths_t = std::vector<std::string>;

    [[nodiscard]] virtual bool allows(const path_t& path, Allowed required) const   = 0;
    [[nodiscard]] virtual bool allows(const paths_t& paths, Allowed required) const = 0;
};

class UnrestrictedAuthorisationContext : public AuthorisationContext {
public:
    using path_t  = std::string;
    using paths_t = std::vector<std::string>;

    [[nodiscard]] bool allows(const path_t& path, Allowed required) const override { return true; }
    [[nodiscard]] bool allows(const paths_t& paths, Allowed required) const override { return true; }
};

class ServiceAuthorisationContext : public AuthorisationContext {
public:
    using path_t  = std::string;
    using paths_t = std::vector<std::string>;

    ServiceAuthorisationContext(const Identity& identity, const AbstractServer& server)
        : identity_{identity},
          defs_{*server.defs()},
          service_{server.authorisation()} {}

    ServiceAuthorisationContext(const Identity& identity, const Defs& defs, const AuthorisationService& service)
        : identity_{identity},
          defs_{defs},
          service_{service} {}

    [[nodiscard]] bool allows(const path_t& path, Allowed required) const override;
    [[nodiscard]] bool allows(const paths_t& paths, Allowed required) const override;

private:
    const Identity& identity_;
    const Defs& defs_;
    const AuthorisationService& service_;
};

} // namespace ecf

#endif /* ecflow_node_Ctx_HPP */
