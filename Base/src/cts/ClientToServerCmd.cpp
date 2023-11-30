/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ClientToServerCmd.hpp"

#include <iostream>
#include <stdexcept>

#include "AbstractServer.hpp"
#include "CmdContext.hpp"
#include "Defs.hpp"
#include "EditHistoryMgr.hpp"
#include "Flag.hpp"
#include "Node.hpp"
#include "ServerToClientCmd.hpp"
#include "SuiteChanged.hpp"
#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/Log.hpp"

using namespace std;
using namespace boost;
using namespace ecf;

// #define DEBUG_INVARIANTS 1

ClientToServerCmd::ClientToServerCmd() : cl_host_(Host().name()) {
}

ClientToServerCmd::~ClientToServerCmd() = default;

bool ClientToServerCmd::equals(ClientToServerCmd* rhs) const {
    return hostname() == rhs->hostname();
}

STC_Cmd_ptr ClientToServerCmd::handleRequest(AbstractServer* as) const {
    // Allow creating of new time stamp, when *not* in a command. i.e during node tree traversal in server
    CmdContext cmdContext;

    // Automatically flush log file at the end of the command
    LogFlusher logFlusher;

    // Create the log time stamp once for a given request
    Log::instance()->cache_time_stamp();

    // LOG the command, *BEFORE* invoking it. (i.e in case server hangs/crashes)
    // Allow override in the rare cases, where we want to output additional debug
    // If logging fails set late flag to warn users, ECFLOW-536
    do_log(as);

#ifdef DEBUG_INVARIANTS
    LOG_ASSERT(as->defs(), "ClientToServerCmd::handleRequest: Start:  No defs? ");
    std::string errmsg;
    if (!as->defs()->checkInvariants(errmsg)) {
        LOG(Log::ERR, "ClientToServerCmd::handleRequest: PreCondition: Failed invariant checking:" << errmsg);
    }
#endif

    // LogTimer timer("ClientToServerCmd::handleRequest");
    STC_Cmd_ptr halted;
    if (!authenticate(as, halted)) {
        assert(halted.get());
        return halted;
    }

    // mark edited nodes, with edit history. relies on doHandleRequest to populate edit_history_nodes_/paths
    // hence must at the same scope level
    EditHistoryMgr edit_history_mgr(this, as);

    // Handle the request, and return the reply back to the client
    STC_Cmd_ptr server_to_client_ptr = doHandleRequest(as);
    if (isWrite() && server_to_client_ptr->ok()) {

        // This can end up, check pointing the defs file
        as->nodeTreeStateChanged();
    }

#ifdef DEBUG_INVARIANTS
    LOG_ASSERT(as->defs(), "ClientToServerCmd::handleRequest: End:  No defs? ");
    std::string errmsg;
    if (!as->defs()->checkInvariants(errmsg)) {
        LOG(Log::ERR, "ClientToServerCmd::handleRequest: PostCondition: Failed invariant checking:" << errmsg);
    }
#endif

    return server_to_client_ptr;
}

void ClientToServerCmd::do_log(AbstractServer* as) const {
    std::string ss;
    print(ss);                // Populate the stream with command details:
    if (!log(Log::MSG, ss)) { // will automatically add end of line
        // problems with opening or writing to log file, warn users, ECFLOW-536
        as->defs()->flag().set(ecf::Flag::LOG_ERROR);
        as->defs()->set_server().add_or_update_user_variables("ECF_LOG_ERROR", Log::instance()->log_error());
    }
}

STC_Cmd_ptr ClientToServerCmd::doJobSubmission(AbstractServer* as) {
    // This function could be called at the end of *USER* command that can change state.
    //
    // We could have other tasks/jobs dependent on the state change. i.e end of time series
    // This will traverse the node tree and resolve dependencies and may force
    // Other jobs to be generated, due to the state change.
    // However *** errors in job submission ***, should *NOT* typically abort the command.
    // Since we will typically just set task to aborted state

    // This job generation will timeout if job generation takes longer than next poll time.
    as->traverse_node_tree_and_job_generate(Calendar::second_clock_time(), true /* user cmd context */);

    return PreAllocatedReply::ok_cmd();
}

node_ptr ClientToServerCmd::find_node(Defs* defs, const std::string& absNodepath) const {
    node_ptr theNode = defs->findAbsNode(absNodepath);
    if (!theNode.get()) {

        std::string errorMsg = "Cannot find node at path '";
        errorMsg += absNodepath;
        errorMsg += "' ";
        throw std::runtime_error(errorMsg);
    }
    return theNode;
}

void ClientToServerCmd::dumpVecArgs(const char* argOption, const std::vector<std::string>& args) {
    cout << "  " << argOption;
    for (size_t i = 0; i < args.size(); i++) {
        cout << " args[" << i << "]='" << args[i] << "'";
    }
    cout << "\n";
}

node_ptr ClientToServerCmd::find_node_for_edit(Defs* defs, const std::string& absNodepath) const {
    node_ptr theNode = find_node(defs, absNodepath);
    add_node_for_edit_history(theNode);
    return theNode;
}

node_ptr ClientToServerCmd::find_node_for_edit_no_throw(Defs* defs, const std::string& absNodepath) const {
    node_ptr theNode = defs->findAbsNode(absNodepath);
    add_node_for_edit_history(theNode);
    return theNode;
}

