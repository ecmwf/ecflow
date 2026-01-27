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
#include "ecflow/core/WhiteListFile.hpp"

namespace ecf {

///
/// NodeRules represent permissions derived from the nodes themselves (based on ECF_PERMISSIONS permissions)
///
struct NodeRules
{
};

///
/// WhiteListRules represent permissions define by a white list file
///
struct WhiteListRules
{
    WhiteListRules(const WhiteListFile& file) : file_(file) {}
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
    Impl() : permissions_(Unrestricted{}) {}
    explicit Impl(NodeRules&& rules) : permissions_(std::move(rules)) {}
    explicit Impl(WhiteListRules&& rules) : permissions_(std::move(rules)) {}

    std::variant<Unrestricted, NodeRules, WhiteListRules> permissions_;
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
                                allowed = rules.file_.verify_read_access(identity.username(), paths);
                            }
                            else if (permission == "write") {
                                allowed = rules.file_.verify_write_access(identity.username(), paths);
                            }
                            else {
                                allowed = false;
                            }
                        },
                        [&server, &identity, &paths, &allowed](const NodeRules& rules) {
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
                                else if (v.permissions.allows(identity.username())) {
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
    return result_t::success(AuthorisationService(std::make_unique<Impl>(NodeRules{})));
}

AuthorisationService::result_t AuthorisationService::load_permissions_from_whitelist(const WhiteListFile& whitelist) {
    return result_t::success(AuthorisationService(std::make_unique<Impl>(WhiteListRules{whitelist})));
}

} // namespace ecf
