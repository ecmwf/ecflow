/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #16 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <algorithm>
#include <boost/make_shared.hpp>

#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "CtsApi.hpp"
#include "Defs.hpp"
#include "Task.hpp"
#include "Suite.hpp"
#include "SuiteChanged.hpp"
#include "Ecf.hpp"
#include "Log.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

// forward declare static functions
static void check_for_active_or_submitted_tasks(AbstractServer* as,node_ptr theNodeToDelete);


PathsCmd::PathsCmd(Api api,const std::string& absNodePath, bool force)
: api_(api),force_(force)
{
   if (!absNodePath.empty()) paths_.push_back(absNodePath);
}

std::ostream& PathsCmd::print(std::ostream& os) const
{
   return my_print(os,paths_);
}

std::ostream& PathsCmd::print(std::ostream& os, const std::string& path) const
{
   std::vector<std::string> paths(1,path);
   return my_print(os,paths);
}

std::ostream& PathsCmd::my_print(std::ostream& os,const std::vector<std::string>& paths) const
{
   switch (api_) {
      case PathsCmd::DELETE:             return user_cmd(os,CtsApi::to_string(CtsApi::delete_node(paths,force_))); break;
      case PathsCmd::SUSPEND:            return user_cmd(os,CtsApi::to_string(CtsApi::suspend(paths))); break;
      case PathsCmd::RESUME:             return user_cmd(os,CtsApi::to_string(CtsApi::resume(paths))); break;
      case PathsCmd::KILL:               return user_cmd(os,CtsApi::to_string(CtsApi::kill(paths))); break;
      case PathsCmd::STATUS:             return user_cmd(os,CtsApi::to_string(CtsApi::status(paths))); break;
      case PathsCmd::CHECK:              return user_cmd(os,CtsApi::to_string(CtsApi::check(paths))); break;
      case PathsCmd::EDIT_HISTORY:       return user_cmd(os,CtsApi::to_string(CtsApi::edit_history(paths))); break;
      case PathsCmd::NO_CMD:       break;
      default: assert(false);break;
   }
   return os;
}

bool PathsCmd::equals(ClientToServerCmd* rhs) const
{
   PathsCmd* the_rhs = dynamic_cast< PathsCmd* > ( rhs );
   if ( !the_rhs ) return false;
   if (api_ != the_rhs->api()) return false;
   if (paths_ != the_rhs->paths()) return false;
   if (force_ != the_rhs->force()) return false;
   return UserCmd::equals(rhs);
}

bool PathsCmd::isWrite() const
{
   switch (api_) {
      case PathsCmd::DELETE:            return true;  break; // requires write privilege
      case PathsCmd::SUSPEND:           return true;  break; // requires write privilege
      case PathsCmd::RESUME:            return true;  break; // requires write privilege
      case PathsCmd::KILL:              return true;  break; // requires write privilege
      case PathsCmd::STATUS:            return false; break; // read only
      case PathsCmd::CHECK:             return false; break; // read only
      case PathsCmd::EDIT_HISTORY:      return false; break; // read only
      case PathsCmd::NO_CMD: break;
      default: break;
   }
   assert(false);
   return false;
}

const char* PathsCmd::theArg() const
{
   switch (api_) {
      case PathsCmd::DELETE:             return CtsApi::delete_node_arg(); break;
      case PathsCmd::SUSPEND:            return CtsApi::suspend_arg(); break;
      case PathsCmd::RESUME:             return CtsApi::resume_arg(); break;
      case PathsCmd::KILL:               return CtsApi::kill_arg(); break;
      case PathsCmd::STATUS:             return CtsApi::statusArg(); break;
      case PathsCmd::CHECK:              return CtsApi::check_arg(); break;
      case PathsCmd::EDIT_HISTORY:       return CtsApi::edit_history_arg(); break;
      case PathsCmd::NO_CMD: break;
      default: break;
   }
   assert(false);
   return NULL;
}

