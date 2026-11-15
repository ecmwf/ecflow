// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>

#include <boost/beast/http/field.hpp>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/Authorisation.hpp"
#include "ecflow/base/RequestContext.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/base/cts/task/AbortCmd.hpp"
#include "ecflow/base/cts/task/CompleteCmd.hpp"
#include "ecflow/base/cts/task/CtsWaitCmd.hpp"
#include "ecflow/base/cts/task/EventCmd.hpp"
#include "ecflow/base/cts/task/InitCmd.hpp"
#include "ecflow/base/cts/task/LabelCmd.hpp"
#include "ecflow/base/cts/task/MeterCmd.hpp"
#include "ecflow/base/cts/task/QueueCmd.hpp"
#include "ecflow/base/cts/user/AlterCmd.hpp"
#include "ecflow/base/cts/user/BeginCmd.hpp"
#include "ecflow/base/cts/user/CFileCmd.hpp"
#include "ecflow/base/cts/user/CSyncCmd.hpp"
#include "ecflow/base/cts/user/CheckPtCmd.hpp"
#include "ecflow/base/cts/user/ClientHandleCmd.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/base/cts/user/CtsNodeCmd.hpp"
#include "ecflow/base/cts/user/DeleteCmd.hpp"
#include "ecflow/base/cts/user/EditScriptCmd.hpp"
#include "ecflow/base/cts/user/ForceCmd.hpp"
#include "ecflow/base/cts/user/FreeDepCmd.hpp"
#include "ecflow/base/cts/user/GroupCTSCmd.hpp"
#include "ecflow/base/cts/user/LoadDefsCmd.hpp"
#include "ecflow/base/cts/user/LogCmd.hpp"
#include "ecflow/base/cts/user/LogMessageCmd.hpp"
#include "ecflow/base/cts/user/MoveCmd.hpp"
#include "ecflow/base/cts/user/OrderNodeCmd.hpp"
#include "ecflow/base/cts/user/PathsCmd.hpp"
#include "ecflow/base/cts/user/PlugCmd.hpp"
#include "ecflow/base/cts/user/QueryCmd.hpp"
#include "ecflow/base/cts/user/ReplaceNodeCmd.hpp"
#include "ecflow/base/cts/user/RequeueNodeCmd.hpp"
#include "ecflow/base/cts/user/RunNodeCmd.hpp"
#include "ecflow/base/cts/user/ServerVersionCmd.hpp"
#include "ecflow/base/cts/user/ShowCmd.hpp"
#include "ecflow/base/cts/user/ZombieCmd.hpp"
#include "ecflow/node/ClientSuiteMgr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/server/BaseServer.hpp"

