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
#include "ecflow/node/Defs.hpp"
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

template <typename COMMAND>
authorisation_t allows_as_per_read_write_rules(const COMMAND& command, AbstractServer& server) {
    static_assert(std::is_base_of_v<TaskCmd, COMMAND> || std::is_base_of_v<UserCmd, COMMAND>,
                  "The command must be either a TaskCmd or a UserCmd");

    if constexpr (std::is_base_of_v<TaskCmd, COMMAND>) {
        // No actual verification is done for task commands
        return authorisation_t::success("Authorisation (task) granted");
    }

    std::vector<std::string> paths = get_affected_paths(command);

    auto required    = Authoriser<COMMAND>::required(command);
    const Defs& defs = *server.defs();
    if (server.authorisation().allows(command.identity(), defs, paths, required)) {
        return authorisation_t::success("Authorisation (user) granted");
    }

    return authorisation_t::failure("Authorisation (user) failed, due to: Insufficient permissions");
}

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

    static void paths(const InitCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }

    static Allowed required(const InitCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<CompleteCmd>
{
    static authorisation_t accept(const CompleteCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const CompleteCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }

    static Allowed required(const CompleteCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<AbortCmd>
{
    static authorisation_t accept(const AbortCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const AbortCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }

    static Allowed required(const AbortCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<LabelCmd>
{
    static authorisation_t accept(const LabelCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const LabelCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }

    static Allowed required(const LabelCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<MeterCmd>
{
    static authorisation_t accept(const MeterCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const MeterCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }

    static Allowed required(const MeterCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<EventCmd>
{
    static authorisation_t accept(const EventCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const EventCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }

    static Allowed required(const EventCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<QueueCmd>
{
    static authorisation_t accept(const QueueCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const QueueCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }

    static Allowed required(const QueueCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<CtsWaitCmd>
{
    static authorisation_t accept(const CtsWaitCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const CtsWaitCmd& command, std::vector<std::string>& paths) { /* Nothing to do... */ }

    static Allowed required(const CtsWaitCmd&) { return Allowed::WRITE; }
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
    static authorisation_t accept(const AlterCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const AlterCmd& command, std::vector<std::string>& paths) { select_all_paths(command, paths); }

    static Allowed required(const AlterCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<BeginCmd>
{
    static authorisation_t accept(const BeginCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const BeginCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }

    static Allowed required(const BeginCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<CFileCmd>
{
    static authorisation_t accept(const CFileCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const CFileCmd& command, std::vector<std::string>& paths) { select_node_path(command, paths); }

    static Allowed required(const CFileCmd&) { return Allowed::READ; }
};

template <>
struct Authoriser<CheckPtCmd>
{
    static authorisation_t accept(const CheckPtCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const CheckPtCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }

    static Allowed required(const CheckPtCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<ClientHandleCmd>
{
    static authorisation_t accept(const ClientHandleCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const ClientHandleCmd& command, std::vector<std::string>& paths) {
        select_root_path(command, paths);
    }

    static Allowed required(const ClientHandleCmd&) {
        // Todo[MB]: Check the correct choice for Suite handle commands
        return Allowed::READ;
    }
};

template <>
struct Authoriser<CSyncCmd>
{
    static authorisation_t accept(const CSyncCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const CSyncCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }

    static Allowed required(const CSyncCmd&) { return Allowed::READ; }
};

template <>
struct Authoriser<CtsCmd>
{
    static authorisation_t accept(const CtsCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const CtsCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }

    static Allowed required(const CtsCmd& cmd) {
        // Todo[MB]: Check the correct choice for "multi-Action" commands, as it really depends on the value of the
        // action
        return cmd.isWrite() ? Allowed::WRITE : Allowed::READ;
    }
};

template <>
struct Authoriser<CtsNodeCmd>
{
    static authorisation_t accept(const CtsNodeCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const CtsNodeCmd& command, std::vector<std::string>& paths) {
        if (auto&& path = command.pathToNode(); path.empty()) {
            select_root_path(command, paths);
        }
        else {
            paths.push_back(path);
        }
    }

    static Allowed required(const CtsNodeCmd& cmd) {
        // Todo[MB]: Check the correct choice for "multi-Action" commands, as it really depends on the value of the
        // action
        return cmd.isWrite() ? Allowed::WRITE : Allowed::READ;
    }
};

template <>
struct Authoriser<DeleteCmd>
{
    static authorisation_t accept(const DeleteCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const DeleteCmd& command, std::vector<std::string>& paths) {
        if (auto&& affected = command.paths(); affected.empty()) {
            paths.push_back("/");
        }
        else {
            paths.push_back(affected[0]);
        }
    }

    static Allowed required(const DeleteCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<EditScriptCmd>
{
    static authorisation_t accept(const EditScriptCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const EditScriptCmd& command, std::vector<std::string>& paths) {
        select_node_path(command, paths);
    }

    static Allowed required(const EditScriptCmd& cmd) {
        // Todo[MB]: Check the correct choice for "multi-Action" commands, as it really depends on the value of the
        // action
        return cmd.isWrite() ? Allowed::WRITE : Allowed::READ;
    }
};

template <>
struct Authoriser<ForceCmd>
{
    static authorisation_t accept(const ForceCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const ForceCmd& command, std::vector<std::string>& paths) { select_all_paths(command, paths); }

    static Allowed required(const ForceCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<FreeDepCmd>
{
    static authorisation_t accept(const FreeDepCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const FreeDepCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }

    static Allowed required(const FreeDepCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<LoadDefsCmd>
{
    static authorisation_t accept(const LoadDefsCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const LoadDefsCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }

    static Allowed required(const LoadDefsCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<LogCmd>
{
    static authorisation_t accept(const LogCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const LogCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }

    static Allowed required(const LogCmd& cmd) {
        // Todo[MB]: Check the correct choice for "multi-Action" commands, as it really depends on the value of the
        // action
        return cmd.isWrite() ? Allowed::WRITE : Allowed::READ;
    }
};

template <>
struct Authoriser<LogMessageCmd>
{
    static authorisation_t accept(const LogMessageCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const LogMessageCmd& command, std::vector<std::string>& paths) {
        select_root_path(command, paths);
    }

    static Allowed required(const LogMessageCmd& cmd) {
        // Todo[MB]: Check the correct choice for "Logging a Message" command
        return Allowed::READ;
    }
};

template <>
struct Authoriser<MoveCmd>
{
    static authorisation_t accept(const MoveCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const MoveCmd& command, std::vector<std::string>& paths) { paths.push_back(command.src_path()); }

    static Allowed required(const MoveCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<OrderNodeCmd>
{
    static authorisation_t accept(const OrderNodeCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const OrderNodeCmd& command, std::vector<std::string>& paths) {
        select_node_path(command, paths);
    }

    static Allowed required(const OrderNodeCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<PathsCmd>
{
    static authorisation_t accept(const PathsCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const PathsCmd& command, std::vector<std::string>& paths) { select_all_paths(command, paths); }

    static Allowed required(const PathsCmd& cmd) {
        // Todo[MB]: Check the correct choice for "multi-Action" commands, as it really depends on the value of the
        // action
        return cmd.isWrite() ? Allowed::WRITE : Allowed::READ;
    }
};

template <>
struct Authoriser<PlugCmd>
{
    static authorisation_t accept(const PlugCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const PlugCmd& command, std::vector<std::string>& paths) { paths.push_back(command.source()); }

    static Allowed required(const PlugCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<QueryCmd>
{
    static authorisation_t accept(const QueryCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const QueryCmd& command, std::vector<std::string>& paths) {
        paths.push_back(command.path_to_attribute());
    }

    static Allowed required(const QueryCmd&) { return Allowed::READ; }
};

template <>
struct Authoriser<ReplaceNodeCmd>
{
    static authorisation_t accept(const ReplaceNodeCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const ReplaceNodeCmd& command, std::vector<std::string>& paths) {
        select_node_path(command, paths);
    }

    static Allowed required(const ReplaceNodeCmd&) { return Allowed::WRITE; }
};

template <>
struct Authoriser<RequeueNodeCmd>
{
    static authorisation_t accept(const RequeueNodeCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const RequeueNodeCmd& command, std::vector<std::string>& paths) {
        select_all_paths(command, paths);
    }

    static Allowed required(const RequeueNodeCmd&) { return Allowed::WRITE | Allowed::EXECUTE; }
};

template <>
struct Authoriser<RunNodeCmd>
{
    static authorisation_t accept(const RunNodeCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const RunNodeCmd& command, std::vector<std::string>& paths) { select_all_paths(command, paths); }

    static Allowed required(const RunNodeCmd&) { return Allowed::WRITE | Allowed::EXECUTE; }
};

template <>
struct Authoriser<ServerVersionCmd>
{
    static authorisation_t accept(const ServerVersionCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const ServerVersionCmd& command, std::vector<std::string>& paths) {
        select_root_path(command, paths);
    }

    static Allowed required(const ServerVersionCmd&) { return Allowed::READ; }
};

template <>
struct Authoriser<ShowCmd>
{
    static authorisation_t accept(const ShowCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const ShowCmd& command, std::vector<std::string>& paths) { select_root_path(command, paths); }

    static Allowed required(const ShowCmd&) { return Allowed::READ; }
};

template <>
struct Authoriser<ZombieCmd>
{
    static authorisation_t accept(const ZombieCmd& command, AbstractServer& server) {
        return allows_as_per_read_write_rules(command, server);
    }

    static void paths(const ZombieCmd& command, std::vector<std::string>& paths) { select_all_paths(command, paths); }

    static Allowed required(const ZombieCmd&) {
        // Todo[MB]: Check the correct choice for Zombie related commands
        return Allowed::READ;
    }
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

    static authorisation_t accept(const GroupCTSCmd& command, AbstractServer& server) {
        for (auto& cmd : command.cmdVec()) {
            authorisation_t found =
                authorisation_t::failure("Authorisation (user) failed, due to: Insufficient permissions");

            Apply_t::to(cmd, [&](auto&& c) {
                if (auto&& result = allows_as_per_read_write_rules(c, server); result.ok()) {
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

    static Allowed required(const GroupCTSCmd& command) {
        // Todo[MB]: Confirm that groups are always allowed, since each individual commands will be checked anyway
        return Allowed::READ | Allowed::WRITE | Allowed::EXECUTE | Allowed::OWNER;
    }
};

} // namespace implementation

} // namespace ecf

#endif /* ecflow_base_AuthorisationDetails_HPP */