bool PathsCmd::delete_all_cmd() const
{
   if (api_ == PathsCmd::DELETE && paths_.empty()) {
      return true;
   }
   return false ;
}

STC_Cmd_ptr PathsCmd::doHandleRequest(AbstractServer* as) const
{
   std::stringstream ss;
   switch (api_) {

      case PathsCmd::CHECK:  {
         as->update_stats().check_++;

         if (  paths_.empty() ) {
            // check all the defs,
            std::string error_msg,warning_msg;
            if (!as->defs()->check(error_msg,warning_msg)) {
               error_msg += "\n";
               error_msg += warning_msg;
               return PreAllocatedReply::string_cmd(error_msg);
            }
            return PreAllocatedReply::string_cmd(warning_msg); // can be empty
         }
         else {
            std::string acc_warning_msg;
            size_t vec_size = paths_.size();
            for(size_t i = 0; i < vec_size; i++) {

               node_ptr theNodeToCheck =  as->defs()->findAbsNode(paths_[i]);
               if (!theNodeToCheck.get()) {
                  ss << "PathsCmd:Check: Could not find node at path '" << paths_[i] << "'\n";
                  LOG(Log::ERR,"Check: Could not find node at path " << paths_[i]);
                  continue;
               }

               std::string error_msg,warning_msg;
               if (!theNodeToCheck->check(error_msg,warning_msg)) {
                  error_msg += "\n";
                  error_msg += warning_msg;
                  return PreAllocatedReply::string_cmd(error_msg);
               }
               acc_warning_msg += warning_msg;
            }
            std::string paths_not_fnd_error_msg = ss.str();
            if (!paths_not_fnd_error_msg.empty()) throw std::runtime_error( paths_not_fnd_error_msg );
            return PreAllocatedReply::string_cmd(acc_warning_msg);
         }
         break;
      }

      case PathsCmd::DELETE: {
         as->update_stats().node_delete_++;

         if ( paths_.empty() ) {
            if (!force_) check_for_active_or_submitted_tasks(as,node_ptr());
            else         as->zombie_ctrl().add_user_zombies(as->defs());
            as->clear_defs();
         }
         else {

            size_t vec_size = paths_.size();
            for(size_t i = 0; i < vec_size; i++) {

               node_ptr theNodeToDelete =  as->defs()->findAbsNode(paths_[i]);
               if (!theNodeToDelete.get()) {
                  ss << "PathsCmd:Delete: Could not find node at path '" << paths_[i] << "'\n";
                  LOG(Log::ERR,"Delete: Could not find node at path " << paths_[i]);
                  continue;
               }
               // since node is to be deleted, we need to record the paths.
               add_node_path_for_edit_history(paths_[i]);

               if (!force_) check_for_active_or_submitted_tasks(as,theNodeToDelete);
               else         as->zombie_ctrl().add_user_zombies(theNodeToDelete);

               if (!as->defs()->deleteChild( theNodeToDelete.get() )) {
                  std::string errorMsg = "Delete: Can not delete node " + theNodeToDelete->debugNodePath();
                  throw std::runtime_error( errorMsg ) ;
               }
            }
         }
         break;
      }

      case PathsCmd::SUSPEND: {
         as->update_stats().node_suspend_++;
         size_t vec_size = paths_.size();
         for(size_t i = 0; i < vec_size; i++) {
            node_ptr theNode = find_node_for_edit_no_throw(as,paths_[i]);
            if (!theNode.get()) {
               ss << "PathsCmd:Suspend: Could not find node at path '" << paths_[i] << "'\n";
               LOG(Log::ERR,"Suspend: Could not find node at path " << paths_[i]);
               continue;
            }
            SuiteChanged0 changed(theNode);
            theNode->suspend();
         }
         break;
      }

      case PathsCmd::RESUME: {

         // At the end of resume, we need to traverse node tree, and do job submission
         as->update_stats().node_resume_++;
         size_t vec_size = paths_.size();
         for(size_t i = 0; i < vec_size; i++) {
            node_ptr theNode = find_node_for_edit_no_throw(as,paths_[i]);
            if (!theNode.get()) {
               ss << "PathsCmd:Resume: Could not find node at path '" << paths_[i] << "'\n";
               LOG(Log::ERR,"Resume: Could not find path " << paths_[i]);
               continue;
            }
            SuiteChanged0 changed(theNode);
            theNode->resume();
            as->increment_job_generation_count(); // in case we throw below
         }
         break;
      }

      case PathsCmd::KILL: {
         as->update_stats().node_kill_++;
         size_t vec_size = paths_.size();
         for(size_t i = 0; i < vec_size; i++) {
            node_ptr theNode = find_node_for_edit_no_throw(as,paths_[i]);
            if (!theNode.get()) {
               ss << "PathsCmd:Kill: Could not find node at path '" << paths_[i] << "'\n";
               LOG(Log::ERR,"Kill: Could not find node at path " << paths_[i]);
               continue;
            }
            SuiteChanged0 changed(theNode);
            theNode->kill();  // this can throw std::runtime_error
         }
         break;
      }

      case PathsCmd::STATUS: {
         as->update_stats().node_status_++;
         size_t vec_size = paths_.size();
         for(size_t i = 0; i < vec_size; i++) {
            node_ptr theNode = find_node_for_edit_no_throw(as,paths_[i]);
            if (!theNode.get()) {
               ss << "PathsCmd:Status: Could not find node at path '" << paths_[i] << "'\n";
               LOG(Log::ERR,"Status: Could not find node at path " << paths_[i]);
               continue;
            }
            if (!theNode->suite()->begun()) {
               std::stringstream ss;
               ss << "Status failed. For " << paths_[i] << " The suite " << theNode->suite()->name() << " must be 'begun' first\n";
               throw std::runtime_error( ss.str() ) ;
            }
            SuiteChanged0 changed(theNode);
            theNode->status();   // this can throw std::runtime_error
         }
         break;
      }

      case PathsCmd::EDIT_HISTORY: {
         as->update_stats().node_edit_history_++;
         if (paths_.empty()) throw std::runtime_error( "No paths specified for edit history") ;
         // Only first path used
         const std::deque<std::string>& edit_history = as->defs()->get_edit_history(paths_[0]);
         std::vector<std::string> vec; vec.reserve(edit_history.size());
         std::copy(edit_history.begin(),edit_history.end(),std::back_inserter(vec));
         return PreAllocatedReply::string_vec_cmd(vec);
      }

      case PathsCmd::NO_CMD: assert(false); break;

      default: assert(false); break;
   }

   std::string error_msg = ss.str();
   if (!error_msg.empty()) {
      throw std::runtime_error( error_msg ) ;
   }

   if ( PathsCmd::RESUME  == api_) {
      // After resume we need to do job submission.
      return doJobSubmission(as);
   }

   return PreAllocatedReply::ok_cmd();
}

