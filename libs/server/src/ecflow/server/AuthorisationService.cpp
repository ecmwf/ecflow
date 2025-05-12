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

ActivePermissions AuthorisationService::permissions_at(const AbstractServer& server, const path_t& path) const {
    assert(good()); // It is up to the caller to check has been properly configured

    ActivePermissions active;

    std::visit(overload{[&active](const Unrestricted&) {
                            // when no rules are loaded, we allow everything...
                            // Dangerous, but backward compatible!
                            active = ActivePermissions::make_empty();
                        },
                        [&server, &active, &path](const Rules& rules) {
                            struct Visitor
                            {
                                Visitor(ActivePermissions& collected) : collected_{collected} {}

                                void handle(const Defs& defs) {
                                    auto p = defs.server_state().permissions();

                                    // At server level, we only care about the server permissions
                                    collected_.bootstrap_server_permission(p);
                                }
                                void handle(const Node& n) {
                                    auto p = n.permissions();

                                    if (auto s = dynamic_cast<const Suite*>(&n); s) {
                                        // At node level, if the node is a Suite we bootstrap the node permissions
                                        collected_.bootstrap_node_permission(p);
                                    }
                                    else {
                                        // ... otherwise, we combine the node permissions
                                        //  -- in practice, this combination only restricts node permissions;
                                        //     for example, a user can't be allowed to read/write/execute a
                                        //     specific node if he can't do it at a higher node level
                                        collected_.combine_node_permission(p);
                                    }
                                }

                                void not_found() { /* do nothing */ }

                            private:
                                ActivePermissions& collected_;
                            };

                            auto d = server.defs();
                            auto p = Path::make(path).value();
                            auto v = Visitor{active};

                            ecf::visit(*d, p, v);
                        }

               },
               impl_->permissions_);

    return active;
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

    for (auto&& path : paths) {
        if (auto&& perms = permissions_at(server, path); !perms.allows(identity.username(), required)) {
            return false;
        }
    }

    return true;
}

AuthorisationService::result_t AuthorisationService::load_permissions_from_nodes() {
    return result_t::success(AuthorisationService(std::make_unique<Impl>(Rules{})));
}

} // namespace ecf
