/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/permissions/ActivePermissions.hpp"

#include "ecflow/core/Overload.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/NodePathAlgorithms.hpp"

namespace ecf {

void ActivePermissions::bootstrap(const Permissions& p) {
    permissions_ = p;
}

void ActivePermissions::combine_supersede(const Permissions& p) {
    if (!is_none()) {
        permissions_ = Permissions::combine_supersede(permissions_, p);
    }
}

void ActivePermissions::combine_override(const Permissions& p) {
    if (!is_none()) {
        permissions_ = Permissions::combine_override(permissions_, p);
    }
}

ActivePermissions
permissions_at(const Defs& defs, const std::string& path, const std::variant<Unrestricted, Rules>& permissions) {
    ActivePermissions active;

    std::visit(overload{[&active](const Unrestricted&) {
                            // when no rules are loaded, we allow everything...
                            // Dangerous, but backward compatible!
                            active = ActivePermissions::make_empty();
                        },
                        [&defs, &active, &path](const Rules& rules) {
                            struct Visitor
                            {
                                Visitor(ActivePermissions& collected) : collected_{collected} {}

                                void handle(const Defs& defs) {
                                    auto p = defs.server_state().permissions();

                                    // At server level, we only care about the server permissions
                                    collected_.bootstrap(p);
                                }
                                void handle(const Node& n) {
                                    auto p = n.permissions();

                                    if (auto s = dynamic_cast<const Suite*>(&n); s) {
                                        // At node level, if the node is a Suite we bootstrap the node permissions
                                        collected_.combine_supersede(p);
                                    }
                                    else {
                                        // ... otherwise, we combine the node permissions
                                        //  -- in practice, this combination only restricts node permissions;
                                        //     for example, a user can't be allowed to read/write/execute a
                                        //     specific node if he can't do it at a higher node level
                                        collected_.combine_override(p);
                                    }
                                }

                                void not_found() { /* do nothing */ }

                            private:
                                ActivePermissions& collected_;
                            };

                            auto p = Path::make(path).value();
                            auto v = Visitor{active};

                            ecf::visit(defs, p, v);
                        }

               },
               permissions);

    return active;
}

} // namespace ecf
