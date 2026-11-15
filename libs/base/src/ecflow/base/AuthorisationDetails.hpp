// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

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
#include "ecflow/server/BaseServer.hpp"

namespace ecf {

namespace implementation {

// The Authoriser struct is a template that provides a static accept() function

template <typename COMMAND>
struct Authoriser
{
};

template <typename COMMAND>
void accumulate_paths(const COMMAND& command, std::vector<std::string>& paths) {
    Authoriser<COMMAND>::paths(command, paths);
}

template <typename COMMAND>
std::vector<std::string> get_affected_paths(const COMMAND& command) {
    std::vector<std::string> paths;
    accumulate_paths(command, paths);
    return paths;
}

///
/// @brief Applies the transitional rules of a command: a read requires reading, a change requires writing, at every
///        path the command addresses.
///
/// The rules hold in each mode:
///
/// - task requests are not subject to the authorisation of users (D5.1);
/// - Permissions mode: the role r for a read, w for a change, at each path, as granted by the Permissions attributes;
///   the white list is not used, and grants nothing, the server user included (D5.17, D11.6);
/// - white-list mode: read or write access, as granted by the white list;
/// - unrestricted: every request is allowed.
///
/// The per-command rules replace them, command by command (S12 to S14).
///
/// @tparam COMMAND The type of the command, a TaskCmd or a UserCmd
/// @param[in] command The command
/// @param[in] context The context of the request
/// @return Success when the request is allowed; otherwise a failure holding the reason.
/// @throws std::exception when the permissions cannot be evaluated; the caller refuses the request (R11.7).
///
template <typename COMMAND>
authorisation_t allows_as_per_read_write_rules(const COMMAND& command, const RequestContext& context) {

    static_assert(std::is_base_of_v<TaskCmd, COMMAND> || std::is_base_of_v<UserCmd, COMMAND>,
                  "The command must be either a TaskCmd or a UserCmd");

    if constexpr (std::is_base_of_v<TaskCmd, COMMAND>) {
        // No actual verification is done for task commands
        return authorisation_t::success("Authorisation (task) granted");
    }

    auto& server = context.server();
    switch (authorisation_mode(server)) {
        case AuthorisationMode::Unrestricted:
            return authorisation_t::success("Authorisation (user) granted");
        case AuthorisationMode::Permissions: {
            const Role role = command.isWrite() ? Role::Write : Role::Read;
            for (const auto& path : get_affected_paths(command)) {
                if (!context.roles_at(path).has(role)) {
                    return authorisation_t::failure("Authorisation (user) failed, due to: Insufficient permissions");
                }
            }
            return authorisation_t::success("Authorisation (user) granted");
        }
        case AuthorisationMode::WhiteList:
            break;
    }

    std::vector<std::string> paths = get_affected_paths(command);

    const std::string required_permission = command.isWrite() ? "write" : "read";
    if (server.authorisation().allows(command.identity(), server, paths, required_permission)) {
        return authorisation_t::success("Authorisation (user) granted");
    }

    return authorisation_t::failure("Authorisation (user) failed, due to: Insufficient permissions");
}

template <typename COMMAND>
authorisation_t do_authorise(const COMMAND& command, const RequestContext& context) {
    return Authoriser<COMMAND>::accept(command, context);
}

// Task commands

template <>
struct Authoriser<InitCmd>
{
    static authorisation_t accept(const InitCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const InitCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }
};

template <>
struct Authoriser<CompleteCmd>
{
    static authorisation_t accept(const CompleteCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const CompleteCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }
};

template <>
struct Authoriser<AbortCmd>
{
    static authorisation_t accept(const AbortCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const AbortCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }
};

template <>
struct Authoriser<LabelCmd>
{
    static authorisation_t accept(const LabelCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const LabelCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }
};

template <>
struct Authoriser<MeterCmd>
{
    static authorisation_t accept(const MeterCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const MeterCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }
};

template <>
struct Authoriser<EventCmd>
{
    static authorisation_t accept(const EventCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const EventCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }
};

template <>
struct Authoriser<QueueCmd>
{
    static authorisation_t accept(const QueueCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const QueueCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }
};

template <>
struct Authoriser<CtsWaitCmd>
{
    static authorisation_t accept(const CtsWaitCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const CtsWaitCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }
};

// User commands

template <typename COMMAND>
inline void select_all_paths(const COMMAND& command, std::vector<std::string>& paths) {
    if (auto&& affected = command.paths(); affected.empty()) {
        paths.push_back("/");
    }
    else {
        for (auto&& path : affected) {
            paths.push_back(path);
        }
    }
}

template <typename COMMAND>
inline void select_root_path([[maybe_unused]] const COMMAND& command, std::vector<std::string>& paths) {
    paths.push_back("/");
}

template <typename COMMAND>
inline void select_node_path(const COMMAND& command, std::vector<std::string>& paths) {
    paths.push_back(command.pathToNode());
}

template <>
struct Authoriser<AlterCmd>
{
    static authorisation_t accept(const AlterCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const AlterCmd& command, std::vector<std::string>& paths) { select_all_paths(command, paths); }
};

template <>
struct Authoriser<BeginCmd>
{
    static authorisation_t accept(const BeginCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const BeginCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }
};

template <>
struct Authoriser<CFileCmd>
{
    static authorisation_t accept(const CFileCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const CFileCmd& command, std::vector<std::string>& paths) { select_node_path(command, paths); }
};

template <>
struct Authoriser<CheckPtCmd>
{
    static authorisation_t accept(const CheckPtCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const CheckPtCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }
};

template <>
struct Authoriser<ClientHandleCmd>
{
    static authorisation_t accept(const ClientHandleCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const ClientHandleCmd& command, std::vector<std::string>& paths) {
        select_root_path(command, paths);
    }
};

template <>
struct Authoriser<CSyncCmd>
{
    static authorisation_t accept(const CSyncCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const CSyncCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }
};

template <>
struct Authoriser<CtsCmd>
{
    static authorisation_t accept(const CtsCmd& command, const RequestContext& context) {
        // In Permissions mode, --ping is allowed to every user, listed or not (D10.1)
        if (command.api() == CtsCmd::PING && authorisation_mode(context.server()) == AuthorisationMode::Permissions) {
            return authorisation_t::success("Authorisation (user) granted (ping)");
        }
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const CtsCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }
};

template <>
struct Authoriser<CtsNodeCmd>
{
    static authorisation_t accept(const CtsNodeCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const CtsNodeCmd& command, std::vector<std::string>& paths) {
        if (auto&& path = command.pathToNode(); path.empty()) {
            select_root_path(command, paths);
        }
        else {
            paths.push_back(path);
        }
    }
};

template <>
struct Authoriser<DeleteCmd>
{
    static authorisation_t accept(const DeleteCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const DeleteCmd& command, std::vector<std::string>& paths) {
        if (auto&& affected = command.paths(); affected.empty()) {
            paths.push_back("/");
        }
        else {
            paths.push_back(affected[0]);
        }
    }
};

template <>
struct Authoriser<EditScriptCmd>
{
    static authorisation_t accept(const EditScriptCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const EditScriptCmd& command, std::vector<std::string>& paths) {
        select_node_path(command, paths);
    }
};

template <>
struct Authoriser<ForceCmd>
{
    static authorisation_t accept(const ForceCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const ForceCmd& command, std::vector<std::string>& paths) { select_all_paths(command, paths); }
};

template <>
struct Authoriser<FreeDepCmd>
{
    static authorisation_t accept(const FreeDepCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const FreeDepCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }
};

template <>
struct Authoriser<LoadDefsCmd>
{
    static authorisation_t accept(const LoadDefsCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const LoadDefsCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }
};

template <>
struct Authoriser<LogCmd>
{
    static authorisation_t accept(const LogCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const LogCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }
};

template <>
struct Authoriser<LogMessageCmd>
{
    static authorisation_t accept(const LogMessageCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const LogMessageCmd& command, std::vector<std::string>& paths) {
        select_root_path(command, paths);
    }
};

template <>
struct Authoriser<MoveCmd>
{
    static authorisation_t accept(const MoveCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const MoveCmd& command, std::vector<std::string>& paths) { paths.push_back(command.src_path()); }
};

template <>
struct Authoriser<OrderNodeCmd>
{
    static authorisation_t accept(const OrderNodeCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const OrderNodeCmd& command, std::vector<std::string>& paths) {
        select_node_path(command, paths);
    }
};

template <>
struct Authoriser<PathsCmd>
{
    static authorisation_t accept(const PathsCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const PathsCmd& command, std::vector<std::string>& paths) { select_all_paths(command, paths); }
};

template <>
struct Authoriser<PlugCmd>
{
    static authorisation_t accept(const PlugCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const PlugCmd& command, std::vector<std::string>& paths) { paths.push_back(command.source()); }
};

template <>
struct Authoriser<QueryCmd>
{
    static authorisation_t accept(const QueryCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const QueryCmd& command, std::vector<std::string>& paths) {
        paths.push_back(command.path_to_attribute());
    }
};

template <>
struct Authoriser<ReplaceNodeCmd>
{
    static authorisation_t accept(const ReplaceNodeCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const ReplaceNodeCmd& command, std::vector<std::string>& paths) {
        select_node_path(command, paths);
    }
};

template <>
struct Authoriser<RequeueNodeCmd>
{
    static authorisation_t accept(const RequeueNodeCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const RequeueNodeCmd& command, std::vector<std::string>& paths) {
        select_all_paths(command, paths);
    }
};

template <>
struct Authoriser<RunNodeCmd>
{
    static authorisation_t accept(const RunNodeCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const RunNodeCmd& command, std::vector<std::string>& paths) { select_all_paths(command, paths); }
};

template <>
struct Authoriser<ServerVersionCmd>
{
    static authorisation_t accept(const ServerVersionCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const ServerVersionCmd& command, std::vector<std::string>& paths) {
        select_root_path(command, paths);
    }
};

template <>
struct Authoriser<ShowCmd>
{
    static authorisation_t accept(const ShowCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const ShowCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }
};

template <>
struct Authoriser<ZombieCmd>
{
    static authorisation_t accept(const ZombieCmd& command, const RequestContext& context) {
        return allows_as_per_read_write_rules(command, context);
    }

