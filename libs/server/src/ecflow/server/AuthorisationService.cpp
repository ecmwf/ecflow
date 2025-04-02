/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/server/AuthorisationService.hpp"

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/Algorithms.hpp"
#include "ecflow/core/Overload.hpp"

namespace ecf {

struct Rules
{
};

struct Unrestricted
{
};

struct AuthorisationService::Impl
{
    Impl() : permissions_(Unrestricted{}) {}
    explicit Impl(Rules&& rules) : permissions_(std::move(rules)) {}

    std::variant<Unrestricted, Rules> permissions_;
};

AuthorisationService::AuthorisationService() = default;

AuthorisationService::AuthorisationService(std::unique_ptr<Impl>&& impl) : impl_{std::move(impl)} {
}

AuthorisationService::AuthorisationService(AuthorisationService&& rhs) noexcept : impl_{std::move(rhs.impl_)} {
}

AuthorisationService::~AuthorisationService() = default;

AuthorisationService& AuthorisationService::operator=(AuthorisationService&& rhs) noexcept {
    if (this != &rhs) {
        impl_ = std::move(rhs.impl_);
    }
    return *this;
}

bool AuthorisationService::good() const {
    return impl_ != nullptr;
}

bool AuthorisationService::allows(const Identity& identity, const AbstractServer& server, Allowed required) const {
    return allows(identity, server, paths_t{ROOT}, required);
}

bool AuthorisationService::allows(const Identity& identity,
                                  const AbstractServer& server,
                                  const path_t& path,
                                  Allowed required) const {
    return allows(identity, server, paths_t{path}, required);
}

bool AuthorisationService::allows(const Identity& identity,
                                  const AbstractServer& server,
                                  const paths_t& paths,
                                  Allowed required) const {
    if (!good()) {
        // When no rules are loaded, we allow everything...
        // Dangerous, but backward compatible!
        return true;
    }

    bool allowed = false;
    std::visit(overload{[&allowed](const Unrestricted&) {
                            // when no rules are loaded, we allow everything...
                            // Dangerous, but backward compatible!
                            allowed = true;
                        },
                        [&server, &identity, &paths, &required, &allowed](const Rules& rules) {
                            for (auto&& path : paths) {

                                struct Visitor
                                {
                                    void handle(const Defs& defs) {
                                        auto p      = defs.server_state().permissions();
                                        permissions = p.is_empty() ? permissions : p;
                                    }
                                    void handle(const Node& s) {
                                        auto p      = s.permissions();
                                        permissions = p.is_empty() ? permissions : p;
                                    }

                                    void not_found() { permissions = Permissions::make_empty(); }

                                    Permissions permissions = Permissions::make_empty();
                                };

                                auto d = server.defs();
                                auto p = Path::make(path).value();
                                auto v = Visitor{};

                                ecf::visit(*d, p, v);

                                if (v.permissions.is_empty()) {
                                    allowed = true;
                                }
                                else if (v.permissions.allows(identity.username(), required)) {
                                    allowed = true;
                                }
                                else {
                                    allowed = false;
                                    break;
                                }
                            }
                        }},
               impl_->permissions_);

    return allowed;
}

AuthorisationService::result_t AuthorisationService::load_permissions_from_nodes() {
    return result_t::success(AuthorisationService(std::make_unique<Impl>(Rules{})));
}

} // namespace ecf