static void check_for_active_or_submitted_tasks(AbstractServer* as,node_ptr theNodeToDelete)
{
   vector<Task*> taskVec;
   if ( theNodeToDelete.get() ) {
      theNodeToDelete->getAllTasks(taskVec);
   }
   else {
      as->defs()->getAllTasks(taskVec);
   }

   vector<Task*> activeVec,submittedVec;
   BOOST_FOREACH(Task* t, taskVec) {
      if (t->state() == NState::ACTIVE)  activeVec.push_back(t);
      if (t->state() == NState::SUBMITTED)  submittedVec.push_back(t);
   }
   if (!activeVec.empty() || !submittedVec.empty()) {
      std::stringstream ss;
      if (theNodeToDelete.get()) ss << "Can not delete node " << theNodeToDelete->debugNodePath() << "\n";
      else                       ss << "Can not delete all nodes.\n";
      if (!activeVec.empty() ) {
         ss << " There are " << activeVec.size() << " active tasks. First : " << activeVec.front()->absNodePath() << "\n";
      }
      if (!submittedVec.empty() ) {
         ss << " There are " << submittedVec.size() << " submitted tasks. First : " << submittedVec.front()->absNodePath() << "\n";
      }
      ss << "Please use the 'force' option to bypass this check, at the expense of creating zombies\n";
      throw std::runtime_error( ss.str() ) ;
   }
}

