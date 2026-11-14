// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/server/AuthorisationService.hpp"

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/core/Overload.hpp"
#include "ecflow/core/WhiteListFile.hpp"

namespace ecf {

///
/// WhiteListRules represent permissions define by a white list file
///
struct WhiteListRules
{
    WhiteListRules(const WhiteListFile& file)
        : file_(file) {}
    const WhiteListFile& file_;
};

///
/// Unrestricted represent no restrictions at all
///
struct Unrestricted
{
};

struct AuthorisationService::Impl
{
    Impl()
        : permissions_(Unrestricted{}) {}
    explicit Impl(WhiteListRules&& rules)
        : permissions_(std::move(rules)) {}

    std::variant<Unrestricted, WhiteListRules> permissions_;
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

bool AuthorisationService::allows(const Identity& identity,
                                  const AbstractServer& server,
                                  const std::string& permission) const {
    return allows(identity, server, paths_t{ROOT}, permission);
}

bool AuthorisationService::allows(const Identity& identity,
                                  const AbstractServer& server,
                                  const path_t& path,
                                  const std::string& permission) const {
    return allows(identity, server, paths_t{path}, permission);
}

bool AuthorisationService::allows(const Identity& identity,
                                  const AbstractServer& server,
                                  const paths_t& paths,
                                  const std::string& permission) const {
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
                        [&allowed, &identity, &paths, &permission](const WhiteListRules& rules) {
                            // Apply white list rules
                            if (permission == "read") {
                                allowed = rules.file_.verify_read_access(identity.username().value(), paths);
                            }
                            else if (permission == "write") {
                                allowed = rules.file_.verify_write_access(identity.username().value(), paths);
                            }
                            else {
                                allowed = false;
                            }
                        }},
               impl_->permissions_);

    return allowed;
}

AuthorisationService::result_t AuthorisationService::make_unrestricted() {
    return result_t::success(AuthorisationService(std::make_unique<Impl>()));
}

AuthorisationService::result_t AuthorisationService::load_permissions_from_whitelist(const WhiteListFile& whitelist) {
    return result_t::success(AuthorisationService(std::make_unique<Impl>(WhiteListRules{whitelist})));
}

} // namespace ecf
