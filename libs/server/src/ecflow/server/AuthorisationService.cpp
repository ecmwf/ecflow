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
#include "ecflow/core/Overload.hpp"
#include "ecflow/core/WhiteListFile.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/permissions/ActivePermissions.hpp"

namespace ecf {

struct AuthorisationService::Impl
{
    explicit Impl(UnrestrictedRules&& rules)
        : rules_(std::move(rules)) {}
    explicit Impl(NodeRules&& rules)
        : rules_(std::move(rules)) {}
    explicit Impl(WhiteListRules&& rules)
        : rules_(std::move(rules)) {}

    std::variant<UnrestrictedRules, WhiteListRules, NodeRules> rules_;
};

AuthorisationService::AuthorisationService() = default;

AuthorisationService::AuthorisationService(std::unique_ptr<Impl>&& impl)
    : impl_{std::move(impl)} {
}

AuthorisationService::AuthorisationService(AuthorisationService&& rhs) noexcept
    : impl_{std::move(rhs.impl_)} {
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

    bool allowed = false;
    std::visit(overload{[&allowed](const UnrestrictedRules&) { allowed = true; },
                        [&allowed, &identity, &paths, &required](const WhiteListRules& rules) {
                            // Apply white list rules
                            if (contains(required, Allowed::READ)) {
                                allowed = rules.file_.verify_read_access(identity.username().value(), paths);
                            }
                            else if (contains(required, Allowed::WRITE) || contains(required, Allowed::EXECUTE)) {
                                allowed = rules.file_.verify_write_access(identity.username().value(), paths);
                            }
                            else {
                                allowed = false;
                            }
                        },
                        [&allowed, &defs, &identity, &paths, &required](const NodeRules& rules) {
                            for (auto&& path : paths) {
                                ActivePermissions active = permissions_at(identity, defs, path);
                                allowed                  = active.allows(identity.username(), required);
                                if (!allowed) {
                                    break;
                                }
                            }
                        }},
               impl_->rules_);

    return allowed;
}

AuthorisationService::result_t AuthorisationService::load_permissions_unrestricted() {
    return result_t::success(AuthorisationService(std::make_unique<Impl>(UnrestrictedRules{})));
}

AuthorisationService::result_t AuthorisationService::load_permissions_from_nodes() {
    return result_t::success(AuthorisationService(std::make_unique<Impl>(NodeRules{})));
}

AuthorisationService::result_t AuthorisationService::load_permissions_from_whitelist(const WhiteListFile& whitelist) {
    return result_t::success(AuthorisationService(std::make_unique<Impl>(WhiteListRules{whitelist})));
}

void AuthorisationService::init(const Permissions& permissions) {
    if (permissions.is_empty()) {
        impl_ = std::make_unique<AuthorisationService::Impl>(WhiteListRules{WhiteListFile{}});
    }
    else {
        impl_ = std::make_unique<AuthorisationService::Impl>(NodeRules{});
    }
}

} // namespace ecf
