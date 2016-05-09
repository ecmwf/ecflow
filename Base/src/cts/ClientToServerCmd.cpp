/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #67 $ 
//
// Copyright 2009-2016 ECMWF. 
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

using namespace std;
using namespace boost;
using namespace ecf;

//#define DEBUG_INVARIANTS 1

ClientToServerCmd::~ClientToServerCmd(){}

STC_Cmd_ptr ClientToServerCmd::handleRequest(AbstractServer* as) const
{
   // Allow creating of new time stamp, when *not* in a command. i.e during node tree traversal in server
   CmdContext cmdContext;

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
      if (as->defs()) {
         as->defs()->flag().set(ecf::Flag::LATE);
      }
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

      std::stringstream ss;
      print(ss);

      std::string errorMsg = "Can not find node at path '";
      errorMsg += absNodepath;
      errorMsg += "' ";
      errorMsg += ss.str();
      errorMsg += " failed";
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

BOOST_CLASS_EXPORT_IMPLEMENT(ServerVersionCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(CtsCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(CSyncCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(ClientHandleCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(CtsNodeCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(PathsCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(CheckPtCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(LoadDefsCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(LogCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(LogMessageCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(BeginCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(ZombieCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(InitCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(EventCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(MeterCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(LabelCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(AbortCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(CtsWaitCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(CompleteCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(RequeueNodeCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(OrderNodeCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(RunNodeCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(ReplaceNodeCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(ForceCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(FreeDepCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(CFileCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(EditScriptCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(PlugCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(AlterCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(MoveCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(GroupCTSCmd)
BOOST_CLASS_EXPORT_IMPLEMENT(ShowCmd)
