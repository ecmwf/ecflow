/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_AuthorisationDetails_HPP
#define ecflow_base_AuthorisationDetails_HPP

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/Authorisation.hpp"
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

template <typename COMMAND>
authorisation_t allows_as_per_read_write_rules(const COMMAND& command, AbstractServer& server) {
    auto base = dynamic_cast<BaseServer*>(&server);

    std::vector<std::string> paths;
    if constexpr (std::is_base_of_v<TaskCmd, COMMAND>) {
        // No actual verification is done for task commands
        return authorisation_t::success("Authorisation (task) granted");
    }
    else if constexpr (std::is_base_of_v<UserCmd, COMMAND>) {

        if constexpr (std::is_same_v<AlterCmd, COMMAND> || std::is_same_v<DeleteCmd, COMMAND> ||
                      std::is_same_v<ForceCmd, COMMAND> || std::is_same_v<PathsCmd, COMMAND> ||
                      std::is_same_v<RequeueNodeCmd, COMMAND> || std::is_same_v<RunNodeCmd, COMMAND> ||
                      std::is_same_v<ZombieCmd, COMMAND>) {
            auto&& p = command.paths();
            for (auto& path : p) {
                paths.push_back(path);
            }
        }
        else if constexpr (std::is_same_v<BeginCmd, COMMAND> || std::is_same_v<CheckPtCmd, COMMAND> ||
                           std::is_same_v<ClientHandleCmd, COMMAND> || std::is_same_v<CSyncCmd, COMMAND> ||
                           std::is_same_v<CtsCmd, COMMAND> || std::is_same_v<FreeDepCmd, COMMAND> ||
                           std::is_same_v<GroupCTSCmd, COMMAND> || std::is_same_v<LoadDefsCmd, COMMAND> ||
                           std::is_same_v<LogCmd, COMMAND> || std::is_same_v<LogMessageCmd, COMMAND> ||
                           std::is_same_v<ServerVersionCmd, COMMAND> || std::is_same_v<ShowCmd, COMMAND>) {
            paths.push_back("/");
        }
        else if constexpr (std::is_same_v<CFileCmd, COMMAND> || std::is_same_v<ReplaceNodeCmd, COMMAND>) {
            paths.push_back(command.pathToNode());
        }
        else if constexpr (std::is_same_v<EditScriptCmd, COMMAND>) {
            paths.push_back(command.path_to_node());
        }
        else if constexpr (std::is_same_v<QueryCmd, COMMAND>) {
            paths.push_back(command.path_to_task());
        }
        else if constexpr (std::is_same_v<MoveCmd, COMMAND>) {
            paths.push_back(command.src_node());
            paths.push_back(command.dest());
        }
        else if constexpr (std::is_same_v<PlugCmd, COMMAND>) {
            paths.push_back(command.source());
            paths.push_back(command.dest());
        }
        else {
            auto path = command.absNodePath() == "" ? "/" : command.absNodePath();
            paths.push_back(path);
        }
    }
    else {
        static_assert(std::is_base_of_v<TaskCmd, COMMAND> || std::is_base_of_v<UserCmd, COMMAND>,
                      "The command must be either a TaskCmd or a UserCmd");
    }

    const std::string permission = command.isWrite() ? "write" : "read";
    if (base->permissions().allows(command.identity(), paths, permission)) {
        return authorisation_t::success("Authorisation (user) granted");
    }

    return authorisation_t::failure("Authorisation (user) failed, due to: Insufficient permissions");
}

// The Authoriser struct is a template that provides a static accept() function

template <typename COMMAND>
struct Authoriser
{
};

template <typename COMMAND>
authorisation_t do_authorise(const COMMAND& command, AbstractServer& server) {
    return Authoriser<COMMAND>::accept(command, server);
}

// Task commands

template <>
struct Authoriser<InitCmd>
{
    static authorisation_t accept(const InitCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<CompleteCmd>
{
    static authorisation_t accept(const CompleteCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<AbortCmd>
{
    static authorisation_t accept(const AbortCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<LabelCmd>
{
    static authorisation_t accept(const LabelCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<MeterCmd>
{
    static authorisation_t accept(const MeterCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<EventCmd>
{
    static authorisation_t accept(const EventCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<QueueCmd>
{
    static authorisation_t accept(const QueueCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

// User commands

template <>
struct Authoriser<AlterCmd>
{
    static authorisation_t accept(const AlterCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<BeginCmd>
{
    static authorisation_t accept(const BeginCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<CFileCmd>
{
    static authorisation_t accept(const CFileCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<CheckPtCmd>
{
    static authorisation_t accept(const CheckPtCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<ClientHandleCmd>
{
    static authorisation_t accept(const ClientHandleCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<CSyncCmd>
{
    static authorisation_t accept(const CSyncCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<CtsCmd>
{
    static authorisation_t accept(const CtsCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<CtsNodeCmd>
{
    static authorisation_t accept(const CtsNodeCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<CtsWaitCmd>
{
    static authorisation_t accept(const CtsWaitCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<DeleteCmd>
{
    static authorisation_t accept(const DeleteCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<EditScriptCmd>
{
    static authorisation_t accept(const EditScriptCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<ForceCmd>
{
    static authorisation_t accept(const ForceCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<FreeDepCmd>
{
    static authorisation_t accept(const FreeDepCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<GroupCTSCmd>
{
    static authorisation_t accept(const GroupCTSCmd& command, AbstractServer& server) {
        for (auto& cmd : command.cmdVec()) {
            if (auto result = cmd->authorise(server); !result.ok()) {
                return result;
            }
        }
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<LoadDefsCmd>
{
    static authorisation_t accept(const LoadDefsCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<LogCmd>
{
    static authorisation_t accept(const LogCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<LogMessageCmd>
{
    static authorisation_t accept(const LogMessageCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<MoveCmd>
{
    static authorisation_t accept(const MoveCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<OrderNodeCmd>
{
    static authorisation_t accept(const OrderNodeCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<PathsCmd>
{
    static authorisation_t accept(const PathsCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<PlugCmd>
{
    static authorisation_t accept(const PlugCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<QueryCmd>
{
    static authorisation_t accept(const QueryCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<ReplaceNodeCmd>
{
    static authorisation_t accept(const ReplaceNodeCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<RequeueNodeCmd>
{
    static authorisation_t accept(const RequeueNodeCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<RunNodeCmd>
{
    static authorisation_t accept(const RunNodeCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<ServerVersionCmd>
{
    static authorisation_t accept(const ServerVersionCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<ShowCmd>
{
    static authorisation_t accept(const ShowCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

template <>
struct Authoriser<ZombieCmd>
{
    static authorisation_t accept(const ZombieCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }
};

// Entry point for the accept() function

} // namespace implementation

} // namespace ecf

#endif /* ecflow_base_AuthorisationDetails_HPP */