    static void paths(const ZombieCmd& command, std::vector<std::string>& paths) { select_all_paths(command, paths); }
};

template <typename... COMMANDS>
struct Apply
{
    template <typename F>
    static void to(const Cmd_ptr& cmd, F f) {
        (
            [&] {
                if (const auto* c = dynamic_cast<const COMMANDS*>(cmd.get()); c != nullptr) {
                    f(*c);
                }
            }(),
            ...);
    }
};

template <>
struct Authoriser<GroupCTSCmd>
{
    using Apply_t = Apply<AbortCmd,
                          AlterCmd,
                          BeginCmd,
                          CFileCmd,
                          CSyncCmd,
                          CheckPtCmd,
                          ClientHandleCmd,
                          CompleteCmd,
                          CtsCmd,
                          CtsNodeCmd,
                          CtsWaitCmd,
                          DeleteCmd,
                          EditScriptCmd,
                          EventCmd,
                          ForceCmd,
                          FreeDepCmd,
                          GroupCTSCmd,
                          InitCmd,
                          LabelCmd,
                          LoadDefsCmd,
                          LogCmd,
                          LogMessageCmd,
                          MeterCmd,
                          MoveCmd,
                          OrderNodeCmd,
                          PathsCmd,
                          PlugCmd,
                          QueryCmd,
                          QueueCmd,
                          ReplaceNodeCmd,
                          RequeueNodeCmd,
                          RunNodeCmd,
                          ServerVersionCmd,
                          ShowCmd,
                          ZombieCmd>;

    static authorisation_t accept(const GroupCTSCmd& command, const RequestContext& context) {
        // In Permissions mode, each sub-command is authorised just before it runs (D5.27); see GroupCTSCmd
        if (authorisation_mode(context.server()) == AuthorisationMode::Permissions) {
            return authorisation_t::success("Authorisation (user) deferred to the sub-commands (group command)");
        }

        // Otherwise, as before, a group is allowed only when all its sub-commands are
        for (auto& cmd : command.cmdVec()) {
            authorisation_t found =
                authorisation_t::failure("Authorisation (user) failed, due to: Insufficient permissions");

            Apply_t::to(cmd, [&](auto&& c) {
                if (auto&& result = allows_as_per_read_write_rules(c, context); result.ok()) {
                    found = result;
                }
            });

            if (!found.ok()) {
                return found;
            }
        }

        return authorisation_t::success("Authorisation (user) granted (group command)");
    }

    static void paths(const GroupCTSCmd& command, std::vector<std::string>& paths) {
        for (auto&& cmd : command.cmdVec()) {
            Apply_t::to(cmd, [&](auto&& c) { accumulate_paths(c, paths); });
        }
    }
};

} // namespace implementation

} // namespace ecf
