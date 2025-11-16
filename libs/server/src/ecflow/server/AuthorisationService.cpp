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
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/permissions/ActivePermissions.hpp"

namespace ecf {

struct AuthorisationService::Impl
{
    explicit Impl(Unrestricted&& unrestricted) : permissions_(std::move(unrestricted)) {}
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

ActivePermissions AuthorisationService::permissions_at(const Defs& defs, const path_t& path) const {
    assert(good()); // It is up to the caller to check has been properly configured
    return ecf::permissions_at(defs, path, impl_->permissions_);
}

bool AuthorisationService::allows(const Identity& identity, const Defs& defs, Allowed required) const {
    return allows(identity, defs, paths_t{ROOT}, required);
}

bool AuthorisationService::allows(const Identity& identity,
                                  const Defs& defs,
                                  const path_t& path,
                                  Allowed required) const {
    return allows(identity, defs, paths_t{path}, required);
}

bool AuthorisationService::allows(const Identity& identity,
                                  const Defs& defs,
                                  const paths_t& paths,
                                  Allowed required) const {
    if (!good()) {
        // When no rules are loaded, we allow everything...
        // Dangerous, but backward compatible!
        return true;
    }

    for (auto&& path : paths) {
        if (auto&& perms = permissions_at(defs, path); !perms.allows(identity.username(), required)) {
            return false;
        }
    }

    return true;
}

AuthorisationService AuthorisationService::make_for(const Defs& defs) {
    if (defs.server_state().permissions().is_empty()) {
        return AuthorisationService(std::make_unique<Impl>(Unrestricted{}));
    }
    else {
        return AuthorisationService(std::make_unique<Impl>(Rules{}));
    }
}

} // namespace ecf
