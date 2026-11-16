// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/python/PermissionsUtil.hpp"

#include <stdexcept>
#include <vector>

#include "ecflow/base/cts/user/PermissionsCmd.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/PermissionsChange.hpp"
#include "ecflow/node/Suite.hpp"

ecf::Permissions PermissionsUtil::from(const py::handle& value) {
    if (py::isinstance<ecf::Permissions>(value)) {
        return value.cast<ecf::Permissions>();
    }
    if (py::isinstance<py::str>(value)) {
        return PermissionsCmd::parse_value(value.cast<std::string>());
    }
    if (py::isinstance<py::list>(value) || py::isinstance<py::tuple>(value)) {
        std::vector<ecf::Permissions::Entry> entries;
        for (const auto& item : value) {
            if (!py::isinstance<py::str>(item)) {
                throw py::type_error("Permissions: each entry must be a string <user>:<letters>");
            }
            const auto parsed = ecf::Permissions::parse_cli(item.cast<std::string>());
            if (parsed.entries().size() != 1) {
                throw std::runtime_error("Invalid Permissions: each item of the list holds one entry, found '" +
                                         item.cast<std::string>() + "'");
            }
            entries.push_back(parsed.entries().front());
        }
        return ecf::Permissions(std::move(entries));
    }
    throw py::type_error("Permissions: expected a Permissions object, a string or a list of strings");
}

std::string PermissionsUtil::to_string(const ecf::Permissions::Entry& entry) {
    return ecf::Permissions({entry}).to_cli();
}

void PermissionsUtil::add_to(const ecf::Permissions& permissions, const py::iterable& nodes, bool replace) {
    // Check every item first, so that nothing changes when one is refused
    std::vector<node_ptr> targets;
    for (const auto& item : nodes) {
        if (!py::isinstance<Node>(item)) {
            throw py::type_error("Permissions.add_to: every item must be a node, found " +
                                 py::str(py::type::of(item)).cast<std::string>());
        }
        auto node = item.cast<node_ptr>();
        if (!replace && node->permissions()) {
            throw std::runtime_error("Permissions.add_to: the node " + node->absNodePath() +
                                     " already has a Permissions attribute; nothing was changed");
        }
        targets.push_back(std::move(node));
    }
    for (const auto& node : targets) {
        node->set_permissions(&permissions);
    }
}

std::string PermissionsUtil::check(const Defs& defs, const ecf::Permissions& server) {
    std::string errors;
    if (defs.server_state().permissions()) {
        errors += "the definitions carry a server Permissions attribute, which is set only by the server "
                  "configuration file and --permissions update /\n";
    }
    for (const auto& suite : defs.suites()) {
        // A server holding no suite, with the declared server attribute, as a load into it would find
        auto target = Defs::create();
        target->server_state().set_permissions(server);
        try {
            ecf::PermissionsChange(*target).validate_transfer(*suite, nullptr);
        }
        catch (const std::exception& e) {
            errors += "suite " + suite->absNodePath() + ": " + e.what() + "\n";
        }
    }
    return errors;
}
