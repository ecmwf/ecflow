/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_AuthenticationDetails_HPP
#define ecflow_base_AuthenticationDetails_HPP

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/Authentication.hpp"
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

inline authentication_t verify_task_authentication_rules(AbstractServer& server, const TaskCmd& command) {
    // Important:
    //      A running task is currently not really authenticated!
    //
    //   The running tasks provide information (pid, pass, tryno) which is used to detect the presence of Zombies,
    //   but there is no "user account" that is used to authenticate the exchange.
    //

    return authentication_t::success("Authentication (task command) successful");
}

inline authentication_t verify_user_authentication_rules(const AbstractServer& server, const UserCmd& command) {
    const auto& service = server.authentication();
    if (service.is_authentic(command.identity())) {
        return authentication_t::success("Authentication (user command) successful");
    }

    return authentication_t::failure("Authentication (user command) failed, due to: Incorrect credentials for (" +
                                     command.identity().username().value() + " / <ommitted>) is not authentic");
}

template <typename COMMAND>
authentication_t verify_authentication_rules(const COMMAND& command, AbstractServer& server) {
    if constexpr (std::is_base_of_v<UserCmd, COMMAND>) {
        return verify_user_authentication_rules(server, command);
    }
    else if constexpr (std::is_base_of_v<TaskCmd, COMMAND>) {
        return verify_task_authentication_rules(server, command);
    }
    else {
        static_assert(std::is_base_of_v<TaskCmd, COMMAND> || std::is_base_of_v<UserCmd, COMMAND>,
                      "The command must be either a TaskCmd or a UserCmd");
    }
    return authentication_t::failure("Authentication failed, due to: Unexpected command type");
}

// The Authenticator struct is a template that provides a static accept() function

template <typename COMMAND>
struct Authenticator
{
};

template <typename COMMAND>
authentication_t do_authenticate(const COMMAND& command, AbstractServer& server) {
    return Authenticator<COMMAND>::accept(command, server);
}

// Task commands

template <>
struct Authenticator<InitCmd>
{
    static authentication_t accept(const InitCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<CompleteCmd>
{
    static authentication_t accept(const CompleteCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<AbortCmd>
{
    static authentication_t accept(const AbortCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<LabelCmd>
{
    static authentication_t accept(const LabelCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<MeterCmd>
{
    static authentication_t accept(const MeterCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<EventCmd>
{
    static authentication_t accept(const EventCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<QueueCmd>
{
    static authentication_t accept(const QueueCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<CtsWaitCmd>
{
    static authentication_t accept(const CtsWaitCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

// User commands

template <>
struct Authenticator<AlterCmd>
{
    static authentication_t accept(const AlterCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<BeginCmd>
{
    static authentication_t accept(const BeginCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<CFileCmd>
{
    static authentication_t accept(const CFileCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<CheckPtCmd>
{
    static authentication_t accept(const CheckPtCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<ClientHandleCmd>
{
    static authentication_t accept(const ClientHandleCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<CSyncCmd>
{
    static authentication_t accept(const CSyncCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<CtsCmd>
{
    static authentication_t accept(const CtsCmd& command, AbstractServer& server) {
        if (command.api() == CtsCmd::Api::PING) { // No authentication required for Ping command
            return authentication_t::success("Authentication successful");
        }
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<CtsNodeCmd>
{
    static authentication_t accept(const CtsNodeCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<DeleteCmd>
{
    static authentication_t accept(const DeleteCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<EditScriptCmd>
{
    static authentication_t accept(const EditScriptCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<ForceCmd>
{
    static authentication_t accept(const ForceCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<FreeDepCmd>
{
    static authentication_t accept(const FreeDepCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<GroupCTSCmd>
{
    static authentication_t accept(const GroupCTSCmd& command, AbstractServer& server) {
        for (auto& cmd : command.cmdVec()) {
            if (auto result = cmd->authenticate(server); !result.ok()) {
                return result;
            }
        }
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<LoadDefsCmd>
{
    static authentication_t accept(const LoadDefsCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<LogCmd>
{
    static authentication_t accept(const LogCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<LogMessageCmd>
{
    static authentication_t accept(const LogMessageCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<MoveCmd>
{
    static authentication_t accept(const MoveCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<OrderNodeCmd>
{
    static authentication_t accept(const OrderNodeCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<PathsCmd>
{
    static authentication_t accept(const PathsCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<PlugCmd>
{
    static authentication_t accept(const PlugCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<QueryCmd>
{
    static authentication_t accept(const QueryCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<ReplaceNodeCmd>
{
    static authentication_t accept(const ReplaceNodeCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<RequeueNodeCmd>
{
    static authentication_t accept(const RequeueNodeCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<RunNodeCmd>
{
    static authentication_t accept(const RunNodeCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<ServerVersionCmd>
{
    static authentication_t accept(const ServerVersionCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<ShowCmd>
{
    static authentication_t accept(const ShowCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

template <>
struct Authenticator<ZombieCmd>
{
    static authentication_t accept(const ZombieCmd& command, AbstractServer& server) {
        return verify_authentication_rules(command, server);
    }
};

} // namespace implementation

} // namespace ecf

#endif /* ecflow_base_AuthenticationDetails_HPP */
