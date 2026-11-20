// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/cts/user/OwnershipCmd.hpp"

#include <iostream>
#include <stdexcept>

#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/Ownership.hpp"
#include "ecflow/node/SuiteChanged.hpp"

using namespace ecf;

namespace {

bool needs_value(OwnershipCmd::Op op) {
    return op == OwnershipCmd::Op::CREATE || op == OwnershipCmd::Op::UPDATE;
}

bool is_valid(OwnershipCmd::Op op) {
    switch (op) {
        case OwnershipCmd::Op::CREATE:
        case OwnershipCmd::Op::READ:
        case OwnershipCmd::Op::UPDATE:
        case OwnershipCmd::Op::DELETE:
            return true;
    }
    return false;
}

/// Checks the arguments of the command; made by the client, and again by the server
void check_arguments(OwnershipCmd::Op op, const std::string& path, const std::string& value) {
    if (!is_valid(op)) {
        throw std::runtime_error("OwnershipCmd: unknown operation " + std::to_string(static_cast<int>(op)));
    }
    const std::string name = OwnershipCmd::to_string(op);
    if (path.empty() || path[0] != '/') {
        throw std::runtime_error("OwnershipCmd: the path must be absolute, but found '" + path + "'");
    }
    if (path == "/" && op != OwnershipCmd::Op::READ) {
        throw std::runtime_error("OwnershipCmd: the server Owner is configuration, and cannot be changed by a command; "
                                 "only 'read' is allowed on '/'");
    }
    if (needs_value(op)) {
        if (value.empty()) {
            throw std::runtime_error("OwnershipCmd: '" + name + "' requires an Owner, as user[:group]");
        }
        (void)OwnerAttr::create(value); // throws when malformed
    }
    else if (!value.empty()) {
        throw std::runtime_error("OwnershipCmd: '" + name + "' does not take an Owner, but found '" + value + "'");
    }
}

std::string value_or_none(const OwnerAttr* owner) {
    return owner ? owner->value() : std::string("none");
}

} // namespace

OwnershipCmd::OwnershipCmd(Op op, const std::string& path, const std::string& value)
    : op_(op),
      path_(path),
      value_(value) {
    check_arguments(op_, path_, value_);
}

OwnershipCmd OwnershipCmd::make_unchecked(Op op, const std::string& path, const std::string& value) {
    OwnershipCmd cmd;
    cmd.op_    = op;
    cmd.path_  = path;
    cmd.value_ = value;
    return cmd;
}

const char* OwnershipCmd::to_string(Op op) {
    switch (op) {
        case Op::CREATE:
            return "create";
        case Op::READ:
            return "read";
        case Op::UPDATE:
            return "update";
        case Op::DELETE:
            return "delete";
    }
    return "unknown";
}

std::optional<OwnershipCmd::Op> OwnershipCmd::to_op(const std::string& name) {
    for (auto op : {Op::CREATE, Op::READ, Op::UPDATE, Op::DELETE}) {
        if (name == to_string(op)) {
            return op;
        }
    }
    return std::nullopt;
}

const char* OwnershipCmd::minimum_server_version() {
    return "5.20.0";
}

std::optional<std::string> OwnershipCmd::explain_unsupported(const std::string& error) {
    // A server that cannot decode the request replies "... replied with: Invalid argument"; the client reports
    // "invalid_argument(Could not decode client protocol)" when it detects the same condition itself
    if (error.find("replied with: Invalid argument") == std::string::npos &&
        error.find("Could not decode client protocol") == std::string::npos) {
        return std::nullopt;
    }
    return std::string("Error: the server does not support the ownership command, which requires an ecFlow server ") +
           minimum_server_version() + " or later.\n" + error;
}

void OwnershipCmd::print(std::string& os) const {
    user_cmd(os, CtsApi::to_string(CtsApi::ownership(to_string(op_), path_, value_)));
}

