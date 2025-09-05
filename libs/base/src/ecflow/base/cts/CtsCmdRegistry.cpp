/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/CtsCmdRegistry.hpp"

#include <memory>

#include "ecflow/base/AbstractClientEnv.hpp"
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

namespace po = boost::program_options;

CtsCmdRegistry::CtsCmdRegistry(bool addGroupCmd) {
    // If a new client to server command is added. Make sure to add it here.
    // Could have used static initialisation' but this is less problematic.

    //
    // In order to improve server responsiveness, the commands which are more
    // often required or need the quickest response appear first.
    //
    // The order of the following 'list' dictates how the --help is shown.
    //
    vec_.reserve(90);

    vec_.push_back(std::make_shared<CSyncCmd>(CSyncCmd::NEWS, 0, 0, 0));
    vec_.push_back(std::make_shared<CSyncCmd>(CSyncCmd::SYNC, 0, 0, 0));
    vec_.push_back(std::make_shared<CSyncCmd>(0)); // SYNC_FULL
    vec_.push_back(std::make_shared<CSyncCmd>(CSyncCmd::SYNC_CLOCK, 0, 0, 0));
    vec_.push_back(std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET));
    vec_.push_back(std::make_shared<CtsNodeCmd>(CtsNodeCmd::GET_STATE));
    vec_.push_back(std::make_shared<CtsNodeCmd>(CtsNodeCmd::MIGRATE));
    vec_.push_back(std::make_shared<CheckPtCmd>());
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::PING));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::RESTORE_DEFS_FROM_CHECKPT));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::RESTART_SERVER));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::HALT_SERVER));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::SHUTDOWN_SERVER));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::TERMINATE_SERVER));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::RELOAD_WHITE_LIST_FILE));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::RELOAD_PASSWD_FILE));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::RELOAD_CUSTOM_PASSWD_FILE));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::FORCE_DEP_EVAL));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::STATS));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::STATS_SERVER));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::STATS_RESET));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::DEBUG_SERVER_ON));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::DEBUG_SERVER_OFF));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::SERVER_LOAD));
    vec_.push_back(std::make_shared<CtsNodeCmd>(CtsNodeCmd::JOB_GEN));
    vec_.push_back(std::make_shared<CtsNodeCmd>(CtsNodeCmd::CHECK_JOB_GEN_ONLY));
    vec_.push_back(std::make_shared<DeleteCmd>());
    vec_.push_back(std::make_shared<PathsCmd>(PathsCmd::SUSPEND));
    vec_.push_back(std::make_shared<PathsCmd>(PathsCmd::RESUME));
    vec_.push_back(std::make_shared<PathsCmd>(PathsCmd::KILL));
    vec_.push_back(std::make_shared<PathsCmd>(PathsCmd::STATUS));
    vec_.push_back(std::make_shared<PathsCmd>(PathsCmd::CHECK));
    vec_.push_back(std::make_shared<PathsCmd>(PathsCmd::EDIT_HISTORY));
    vec_.push_back(std::make_shared<PathsCmd>(PathsCmd::ARCHIVE));
    vec_.push_back(std::make_shared<PathsCmd>(PathsCmd::RESTORE));
    vec_.push_back(std::make_shared<ZombieCmd>(ecf::ZombieCtrlAction::FOB));
    vec_.push_back(std::make_shared<ZombieCmd>(ecf::ZombieCtrlAction::FAIL));
    vec_.push_back(std::make_shared<ZombieCmd>(ecf::ZombieCtrlAction::ADOPT));
    vec_.push_back(std::make_shared<ZombieCmd>(ecf::ZombieCtrlAction::BLOCK));
    vec_.push_back(std::make_shared<ZombieCmd>(ecf::ZombieCtrlAction::REMOVE));
    vec_.push_back(std::make_shared<ZombieCmd>(ecf::ZombieCtrlAction::KILL));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::GET_ZOMBIES));
    vec_.push_back(std::make_shared<CtsCmd>(CtsCmd::SUITES));
    vec_.push_back(std::make_shared<ClientHandleCmd>(ClientHandleCmd::REGISTER));
    vec_.push_back(std::make_shared<ClientHandleCmd>(ClientHandleCmd::DROP));
    vec_.push_back(std::make_shared<ClientHandleCmd>(ClientHandleCmd::DROP_USER));
    vec_.push_back(std::make_shared<ClientHandleCmd>(ClientHandleCmd::ADD));
    vec_.push_back(std::make_shared<ClientHandleCmd>(ClientHandleCmd::REMOVE));
    vec_.push_back(std::make_shared<ClientHandleCmd>(ClientHandleCmd::AUTO_ADD));
    vec_.push_back(std::make_shared<ClientHandleCmd>(ClientHandleCmd::SUITES));
    vec_.push_back(std::make_shared<LogCmd>());
    vec_.push_back(std::make_shared<ServerVersionCmd>());
    vec_.push_back(std::make_shared<LogMessageCmd>());
    vec_.push_back(std::make_shared<BeginCmd>());
    vec_.push_back(std::make_shared<InitCmd>());
    vec_.push_back(std::make_shared<CompleteCmd>());
    vec_.push_back(std::make_shared<AbortCmd>());
    vec_.push_back(std::make_shared<CtsWaitCmd>());
    vec_.push_back(std::make_shared<EventCmd>());
    vec_.push_back(std::make_shared<MeterCmd>());
    vec_.push_back(std::make_shared<LabelCmd>());
    vec_.push_back(std::make_shared<QueueCmd>());
    vec_.push_back(std::make_shared<RequeueNodeCmd>());
    vec_.push_back(std::make_shared<OrderNodeCmd>());
    vec_.push_back(std::make_shared<RunNodeCmd>());
    vec_.push_back(std::make_shared<ForceCmd>());
    vec_.push_back(std::make_shared<FreeDepCmd>());
    vec_.push_back(std::make_shared<LoadDefsCmd>());
    vec_.push_back(std::make_shared<ReplaceNodeCmd>());
    vec_.push_back(std::make_shared<CFileCmd>());
    vec_.push_back(std::make_shared<EditScriptCmd>());
    vec_.push_back(std::make_shared<AlterCmd>());
    vec_.push_back(std::make_shared<QueryCmd>());
    vec_.push_back(std::make_shared<PlugCmd>());
    // Note: we deliberately do not add MoveCmd, as it should not appear in the public api
    //       It is created on the fly by the PlugCmd

    /// Command that can *ONLY* be used in a group command
    vec_.push_back(std::make_shared<CtsNodeCmd>(CtsNodeCmd::WHY));
    vec_.push_back(std::make_shared<ShowCmd>());
    if (addGroupCmd) {
        vec_.push_back(std::make_shared<GroupCTSCmd>());
    }
}

