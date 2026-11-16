// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/cts/user/PermissionsCmd.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/RequestContext.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/EffectivePermissions.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/PermissionsChange.hpp"
#include "ecflow/node/Visibility.hpp"

using namespace ecf;

namespace {

const std::array<const char*, 5> operations{"create", "read", "update", "delete", "effective"};

bool known(const std::string& operation) {
    for (const auto* candidate : operations) {
        if (operation == candidate) {
            return true;
        }
    }
    return false;
}

} // namespace

PermissionsCmd::PermissionsCmd(std::string operation, std::string path, std::string value, Format format)
    : operation_{std::move(operation)},
      path_{std::move(path)},
      value_{std::move(value)},
      format_{format} {
}

bool PermissionsCmd::changes() const {
    return operation_ == "create" || operation_ == "update" || operation_ == "delete";
}

Permissions PermissionsCmd::parse_value(const std::string& value) {
    std::string json_error;
    try {
        return Permissions::parse_json(value);
    }
    catch (const std::exception& e) {
        json_error = e.what();
    }
    try {
        return Permissions::parse_cli(value);
    }
    catch (const std::exception& e) {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first != std::string::npos && value[first] == '[') {
            throw std::runtime_error(std::string(e.what()) + "; as the JSON form: " + json_error);
        }
        throw;
    }
}

void PermissionsCmd::check_arguments(const std::string& operation, const std::string& path, const std::string& value) {
    if (!known(operation)) {
        throw std::runtime_error("--permissions: unknown operation '" + operation +
                                 "', expected create, read, update, delete or effective");
    }
    if (path.empty() || path[0] != '/') {
        throw std::runtime_error("--permissions: the path must be absolute, found '" + path + "'");
    }
    if ((operation == "create" || operation == "update") && value.empty()) {
        throw std::runtime_error("--permissions: " + operation + " requires a value");
    }
    if ((operation == "read" || operation == "delete") && !value.empty()) {
        throw std::runtime_error("--permissions: " + operation + " takes no value");
    }
    if (operation == "create" || operation == "update") {
        (void)parse_value(value);
    }
    if (operation == "effective" && !value.empty()) {
        // The user diagnosed follows the user name model (R2.8); empty stands for the requester
        if (std::string error; !is_valid_user_name(value, error)) {
            throw std::runtime_error("--permissions: effective: invalid user name: " + error);
        }
    }
}

std::string PermissionsCmd::print_as_string() const {
    return CtsApi::to_string(CtsApi::permissions(operation_, path_, value_, format_ == Format::Json));
}

void PermissionsCmd::print(std::string& os) const {
    user_cmd(os, print_as_string());
}

void PermissionsCmd::print_only(std::string& os) const {
    os += print_as_string();
}

bool PermissionsCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<PermissionsCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (operation_ != the_rhs->operation() || path_ != the_rhs->path() || value_ != the_rhs->value() ||
        format_ != the_rhs->format()) {
        return false;
    }
    return UserCmd::equals(rhs);
}

ecf::authentication_t PermissionsCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t PermissionsCmd::authorise(const ecf::RequestContext& context) const {
    return implementation::do_authorise(*this, context);
}

const char* PermissionsCmd::arg() {
    return CtsApi::permissionsArg();
}

void PermissionsCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(PermissionsCmd::arg(), boost::program_options::value<std::vector<std::string>>()->multitoken());
    desc.add_options()(CtsApi::permissions_format_arg(), boost::program_options::value<std::string>());
}