static const char* delete_node_desc() {
   return
            "Deletes the specified node(s) or _ALL_ existing definitions( i.e delete all suites) in the server.\n"
            "  arg1 = [ force | yes ](optional)  # Use this parameter to bypass checks, i.e. for active or submitted tasks\n"
            "  arg2 = yes(optional)              # Use 'yes' to bypass the confirmation prompt\n"
            "  arg3 = node paths | _all_         # _all_ means delete all suites\n"
            "                                    # node paths must start with a leading '/'\n"
            "Usage:\n"
            "  --delete=_all_                    # Delete all suites in server. Use with care.\n"
            "  --delete=/suite/f1/t1             # Delete node at /suite/f1/t1. This will prompt\n"
            "  --delete=force /suite/f1/t1       # Delete node at /suite/f1/t1 even if active or submitted\n"
            "  --delete=force yes /s1 /s2        # Delete suites s1,s2 even if active or submitted, bypassing prompt"
            ;
}

static const char* get_check_desc() {
   return
            "Checks the expression and limits in the server. Will also check trigger references.\n"
            "Trigger expressions that reference paths that don't exist, will be reported as errors.\n"
            "(Note: On the client side unresolved paths in trigger expressions must\n"
            "have an associated 'extern' specified)\n"
            "  arg = [ _all_ | list of node paths ]\n"
            "Usage:\n"
            "  --check=_all_           # Checks the whole suite\n"
            "  --check /s1 /s2/f1/t1   # Check suite /s1 and task t1"
            ;
}

static const char* get_kill_desc() {
   return
            "Kills the job associated with the node.\n"
            "If a family or suite is selected, will kill hierarchically.\n"
            "Kill uses the ECF_KILL_CMD variable. After variable substitution it is invoked as a command.\n"
            "The command should be written in such a way that the output is written to %ECF_JOB%.kill\n"
            "as this allow the --file command to report the output: .e.e.\n"
            " /home/ma/emos/bin/ecfkill %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.kill 2>&1::\n"
            "Usage::\n"
            "   --kill /s1/f1/t1 /s1/f2/t2 # kill the jobs for tasks t1 and t2\n"
            "   --file /s1/f1/t1 kill      # write to standard out the '.kill' file for task /s1/f1/t1"
            ;
}
const char*  get_status_desc(){
   return
            "Shows the status of a job associated with a task.\n"
            "If a family or suite is selected, will invoke status command hierarchically.\n"
            "Status uses the ECF_STATUS_CMD variable. After variable substitution it is invoked as a command.\n"
            "The command should be written in such a way that the output is written to %ECF_JOB%.stat\n"
            "This will allow the output of status command to be shown by the --file command\n"
            "i.e /home/ma/emos/bin/ecfstatus  %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.stat 2>&1::\n"
            "Usage::\n"
            "   --status /s1/f1/t1 /s1/f2/t2\n"
            "   --file /s1/f1/t1 stat  # write to standard out the '.stat' file"
            ;
}
const char* get_edit_history_desc(){
   return
            "Returns the edit history associated with a Node.\n"
             "Usage::\n"
            "   --edit_history /s1/f1/t1\n"
            ;
}
const char* suspend_desc(){
   return
            "Suspend the given node. This prevents job generation for the given node, or any child node.\n"
            "Usage::\n"
            "   --suspend /s1/f1/t1   # suspend task s1/f1/t1\n"
            "   --suspend /s1 /s2     # suspend suites /s1 and /s2\n"
            ;
}
const char* resume_desc(){
   return
            "Resume the given node. This allows job generation for the given node, or any child node.\n"
            "Usage::\n"
            "   --resume /s1/f1/t1   # resume task s1/f1/t1\n"
            "   --resume /s1 /s2     # resume suites /s1 and /s2\n"
            ;
}