namespace ecf {

namespace implementation {

///
/// @brief Holds the authorisation of a command, in a static function accept(command, context).
///
/// Every command has its specialisation; the template is declared without definition, so that a command without one
/// does not build (R5.11).
///
/// @tparam COMMAND The type of the command
///
template <typename COMMAND>
struct Authoriser;

///
/// @brief Allows a task request: task requests are not subject to the authorisation of users (D5.1).
///
/// @tparam COMMAND The type of the command, a TaskCmd
/// @param[in] command The command
/// @return Success.
///
template <typename COMMAND>
authorisation_t task_request([[maybe_unused]] const COMMAND& command) {
    static_assert(std::is_base_of_v<TaskCmd, COMMAND>, "Only task requests are allowed without authorisation");
    return authorisation_t::success("Authorisation (task) granted");
}

///
/// @brief Returns whether a node, or any node below it, carries a Permissions attribute.
///
/// @param[in] node The node
/// @return True when an attribute is found.
///
inline bool carries_permissions(const Node& node) {
    if (node.permissions()) {
        return true;
    }
    std::vector<node_ptr> children;
    node.immediateChildren(children);
    return std::any_of(
        children.begin(), children.end(), [](const node_ptr& child) { return carries_permissions(*child); });
}

template <typename COMMAND>
authorisation_t do_authorise(const COMMAND& command, const RequestContext& context) {
    return Authoriser<COMMAND>::accept(command, context);
}

///
/// @brief Returns the node path of a target, without the attribute name of "path:name".
///
/// @param[in] target The target, as given to the command
/// @return The node path, or the target unchanged when it names no attribute.
///
inline std::string node_path_of(const std::string& target) {
    const auto slash = target.rfind('/');
    const auto colon = target.find(':', slash == std::string::npos ? 0 : slash);
    return colon == std::string::npos ? target : target.substr(0, colon);
}

///
/// @brief Requires a role at the targets of a request, in the mode in force.
///
/// In Permissions mode, the requester must hold the role at every target. In white-list mode, the role r requires
/// read access, and every other role write access, at the targets (D13.4). Without restriction, the request is
/// allowed.
///
/// @param[in] command The request
/// @param[in] context The context of the request
/// @param[in] role The role required
/// @param[in] targets The paths addressed: "/" for the server, or nodes
/// @return Success when the request is allowed; otherwise a failure holding the reason.
/// @throws std::exception when the permissions cannot be evaluated; the caller refuses the request (R11.7).
///
inline authorisation_t require_role(const ClientToServerCmd& command,
                                    const RequestContext& context,
                                    Role role,
                                    const std::vector<std::string>& targets) {
    static const std::string refused = "Authorisation (user) failed, due to: Insufficient permissions";
    auto& server                     = context.server();
    switch (authorisation_mode(server)) {
        case AuthorisationMode::Unrestricted:
            return authorisation_t::success("Authorisation (user) granted");
        case AuthorisationMode::Permissions:
            for (const auto& target : targets) {
                if (!context.roles_at(target).has(role)) {
                    // The reason names the role and the target denied; it reaches the client and the server log (D5.29)
                    return authorisation_t::failure(refused + " (role " + letter_of(role) + " required at " + target +
                                                    ")");
                }
            }
            return authorisation_t::success("Authorisation (user) granted");
        case AuthorisationMode::WhiteList:
            break;
    }
    // The white list matches node paths: an attribute named as "path:name" is checked at its node
    std::vector<std::string> nodes;
    nodes.reserve(targets.size());
    for (const auto& target : targets) {
        nodes.push_back(node_path_of(target));
    }
    const std::string access = role == Role::Read ? "read" : "write";
    if (server.authorisation().allows(command.identity(), server, nodes, access)) {
        return authorisation_t::success("Authorisation (user) granted");
    }
    std::string named;
    for (const auto& node : nodes) {
        named += (named.empty() ? "" : ", ") + node;
    }
    return authorisation_t::failure(refused + " (" + access + " access required at " + named + ")");
}

///
/// @brief Requires every role of a set at the targets of a request, in the mode in force.
///
/// In white-list mode, the set {r} requires read access, and any other set write access.
///
/// @param[in] command The request
/// @param[in] context The context of the request
/// @param[in] roles The roles required, all of them; at least one
/// @param[in] targets The paths addressed
/// @return Success when the request is allowed; otherwise a failure holding the reason.
/// @throws std::exception when the permissions cannot be evaluated; the caller refuses the request (R11.7).
///
inline authorisation_t require_roles(const ClientToServerCmd& command,
                                     const RequestContext& context,
                                     const std::vector<Role>& roles,
                                     const std::vector<std::string>& targets) {
    if (authorisation_mode(context.server()) == AuthorisationMode::WhiteList) {
        const bool read = roles.size() == 1 && roles.front() == Role::Read;
        return require_role(command, context, read ? Role::Read : Role::Write, targets);
    }
    for (const auto role : roles) {
        if (auto result = require_role(command, context, role, targets); !result.ok()) {
            return result;
        }
    }
    return authorisation_t::success("Authorisation (user) granted");
}

///
/// @brief Requires a role at the server level.
///
inline authorisation_t require_server_role(const ClientToServerCmd& command, const RequestContext& context, Role role) {
    return require_role(command, context, role, {"/"});
}

///
/// @brief Returns the targets of a command addressing nodes, or the server when it addresses none.
///
inline std::vector<std::string> node_or_server_paths(const std::vector<std::string>& paths) {
    return paths.empty() ? std::vector<std::string>{"/"} : paths;
}

///
/// @brief Returns the target of a command addressing a node, or the server when it addresses none.
///
inline std::vector<std::string> node_or_server(const std::string& path) {
    return {path.empty() ? std::string("/") : path};
}

// Task commands

template <>
struct Authoriser<InitCmd>
{
    static authorisation_t accept(const InitCmd& command, const RequestContext& context) {
        return task_request(command);
    }
};

template <>
struct Authoriser<CompleteCmd>
{
    static authorisation_t accept(const CompleteCmd& command, const RequestContext& context) {
        return task_request(command);
    }
};

template <>
struct Authoriser<AbortCmd>
{
    static authorisation_t accept(const AbortCmd& command, const RequestContext& context) {
        return task_request(command);
    }
};

template <>
struct Authoriser<LabelCmd>
{
    static authorisation_t accept(const LabelCmd& command, const RequestContext& context) {
        return task_request(command);
    }
};

template <>
struct Authoriser<MeterCmd>
{
    static authorisation_t accept(const MeterCmd& command, const RequestContext& context) {
        return task_request(command);
    }
};

template <>
struct Authoriser<EventCmd>
{
    static authorisation_t accept(const EventCmd& command, const RequestContext& context) {
        return task_request(command);
    }
};

template <>
struct Authoriser<QueueCmd>
{
    static authorisation_t accept(const QueueCmd& command, const RequestContext& context) {
        return task_request(command);
    }
};

template <>
struct Authoriser<CtsWaitCmd>
{
    static authorisation_t accept(const CtsWaitCmd& command, const RequestContext& context) {
        return task_request(command);
    }
};

// User commands

template <>
struct Authoriser<AlterCmd>
{
    static authorisation_t accept(const AlterCmd& command, const RequestContext& context) {
        // Attributes and variables, the server variables at "/" included (D2.10, D11.1)
        return require_role(command, context, Role::Write, node_or_server_paths(command.paths()));
    }
};

template <>
struct Authoriser<BeginCmd>
{
    static authorisation_t accept(const BeginCmd& command, const RequestContext& context) {
        // Beginning suites is an owner operation at server level (D2.10)
        return require_server_role(command, context, Role::Owner);
    }
};

template <>
struct Authoriser<CFileCmd>
{
    static authorisation_t accept(const CFileCmd& command, const RequestContext& context) {
        // Retrieving files is a read, even when the server runs a command to do so (R11.3, D11.3)
        return require_role(command, context, Role::Read, node_or_server(command.pathToNode()));
    }
};

template <>
struct Authoriser<CheckPtCmd>
{
    static authorisation_t accept(const CheckPtCmd& command, const RequestContext& context) {
        // Checkpoint commands administer the server (D10.1), with or without arguments (R11.3)
        return require_server_role(command, context, Role::Execute);
    }
};

template <>
struct Authoriser<ClientHandleCmd>
{
    static authorisation_t accept(const ClientHandleCmd& command, const RequestContext& context) {
        if (auto result = require_server_role(command, context, Role::Read); !result.ok()) {
            return result; // D10.1
        }
        if (authorisation_mode(context.server()) == AuthorisationMode::Unrestricted) {
            return authorisation_t::success("Authorisation (user) granted");
        }

        // Handles are bound to their creator (D10.7)
        const auto& requester = command.identity().username().value();
        switch (command.api()) {
            case ClientHandleCmd::REGISTER:
                if (command.client_handle() == 0) {
                    break; // a new handle
                }
                [[fallthrough]]; // registering an existing handle drops it first
            case ClientHandleCmd::DROP:
            case ClientHandleCmd::ADD:
            case ClientHandleCmd::REMOVE:
            case ClientHandleCmd::AUTO_ADD: {
                const auto defs = context.server().defs();
                for (const auto& handle : defs->client_suite_mgr().clientSuites()) {
                    if (static_cast<int>(handle.handle()) == command.client_handle() && handle.user() != requester) {
                        return authorisation_t::failure(
                            "Authorisation (user) failed, due to: the handle belongs to another user");
                    }
                }
                break; // a handle that does not exist is reported by the command
            }
            case ClientHandleCmd::DROP_USER:
                if (!command.drop_user().empty() && command.drop_user() != requester) {
                    return authorisation_t::failure(
                        "Authorisation (user) failed, due to: the handles belong to another user");
                }
                break;
            case ClientHandleCmd::SUITES:
                break;
        }
        return authorisation_t::success("Authorisation (user) granted");
    }
};

template <>
struct Authoriser<CSyncCmd>
{
    static authorisation_t accept(const CSyncCmd& command, const RequestContext& context) {
        // Synchronisation reads the server; its content is filtered (D10.1)
        return require_server_role(command, context, Role::Read);
    }
};

template <>
struct Authoriser<CtsCmd>
{
    static authorisation_t accept(const CtsCmd& command, const RequestContext& context) {
        switch (command.api()) {
            case CtsCmd::PING:
                // In Permissions mode, --ping is allowed to every user, listed or not (D10.1)
                if (authorisation_mode(context.server()) == AuthorisationMode::Permissions) {
                    return authorisation_t::success("Authorisation (user) granted (ping)");
                }
                return require_server_role(command, context, Role::Read);
            case CtsCmd::GET_ZOMBIES:
            case CtsCmd::STATS:
            case CtsCmd::STATS_SERVER:
            case CtsCmd::SUITES:
            case CtsCmd::SERVER_LOAD:
                return require_server_role(command, context, Role::Read); // D10.1
            case CtsCmd::DEBUG_SERVER_ON:
            case CtsCmd::DEBUG_SERVER_OFF:
            case CtsCmd::STATS_RESET:
                return require_server_role(command, context, Role::Write); // D10.6
            case CtsCmd::RESTORE_DEFS_FROM_CHECKPT:
            case CtsCmd::RESTART_SERVER:
            case CtsCmd::SHUTDOWN_SERVER:
            case CtsCmd::HALT_SERVER:
            case CtsCmd::TERMINATE_SERVER:
            case CtsCmd::RELOAD_WHITE_LIST_FILE:
            case CtsCmd::RELOAD_PASSWD_FILE:
            case CtsCmd::RELOAD_CUSTOM_PASSWD_FILE:
            case CtsCmd::FORCE_DEP_EVAL:
                return require_server_role(command, context, Role::Execute); // D10.1
            case CtsCmd::NO_CMD:
                break;
        }
        return authorisation_t::failure("Authorisation (user) failed, due to: unknown command");
    }
};

template <>
struct Authoriser<CtsNodeCmd>
{
    static authorisation_t accept(const CtsNodeCmd& command, const RequestContext& context) {
        switch (command.api()) {
            case CtsNodeCmd::GET:
            case CtsNodeCmd::GET_STATE:
            case CtsNodeCmd::MIGRATE:
            case CtsNodeCmd::WHY:
                return require_role(command, context, Role::Read, node_or_server(command.pathToNode()));
            case CtsNodeCmd::JOB_GEN:
            case CtsNodeCmd::CHECK_JOB_GEN_ONLY:
                // Job generation changes the state of the nodes, and runs commands (R11.3)
                return require_role(command, context, Role::Execute, node_or_server(command.pathToNode()));
            case CtsNodeCmd::NO_CMD:
                break;
        }
        return authorisation_t::failure("Authorisation (user) failed, due to: unknown command");
    }
};

template <>
struct Authoriser<DeleteCmd>
{
    static authorisation_t accept(const DeleteCmd& command, const RequestContext& context) {
        // Deleting a node requires w at the node; deleting a suite, or every suite, o at server level (D4.7);
        // every path is checked (R5.12), as resolved by the server (e.g. "/s/" is the suite s)
        const auto& paths = command.paths();
        if (paths.empty()) {
            return require_server_role(command, context, Role::Owner);
        }
        const auto defs = context.server().defs();
        for (const auto& path : paths) {
            const auto node = defs->findAbsNode(path);
            if (auto result = (node && node->isSuite())
                                  ? require_server_role(command, context, Role::Owner)
                                  : require_role(command, context, Role::Write, {node ? node->absNodePath() : path});
                !result.ok()) {
                return result;
            }
        }
        return authorisation_t::success("Authorisation (user) granted");
    }
};

template <>
struct Authoriser<EditScriptCmd>
{
    static authorisation_t accept(const EditScriptCmd& command, const RequestContext& context) {
        const auto targets = node_or_server(command.pathToNode());
        switch (command.edit_type()) {
            case EditScriptCmd::EDIT:
            case EditScriptCmd::PREPROCESS:
            case EditScriptCmd::PREPROCESS_USER_FILE:
                return require_role(command, context, Role::Read, targets); // R11.3, D11.3
            case EditScriptCmd::SUBMIT:
                return require_role(command, context, Role::Execute, targets);
            case EditScriptCmd::SUBMIT_USER_FILE:
                // Creating an alias also defines a node
                return command.alias() ? require_roles(command, context, {Role::Execute, Role::Write}, targets)
                                       : require_role(command, context, Role::Execute, targets);
        }
        return authorisation_t::failure("Authorisation (user) failed, due to: unknown command");
    }
};

template <>
struct Authoriser<ForceCmd>
{
    static authorisation_t accept(const ForceCmd& command, const RequestContext& context) {
        // Forcing a state or an event operates the node (D2.10); "path:event" is checked at the node
        return require_role(command, context, Role::Execute, node_or_server_paths(command.paths()));
    }
};

template <>
struct Authoriser<FreeDepCmd>
{
    static authorisation_t accept(const FreeDepCmd& command, const RequestContext& context) {
        // Every path is checked (R5.12), not the server only
        return require_role(command, context, Role::Execute, node_or_server_paths(command.paths()));
    }
};

template <>
struct Authoriser<LoadDefsCmd>
{
    static authorisation_t accept(const LoadDefsCmd& command, const RequestContext& context) {
        // Loading suites, and overwriting them, requires o at server level (D4.5)
        if (auto result = require_server_role(command, context, Role::Owner); !result.ok()) {
            return result;
        }
        const auto defs = command.definitions();
        if (!defs) {
            return authorisation_t::success("Authorisation (user) granted"); // the command reports the error
        }

        // Server variables carried by the definitions require w at server level (R11.4)
        if (!defs->server_state().user_variables().empty()) {
            if (auto result = require_server_role(command, context, Role::Write); !result.ok()) {
                return result;
            }
        }

        // Attributes carried, or overwritten by --force, require p at the suite (R4.5)
        const auto server_defs = context.server().defs();
        for (const auto& suite : defs->suites()) {
            const auto existing = server_defs->findSuite(suite->name());
            if (carries_permissions(*suite) || (command.force() && existing && carries_permissions(*existing))) {
                if (auto result = require_role(command, context, Role::Permissions, {suite->absNodePath()});
                    !result.ok()) {
                    return result;
                }
            }
        }
        return authorisation_t::success("Authorisation (user) granted");
    }
};

template <>
struct Authoriser<LogCmd>
{
    static authorisation_t accept(const LogCmd& command, const RequestContext& context) {
        switch (command.api()) {
            case LogCmd::GET:
            case LogCmd::PATH:
                return require_server_role(command, context, Role::Read); // D10.1
            case LogCmd::CLEAR:
            case LogCmd::FLUSH:
            case LogCmd::NEW:
                return require_server_role(command, context, Role::Log); // D2.21
        }
        return authorisation_t::failure("Authorisation (user) failed, due to: unknown command");
    }
};

template <>
struct Authoriser<LogMessageCmd>
{
    static authorisation_t accept(const LogMessageCmd& command, const RequestContext& context) {
        // --msg appends to the server log (D10.6, D2.21)
        return require_server_role(command, context, Role::Write);
    }
};

template <>
struct Authoriser<MoveCmd>
{
    static authorisation_t accept(const MoveCmd& command, const RequestContext& context) {
        // The receiving side of a plug: a whole suite requires o at server level (D4.15), and p when it carries
        // attributes; a node under a destination requires w and p there (D4.11, R4.5, R11.9)
        if (command.dest().empty()) {
            if (auto result = require_server_role(command, context, Role::Owner); !result.ok()) {
                return result;
            }
            std::string error;
            const auto node = Node::create(command.src_node(), error);
            if (node && carries_permissions(*node)) {
                return require_role(command, context, Role::Permissions, {"/" + node->name()});
            }
            return authorisation_t::success("Authorisation (user) granted");
        }
        return require_roles(command, context, {Role::Write, Role::Permissions}, {command.dest()});
    }
};

template <>
struct Authoriser<OrderNodeCmd>
{
    static authorisation_t accept(const OrderNodeCmd& command, const RequestContext& context) {
        // Ordering changes the definitions; checked at the node only (D5.19)
        return require_role(command, context, Role::Write, node_or_server(command.pathToNode()));
    }
};

template <>
struct Authoriser<PathsCmd>
{
    static authorisation_t accept(const PathsCmd& command, const RequestContext& context) {
        const auto& paths = command.paths();
        switch (command.api()) {
            case PathsCmd::SUSPEND:
            case PathsCmd::RESUME:
            case PathsCmd::KILL:
            case PathsCmd::STATUS:
                return require_role(command, context, Role::Execute, node_or_server_paths(paths)); // D2.10
            case PathsCmd::CHECK:
                // "_all_" checks the whole server
                if (paths.size() == 1 && paths.front() == "_all_") {
                    return require_server_role(command, context, Role::Read);
                }
                return require_role(command, context, Role::Read, node_or_server_paths(paths));
            case PathsCmd::EDIT_HISTORY:
                // "clear" clears the history of every node: a change of the server
                if (paths.size() == 1 && paths.front() == "clear") {
                    return require_server_role(command, context, Role::Write);
                }
                // The edit history is returned unfiltered to users with r at server level (D5.28)
                return require_server_role(command, context, Role::Read);
            case PathsCmd::ARCHIVE:
                // Archiving changes the tree below the node
                return require_role(command, context, Role::Write, node_or_server_paths(paths));
            case PathsCmd::RESTORE:
                // Restoring brings back nodes, which may carry Permissions attributes (R4.5)
                return require_roles(command, context, {Role::Write, Role::Permissions}, node_or_server_paths(paths));
            case PathsCmd::NO_CMD:
                break;
        }
        return authorisation_t::failure("Authorisation (user) failed, due to: unknown command");
    }
};

template <>
struct Authoriser<PlugCmd>
{
    static authorisation_t accept(const PlugCmd& command, const RequestContext& context) {
        // Plugging moves the source under the destination, which changes the attributes applying to it: w and p
        // at both (D4.11, R4.5); a destination on another server is authorised there, under the identity of the
        // user (R11.9)
        std::vector<std::string> targets{command.source()};
        if (const auto destination = command.local_destination(context.server())) {
            targets.push_back(*destination);
        }
        return require_roles(command, context, {Role::Write, Role::Permissions}, targets);
    }
};

template <>
struct Authoriser<QueryCmd>
{
    static authorisation_t accept(const QueryCmd& command, const RequestContext& context) {
        // A query reads the node holding the attribute, or the server for "/" (D3.1)
        return require_role(command, context, Role::Read, node_or_server(command.path_to_attribute()));
    }
};

template <>
struct Authoriser<ReplaceNodeCmd>
{
    static authorisation_t accept(const ReplaceNodeCmd& command, const RequestContext& context) {
        const auto server_defs = context.server().defs();
        const auto defs        = command.client_definitions();
        const auto client_node = defs ? defs->findAbsNode(command.pathToNode()) : node_ptr{};
        if (!client_node) {
            // The command reports the missing or unparsable client definitions; nothing is imported
            return require_role(command, context, Role::Write, {command.pathToNode()});
        }

        // The node replaced, or the subtree imported, as Defs::replaceChild determines them: when the node does not
        // exist and parents are created, the topmost missing ancestor of the client node, or its whole suite
        const auto path      = client_node->absNodePath();
        const auto existing  = server_defs->findAbsNode(path);
        const Node* imported = client_node.get();
        node_ptr server_parent;
        if (!existing && command.createNodesAsNeeded()) {
            for (const Node* parent = imported->parent(); parent != nullptr; parent = parent->parent()) {
                if ((server_parent = server_defs->findAbsNode(parent->absNodePath()))) {
                    break;
                }
                imported = parent;
            }
        }

        // Replacing or creating a suite requires o at server level (D4.5, D4.14); otherwise w at the node replaced,
        // or at the existing parent of the subtree created (D4.14)
        const bool suite = existing ? existing->isSuite() != nullptr : imported->isSuite() != nullptr;
        if (auto result = suite
                              ? require_server_role(command, context, Role::Owner)
                              : require_role(command,
                                             context,
                                             Role::Write,
                                             {existing ? path : (server_parent ? server_parent->absNodePath() : path)});
            !result.ok()) {
            return result;
        }

        // Attributes brought, anywhere in the imported subtree, or replaced, require p (R4.5, D4.10)
        if (carries_permissions(*imported) || (existing && carries_permissions(*existing))) {
            return require_role(command, context, Role::Permissions, {imported->absNodePath()});
        }
        return authorisation_t::success("Authorisation (user) granted");
    }
};

template <>
struct Authoriser<RequeueNodeCmd>
{
    static authorisation_t accept(const RequeueNodeCmd& command, const RequestContext& context) {
        return require_role(command, context, Role::Execute, node_or_server_paths(command.paths())); // D2.10
    }
};

template <>
struct Authoriser<RunNodeCmd>
{
    static authorisation_t accept(const RunNodeCmd& command, const RequestContext& context) {
        return require_role(command, context, Role::Execute, node_or_server_paths(command.paths())); // D2.10
    }
};

template <>
struct Authoriser<ServerVersionCmd>
{
    static authorisation_t accept(const ServerVersionCmd& command, const RequestContext& context) {
        return require_server_role(command, context, Role::Read); // D10.1
    }
};

template <>
struct Authoriser<ShowCmd>
{
    static authorisation_t accept(const ShowCmd& command, const RequestContext& context) {
        // The client formats the definitions it received; the server is read
        return require_server_role(command, context, Role::Read);
    }
};

template <>
struct Authoriser<ZombieCmd>
{
    static authorisation_t accept(const ZombieCmd& command, const RequestContext& context) {
        // The zombie actions change the state of the server (D10.6)
        return require_server_role(command, context, Role::Write);
    }
};

template <>
struct Authoriser<GroupCTSCmd>
{
    static authorisation_t accept(const GroupCTSCmd& command, const RequestContext& context) {
        // In Permissions mode, each sub-command is authorised just before it runs (D5.27); see GroupCTSCmd
        if (authorisation_mode(context.server()) == AuthorisationMode::Permissions) {
            return authorisation_t::success("Authorisation (user) deferred to the sub-commands (group command)");
        }

        // Otherwise, as before, a group is allowed only when all its sub-commands are, each by its own rules
        for (const auto& cmd : command.cmdVec()) {
            if (auto result = cmd->authorise(context); !result.ok()) {
                return result;
            }
        }

        return authorisation_t::success("Authorisation (user) granted (group command)");
    }
};

} // namespace implementation

} // namespace ecf