void PermissionsCmd::create(Cmd_ptr& cmd,
                            boost::program_options::variables_map& vm,
                            AbstractClientEnv* clientEnv) const {
    const auto args = vm[arg()].as<std::vector<std::string>>();
    if (clientEnv->debug()) {
        dumpVecArgs(PermissionsCmd::arg(), args);
    }

    const std::string usage = "--permissions: expected --permissions <create|read|update|delete|effective> <path> "
                              "[<value>] [--fmt cli|json]";
    if (args.size() < 2 || args.size() > 3) {
        throw std::runtime_error(usage);
    }
    const auto& operation = args[0];
    const auto& path      = args[1];
    std::string value     = args.size() == 3 ? args[2] : std::string{};
    try {
        check_arguments(operation, path, value); // reported by the client, before the request is sent
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string(e.what()) + "\n" + usage);
    }

    auto format = Format::Cli;
    if (vm.count(CtsApi::permissions_format_arg()) != 0) {
        const auto name = vm[CtsApi::permissions_format_arg()].as<std::string>();
        if (name == "json") {
            format = Format::Json;
        }
        else if (name != "cli") {
            throw std::runtime_error("--permissions: unknown format '" + name + "', expected cli or json");
        }
    }

    cmd = std::make_shared<PermissionsCmd>(operation, path, value, format);
}

STC_Cmd_ptr PermissionsCmd::doHandleRequest(AbstractServer* as) const {
    Defs* defs = as->defs().get();
    // The client checks the arguments too, but the command can be built directly (C++, Python)
    check_arguments(operation_, path_, value_);

    const bool server = path_ == "/";
    node_ptr node;
    if (!server) {
        node = find_node(defs, path_); // throws when no node has the path
    }
    const Permissions* current = server ? defs->server_state().permissions() : node->permissions();

    // A node the requester cannot read is never described; the authorisation already refuses a read lacking r, in
    // every mode (audit-only included), and this check guards the reply itself
    if (!server && !changes()) {
        if (const auto visibility = request_context().visibility();
            visibility->applies() && !visibility->readable(*node)) {
            throw std::runtime_error("The node " + path_ + " is not readable");
        }
    }

    if (operation_ == "effective") {
        const std::string user = value_.empty() ? identity().username().value() : value_;
        const auto roles       = effective_permissions(*defs, path_, user);
        if (!roles) {
            throw std::runtime_error("--permissions: the server does not use Permissions attributes");
        }
        if (format_ == Format::Json) {
            nlohmann::json reply{{"user", user}, {"path", path_}, {"permissions", roles->to_string()}};
            return PreAllocatedReply::string_cmd(reply.dump());
        }
        return PreAllocatedReply::string_cmd(roles->to_string());
    }

    if (operation_ == "read") {
        if (!current) {
            throw std::runtime_error("--permissions: " + path_ + " has no Permissions attribute");
        }
        return PreAllocatedReply::string_cmd(format_ == Format::Json ? current->to_json() : current->to_cli());
    }

    // create, update and delete
    if (operation_ == "create" && current) {
        throw std::runtime_error("--permissions: " + path_ + " already has a Permissions attribute");
    }
    if (operation_ != "create" && !current) {
        throw std::runtime_error("--permissions: " + path_ + " has no Permissions attribute");
    }
    if (server && operation_ != "update") {
        // The server is restricted, or unrestricted, only by its configuration file, at start-up (D10.2)
        throw std::runtime_error("--permissions: the server Permissions attribute cannot be " +
                                 std::string(operation_ == "create" ? "created" : "deleted") +
                                 " at run time; change the server configuration file and restart the server");
    }

    PermissionsChange change(*defs);
    if (server) {
        change.set_server(parse_value(value_));
    }
    else {
        change.set(*node, operation_ == "delete" ? std::nullopt : std::optional<Permissions>(parse_value(value_)));
    }
    change.commit(identity().username().value() + "@" + hostname());

    // Recorded in the edit history of the node by the server, as for every change; the server for "/"
    if (!server) {
        add_node_for_edit_history(node);
    }
    return PreAllocatedReply::ok_cmd();
}

std::ostream& operator<<(std::ostream& os, const PermissionsCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(PermissionsCmd)
CEREAL_REGISTER_DYNAMIC_INIT(PermissionsCmd)
