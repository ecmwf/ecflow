/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #67 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iostream>
#include <sstream>

#include "ClientToServerCmd.hpp"
#include "ServerToClientCmd.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Alias.hpp"

#include "JobsParam.hpp"
#include "Jobs.hpp"
#include "AbstractServer.hpp"
#include "Log.hpp"
#include "Str.hpp"
#include "CmdContext.hpp"
#include "EditHistoryMgr.hpp"
#include "Host.hpp"

using namespace std;
using namespace boost;
using namespace ecf;

//#define DEBUG_INVARIANTS 1

ClientToServerCmd::ClientToServerCmd() : cl_host_(Host().name()) {}

ClientToServerCmd::~ClientToServerCmd()= default;

bool ClientToServerCmd::equals(ClientToServerCmd* rhs) const
{
	return hostname() == rhs->hostname();
}

STC_Cmd_ptr ClientToServerCmd::handleRequest(AbstractServer* as) const
{
   // Allow creating of new time stamp, when *not* in a command. i.e during node tree traversal in server
   CmdContext cmdContext;

   // Automatically flush log file at the end of the command
   LogFlusher logFlusher;

   // Create the log time stamp once for a given request
   if (Log::instance()) Log::instance()->cache_time_stamp();

   // LOG the command, *BEFORE* invoking it. (i.e in case server hangs/crashes)
   // Allow override in the rare cases, where we want to output additional debug
   // If logging fails set late flag to warn users, ECFLOW-536
   do_log(as);

#ifdef DEBUG_INVARIANTS
   LOG_ASSERT( as->defs() , "ClientToServerCmd::handleRequest: Start:  No defs? ");
   std::string errmsg;
   if (!as->defs()->checkInvariants(errmsg)) {
      LOG(Log::ERR,"ClientToServerCmd::handleRequest: PreCondition: Failed invariant checking:" << errmsg);
   }
#endif


   STC_Cmd_ptr halted;
   if (! authenticate(as,halted)) {
      assert (halted.get());
      return halted;
   }

   // mark edited nodes, with edit history. relies on doHandleRequest to populate edit_history_nodes_/paths
   // hence must at the same scope level
   EditHistoryMgr edit_history_mgr(this,as);

   // Handle the request, and return the reply back to the client
   STC_Cmd_ptr server_to_client_ptr = doHandleRequest(as);
   if ( isWrite() && server_to_client_ptr->ok()) {

      // This can end up, check pointing the defs file
      as->nodeTreeStateChanged();
   }

#ifdef DEBUG_INVARIANTS
   LOG_ASSERT( as->defs() , "ClientToServerCmd::handleRequest: End:  No defs? ");
   std::string errmsg;
   if (!as->defs()->checkInvariants(errmsg)) {
      LOG(Log::ERR,"ClientToServerCmd::handleRequest: PostCondition: Failed invariant checking:" << errmsg);
   }
#endif

   return server_to_client_ptr;
}

void ClientToServerCmd::do_log(AbstractServer* as) const
{
   std::stringstream ss;
   print(ss);                        // Populate the stream with command details:
   if (!log(Log::MSG,ss.str())) {    // will automatically add end of line
      // problems writing to log file, warn users, ECFLOW-536
      as->defs()->flag().set(ecf::Flag::LATE);
   }
}

STC_Cmd_ptr ClientToServerCmd::doJobSubmission(AbstractServer* as)
{
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

node_ptr ClientToServerCmd::find_node(AbstractServer* as, const std::string& absNodepath) const
{
   node_ptr theNode =  as->defs()->findAbsNode(absNodepath);
   if ( !theNode.get() ) {

      std::string errorMsg = "Can not find node at path '";
      errorMsg += absNodepath;
      errorMsg += "' ";
      throw std::runtime_error( errorMsg) ;
   }
   return theNode;
}


void ClientToServerCmd::dumpVecArgs(const char* argOption, const std::vector<std::string>& args) {
   cout << "  " << argOption;
   for(size_t i= 0; i < args.size(); i++)  { cout << " args[" << i << "]='" << args[i] << "'";} cout << "\n";
}

node_ptr ClientToServerCmd::find_node_for_edit(AbstractServer* as, const std::string& absNodepath) const
{
   node_ptr theNode = find_node(as, absNodepath);
   add_node_for_edit_history(theNode);
   return theNode;
}

node_ptr ClientToServerCmd::find_node_for_edit_no_throw(AbstractServer* as, const std::string& absNodepath) const
{
   node_ptr theNode = as->defs()->findAbsNode(absNodepath);
   add_node_for_edit_history(theNode);
   return theNode;
}

void ClientToServerCmd::add_node_for_edit_history(AbstractServer* as,const std::string& absNodepath) const
{
    add_node_for_edit_history(as->defs()->findAbsNode(absNodepath));
}

void ClientToServerCmd::add_node_for_edit_history(node_ptr the_node) const
{
   if (the_node.get()) edit_history_nodes_.push_back(the_node);
}

void ClientToServerCmd::add_node_path_for_edit_history(const std::string& absNodepath) const
{
   edit_history_node_paths_.push_back(absNodepath);
}

CEREAL_REGISTER_TYPE(ServerVersionCmd);
CEREAL_REGISTER_TYPE(CtsCmd);
CEREAL_REGISTER_TYPE(CSyncCmd);
CEREAL_REGISTER_TYPE(ClientHandleCmd);
CEREAL_REGISTER_TYPE(CtsNodeCmd);
CEREAL_REGISTER_TYPE(PathsCmd);
CEREAL_REGISTER_TYPE(DeleteCmd);
CEREAL_REGISTER_TYPE(CheckPtCmd);
CEREAL_REGISTER_TYPE(LoadDefsCmd);
CEREAL_REGISTER_TYPE(LogCmd);
CEREAL_REGISTER_TYPE(LogMessageCmd);
CEREAL_REGISTER_TYPE(BeginCmd);
CEREAL_REGISTER_TYPE(ZombieCmd);
CEREAL_REGISTER_TYPE(InitCmd);
CEREAL_REGISTER_TYPE(EventCmd);
CEREAL_REGISTER_TYPE(MeterCmd);
CEREAL_REGISTER_TYPE(LabelCmd);
CEREAL_REGISTER_TYPE(QueueCmd);
CEREAL_REGISTER_TYPE(AbortCmd);
CEREAL_REGISTER_TYPE(CtsWaitCmd);
CEREAL_REGISTER_TYPE(CompleteCmd);
CEREAL_REGISTER_TYPE(RequeueNodeCmd);
CEREAL_REGISTER_TYPE(OrderNodeCmd);
CEREAL_REGISTER_TYPE(RunNodeCmd);
CEREAL_REGISTER_TYPE(ReplaceNodeCmd);
CEREAL_REGISTER_TYPE(ForceCmd);
CEREAL_REGISTER_TYPE(FreeDepCmd);
CEREAL_REGISTER_TYPE(CFileCmd);
CEREAL_REGISTER_TYPE(EditScriptCmd);
CEREAL_REGISTER_TYPE(PlugCmd);
CEREAL_REGISTER_TYPE(AlterCmd);
CEREAL_REGISTER_TYPE(MoveCmd);
CEREAL_REGISTER_TYPE(GroupCTSCmd);
CEREAL_REGISTER_TYPE(ShowCmd);
CEREAL_REGISTER_TYPE(QueryCmd);
