// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

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

ActivePermissions permissions_at(const Identity& identity, const Defs& defs, const std::string& path) {
    ActivePermissions active;

    struct Visitor
    {
        Visitor(ActivePermissions& collected)
            : collected_{collected} {}

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

    return active;
}

} // namespace ecf
