/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #16 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "CtsApi.hpp"
#include "Defs.hpp"
#include "Task.hpp"
#include "Suite.hpp"
#include "Log.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

DeleteCmd::DeleteCmd(const std::string& absNodePath, bool force) : force_(force), group_cmd_(nullptr)
{
   if (!absNodePath.empty()) paths_.push_back(absNodePath);
}

std::ostream& DeleteCmd::print(std::ostream& os) const
{
   return user_cmd(os,CtsApi::to_string(CtsApi::delete_node(paths_,force_)));
}

std::ostream& DeleteCmd::print_only(std::ostream& os) const
{
   os << CtsApi::to_string(CtsApi::delete_node(paths_,force_));
   return os;
}

std::ostream& DeleteCmd::print(std::ostream& os, const std::string& path) const
{
   std::vector<std::string> paths(1,path);
   return user_cmd(os,CtsApi::to_string(CtsApi::delete_node(paths,force_)));
}

bool DeleteCmd::equals(ClientToServerCmd* rhs) const
{
   auto* the_rhs = dynamic_cast< DeleteCmd* > ( rhs );
   if ( !the_rhs ) return false;
   if (paths_ != the_rhs->paths()) return false;
   if (force_ != the_rhs->force()) return false;
   return UserCmd::equals(rhs);
}

const char* DeleteCmd::theArg() const
{
   return CtsApi::delete_node_arg();
}


STC_Cmd_ptr DeleteCmd::doHandleRequest(AbstractServer* as) const
{
   as->update_stats().node_delete_++;
   std::stringstream ss;

   if ( paths_.empty() ) {
      if (!force_) check_for_active_or_submitted_tasks(as,node_ptr());
      else         as->zombie_ctrl().add_user_zombies(as->defs(),CtsApi::delete_node_arg());
      as->clear_defs();

      // If this command is part of a group command, let the following sync command, know about the new handle
      if (group_cmd_) group_cmd_->set_client_handle(0);

      // This will reset client defs and set client handle to zero on the client side.
      return PreAllocatedReply::delete_all_cmd();
   }
   else {

      size_t vec_size = paths_.size();
      for(size_t i = 0; i < vec_size; i++) {

         node_ptr theNodeToDelete =  as->defs()->findAbsNode(paths_[i]);
         if (!theNodeToDelete.get()) {
            ss << "DeleteCmd:Delete: Could not find node at path '" << paths_[i] << "'\n";
            LOG(Log::ERR,"Delete: Could not find node at path " << paths_[i]);
            continue;
         }
         // since node is to be deleted, we need to record the paths.
         add_node_path_for_edit_history(paths_[i]);

         if (!force_) check_for_active_or_submitted_tasks(as,theNodeToDelete);
         else         as->zombie_ctrl().add_user_zombies(theNodeToDelete,CtsApi::delete_node_arg());

         if (!as->defs()->deleteChild( theNodeToDelete.get() )) {
            std::string errorMsg = "Delete: Can not delete node " + theNodeToDelete->debugNodePath();
            throw std::runtime_error( errorMsg ) ;
         }
      }
   }

   std::string error_msg = ss.str();
   if (!error_msg.empty()) {
      throw std::runtime_error( error_msg ) ;
   }

   return PreAllocatedReply::ok_cmd();
}

bool DeleteCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& cmd) const
{
   return do_authenticate(as,cmd,paths_);
}

void DeleteCmd::check_for_active_or_submitted_tasks(AbstractServer* as,node_ptr theNodeToDelete)
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

void DeleteCmd::addOption(boost::program_options::options_description& desc) const
{
    desc.add_options()( CtsApi::delete_node_arg(), po::value< vector<string> >()->multitoken(), delete_node_desc() );
}

void DeleteCmd::create(   Cmd_ptr& cmd,
         boost::program_options::variables_map& vm,
         AbstractClientEnv* ac ) const
{
   vector<string> args = vm[  theArg() ].as< vector<string> >();
   if (ac->debug()) dumpVecArgs( theArg(), args);

   std::vector<std::string> options,paths;
   split_args_to_options_and_paths(args,options,paths); // relative order is still preserved

   bool force = false;
   bool all = false;
   bool do_prompt = true;
   size_t vec_size = options.size();
   for(size_t i = 0; i < vec_size; i++) {
      if (args[i] == "_all_") all = true;
      if (args[i] == "force") force = true;
      if (args[i] == "yes")   do_prompt = false;
   }

   if (!all && paths.empty()) {
      std::stringstream ss; ss << "Delete: No paths specified. Paths must begin with a leading '/' character\n";
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

   cmd = Cmd_ptr(new DeleteCmd(paths, force));
}

std::ostream& operator<<(std::ostream& os, const DeleteCmd& c) { return c.print(os); }