void PathsCmd::addOption(boost::program_options::options_description& desc) const
{
   switch (api_) {
      case PathsCmd::CHECK:{
         desc.add_options()(CtsApi::check_arg(),po::value< vector<string> >()->multitoken(),get_check_desc());
         break;
      }
      case PathsCmd::DELETE:{
         desc.add_options()( CtsApi::delete_node_arg(), po::value< vector<string> >()->multitoken(), delete_node_desc() );
         break;
      }
      case PathsCmd::SUSPEND:{
         desc.add_options()( CtsApi::suspend_arg(), po::value< vector<string> >()->multitoken(),suspend_desc());
         break;
      }
      case PathsCmd::RESUME: {
         desc.add_options()( CtsApi::resume_arg(), po::value< vector<string> >()->multitoken(),resume_desc());
         break;
      }
      case PathsCmd::KILL: {
         desc.add_options()( CtsApi::kill_arg(), po::value< vector<string> >()->multitoken(),get_kill_desc());
         break;
      }
      case PathsCmd::STATUS: {
         desc.add_options()( CtsApi::statusArg(), po::value< vector<string> >()->multitoken(), get_status_desc());
         break;
      }
      case PathsCmd::EDIT_HISTORY: {
         desc.add_options()( CtsApi::edit_history_arg(), po::value< vector<string> >()->multitoken(), get_edit_history_desc());
         break;
      }
      case PathsCmd::NO_CMD:  assert(false); break;
      default: assert(false); break;
   }
}

void PathsCmd::create(   Cmd_ptr& cmd,
         boost::program_options::variables_map& vm,
         AbstractClientEnv* ac ) const
{
   assert( api_ != PathsCmd::NO_CMD);

   vector<string> args = vm[  theArg() ].as< vector<string> >();
   if (ac->debug()) dumpVecArgs( theArg(), args);

   std::vector<std::string> options,paths;
   split_args_to_options_and_paths(args,options,paths); // relative order is still preserved

   bool force = false;
   if (api_ == PathsCmd::DELETE) {
      bool all = false;
      bool do_prompt = true;
      size_t vec_size = options.size();
      for(size_t i = 0; i < vec_size; i++) {
         if (args[i] == "_all_") all = true;
         if (args[i] == "force") force = true;
         if (args[i] == "yes")   do_prompt = false;
      }

      if (!all && paths.empty()) {
         std::stringstream ss;
         ss << "Delete: No paths specified. Paths must begin with a leading '/' character\n";
         throw std::runtime_error( ss.str() );
      }

      if (do_prompt) {
         std::string confirm;
         if (paths.empty()) confirm = "Are you sure you want to delete all the suites ? ";
         else {
            confirm = "Are you sure want to delete nodes at paths:\n";
            size_t vec_size = paths.size();
            for(size_t i = 0; i < vec_size; i++) {
               confirm += "  " + paths[i];
               if ( i == vec_size -1) confirm += " ? ";
               else                   confirm += "\n";
            }
         }
         prompt_for_confirmation(confirm);
      }
   }
   else if (api_ == PathsCmd::CHECK) {

      bool all = false;
      size_t vec_size = options.size();
      for(size_t i = 0; i < vec_size; i++) {
         if (args[i] == "_all_") all = true;
      }
      if (!all && paths.empty()) {
         std::stringstream ss;
         ss << "Check: Please specify '_all_' or a list of paths. Paths must begin with a leading '/' character\n";
         throw std::runtime_error( ss.str() );
      }
   }
   else {
      if (paths.empty()) {
         std::stringstream ss;
         ss << theArg() << ":  No paths specified. Paths must begin with a leading '/' character\n";
         throw std::runtime_error( ss.str() );
      }
   }

   cmd = Cmd_ptr(new PathsCmd( api_ , paths, force));
}

std::ostream& operator<<(std::ostream& os, const PathsCmd& c) { return c.print(os); }