void OwnershipCmd::print_only(std::string& os) const {
    os += CtsApi::to_string(CtsApi::ownership(to_string(op_), path_, value_));
}

bool OwnershipCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<OwnershipCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (op_ != the_rhs->op() || path_ != the_rhs->path() || value_ != the_rhs->value()) {
        return false;
    }
    return UserCmd::equals(rhs);
}

ecf::authentication_t OwnershipCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t OwnershipCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

const char* OwnershipCmd::arg() {
    return CtsApi::ownershipArg();
}

void OwnershipCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(OwnershipCmd::arg(), boost::program_options::value<std::vector<std::string>>()->multitoken());
}

void OwnershipCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const {
    auto args = vm[arg()].as<std::vector<std::string>>();
    if (clientEnv->debug()) {
        dumpVecArgs(OwnershipCmd::arg(), args);
    }

    if (args.empty()) {
        throw std::runtime_error("OwnershipCmd: expected an operation, one of [create | read | update | delete]");
    }
    auto op = to_op(args[0]);
    if (!op) {
        throw std::runtime_error("OwnershipCmd: expected an operation, one of [create | read | update | delete], but "
                                 "found '" +
                                 args[0] + "'");
    }
    const std::size_t expected = needs_value(*op) ? 3 : 2;
    if (args.size() != expected) {
        throw std::runtime_error(std::string("OwnershipCmd: '") + to_string(*op) + "' expects " +
                                 (needs_value(*op) ? "a path and an Owner (user[:group])" : "a path only") +
                                 ", but found " + std::to_string(args.size() - 1) + " argument(s)");
    }

    cmd = std::make_shared<OwnershipCmd>(*op, args[1], expected == 3 ? args[2] : std::string());
}

STC_Cmd_ptr OwnershipCmd::doHandleRequest(AbstractServer* as) const {
    check_arguments(op_, path_, value_); // the command may come from any client

    if (op_ == Op::READ) {
        return read(as);
    }

    Defs* defs         = as->defs().get();
    node_ptr node      = find_node_for_edit(defs, path_);
    const auto& policy = as->ownership();

    SuiteChangedPtr changed(node.get());
    switch (op_) {
        case Op::CREATE: {
            const auto owner = OwnerAttr::create(value_);
            policy.check_create(identity(), *node, owner);
            node->addOwner(owner);
            policy.report_addition(owner, node->absNodePath(), "create");
            break;
        }
        case Op::UPDATE: {
            const auto owner = OwnerAttr::create(value_);
            policy.check_update(identity(), *node, owner);
            node->changeOwner(owner);
            policy.report_addition(owner, node->absNodePath(), "update");
            break;
        }
        case Op::DELETE: {
            policy.check_delete(identity(), *node);
            node->deleteOwner();
            break;
        }
        case Op::READ: // handled above
            break;
    }
    return PreAllocatedReply::ok_cmd();
}

STC_Cmd_ptr OwnershipCmd::read(AbstractServer* as) const {
    Defs* defs = as->defs().get();

    std::string reply;
    std::optional<ownership::ResolvedOwner> effective;
    if (path_ == "/") {
        if (const auto* server_owner = defs->server_state().server_owner(); server_owner) {
            effective = ownership::ResolvedOwner{*server_owner, nullptr};
        }
    }
    else {
        node_ptr node = find_node(defs, path_);
        reply += "owner=" + value_or_none(node->get_owner()) + "\n";
        effective = ownership::resolve(*node);
    }
    reply += "effective=" + (effective ? effective->owner.value() : std::string("none")) + "\n";
    reply += "origin=" + (effective ? effective->origin_path() : std::string("none")) + "\n";
    return PreAllocatedReply::string_cmd(reply);
}

std::ostream& operator<<(std::ostream& os, const OwnershipCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(OwnershipCmd)
CEREAL_REGISTER_DYNAMIC_INIT(OwnershipCmd)