void ClientToServerCmd::add_node_for_edit_history(Defs* defs, const std::string& absNodepath) const {
    add_node_for_edit_history(defs->findAbsNode(absNodepath));
}

void ClientToServerCmd::add_node_for_edit_history(node_ptr the_node) const {
    if (the_node.get())
        edit_history_nodes_.push_back(the_node);
}

void ClientToServerCmd::add_node_path_for_edit_history(const std::string& absNodepath) const {
    edit_history_node_paths_.push_back(absNodepath);
}

void ClientToServerCmd::add_edit_history(Defs* defs) const {
    if (!use_EditHistoryMgr_) {
        return; // edit history will be added by the command
    }

    // record all the user edits to the node. Reuse the time stamp cache created in handleRequest()
    if (edit_history_nodes_.empty() && edit_history_node_paths_.empty()) {

        defs->flag().set(ecf::Flag::MESSAGE);
        add_edit_history(defs, Str::ROOT_PATH());
    }
    else {
        // edit_history_node_paths_ is only populated by the delete command
        size_t the_size = edit_history_node_paths_.size();
        if (the_size != 0)
            defs->flag().set(ecf::Flag::MESSAGE);
        for (size_t i = 0; i < the_size; i++) {
            add_delete_edit_history(defs, edit_history_node_paths_[i]);
        }

        the_size = edit_history_nodes_.size();
        for (size_t i = 0; i < the_size; i++) {
            node_ptr edited_node = edit_history_nodes_[i].lock();
            if (edited_node.get()) {
                // Setting the flag will make a state change. But its OK command allows it.
                // Since we only get called if command can make state changes (isWrite() == true)
                SuiteChangedPtr suiteChanged(edited_node.get());
                edited_node->flag().set(ecf::Flag::MESSAGE); // trap state change in suite for sync
                add_edit_history(defs, edited_node->absNodePath());
            }
        }
    }

    edit_history_nodes_.clear();
    edit_history_node_paths_.clear();
}

void ClientToServerCmd::add_edit_history(Defs* defs, const std::string& path) const {
    // Note: if the cts_cmd_, had thousands of paths, calling  cts_cmd_->print(ss); will append those paths to the
    //       output, HUGE performance bottle neck, Since we are recording what command was applied to each node
    //       we ONLY need the single path.
    //
    //       The old code hacked around this issue by doing vector<string>().swap(paths_);in handleRequest()
    //       This caused its own set of problems. JIRA 434
    // See: Client/bin/gcc-4.8/release/perf_test_large_defs

    // record all the user edits to the node. Reuse the time stamp cache created in handleRequest()
    std::string ss("MSG:");
    ss += Log::instance()->get_cached_time_stamp();

    print(ss, path); // custom print
    defs->add_edit_history(path, ss);
}

void ClientToServerCmd::add_delete_edit_history(Defs* defs, const std::string& path) const {
    // History is added to Str::ROOT_PATH(), but the path must show deleted node path
    std::string ss("MSG:");
    ss += Log::instance()->get_cached_time_stamp();

    print(ss, path); // custom print
    defs->add_edit_history(Str::ROOT_PATH(), ss);
}

CEREAL_REGISTER_TYPE(ServerVersionCmd)
CEREAL_REGISTER_TYPE(CtsCmd)
CEREAL_REGISTER_TYPE(CSyncCmd)
CEREAL_REGISTER_TYPE(ClientHandleCmd)
CEREAL_REGISTER_TYPE(CtsNodeCmd)
CEREAL_REGISTER_TYPE(PathsCmd)
CEREAL_REGISTER_TYPE(DeleteCmd)
CEREAL_REGISTER_TYPE(CheckPtCmd)
CEREAL_REGISTER_TYPE(LoadDefsCmd)
CEREAL_REGISTER_TYPE(LogCmd)
CEREAL_REGISTER_TYPE(LogMessageCmd)
CEREAL_REGISTER_TYPE(BeginCmd)
CEREAL_REGISTER_TYPE(ZombieCmd)
CEREAL_REGISTER_TYPE(InitCmd)
CEREAL_REGISTER_TYPE(EventCmd)
CEREAL_REGISTER_TYPE(MeterCmd)
CEREAL_REGISTER_TYPE(LabelCmd)
CEREAL_REGISTER_TYPE(QueueCmd)
CEREAL_REGISTER_TYPE(AbortCmd)
CEREAL_REGISTER_TYPE(CtsWaitCmd)
CEREAL_REGISTER_TYPE(CompleteCmd)
CEREAL_REGISTER_TYPE(RequeueNodeCmd)
CEREAL_REGISTER_TYPE(OrderNodeCmd)
CEREAL_REGISTER_TYPE(RunNodeCmd)
CEREAL_REGISTER_TYPE(ReplaceNodeCmd)
CEREAL_REGISTER_TYPE(ForceCmd)
CEREAL_REGISTER_TYPE(FreeDepCmd)
CEREAL_REGISTER_TYPE(CFileCmd)
CEREAL_REGISTER_TYPE(EditScriptCmd)
CEREAL_REGISTER_TYPE(PlugCmd)
CEREAL_REGISTER_TYPE(AlterCmd)
CEREAL_REGISTER_TYPE(MoveCmd)
CEREAL_REGISTER_TYPE(GroupCTSCmd)
CEREAL_REGISTER_TYPE(ShowCmd)
CEREAL_REGISTER_TYPE(QueryCmd)