bool CtsCmdRegistry::parse(Cmd_ptr& cmd,
                           boost::program_options::variables_map& vm,
                           AbstractClientEnv* clientEnv) const {
    for (const auto& registered_cmd : vec_) {

        if (vm.count(registered_cmd->theArg())) {

            if (clientEnv->debug()) {
                std::cout << "  CtsCmdRegistry::parse matched with registered command " << registered_cmd->theArg()
                          << "\n";
            }

            registered_cmd->create(cmd, vm, clientEnv);
            return true;
        }
    }

    return false;
}

void CtsCmdRegistry::addAllOptions(boost::program_options::options_description& desc) const {
    addCmdOptions(desc);
    addHelpOption(desc);
}

void CtsCmdRegistry::addCmdOptions(boost::program_options::options_description& desc) const {
    for (const auto& registered_cmd : vec_) {
        registered_cmd->addOption(desc);
    }
}

void CtsCmdRegistry::addHelpOption(boost::program_options::options_description& desc) const {
    /////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
    desc.add_options()("help,h", po::value<std::string>()->implicit_value(std::string("")), "Produce help message");
    desc.add_options()("version,v", "Show ecflow client version number, and version of the boost library used");
    desc.add_options()("debug,d",
                       "Enables the display of client environment settings and execution details.\n"
                       "Has the same effect as setting environment variable ECF_DEBUG_CLIENT.");
}
