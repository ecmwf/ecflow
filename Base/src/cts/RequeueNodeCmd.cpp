/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
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
#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "CtsApi.hpp"
#include "Defs.hpp"
#include "Task.hpp"
#include "SuiteChanged.hpp"
#include "Log.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;


bool RequeueNodeCmd::equals(ClientToServerCmd* rhs) const
{
	auto* the_rhs = dynamic_cast< RequeueNodeCmd* > ( rhs );
	if ( !the_rhs ) return false;
	if (paths_ != the_rhs->paths()) return false;
	if (option_ != the_rhs->option()) return false;
	return UserCmd::equals(rhs);
}

static std::string to_string(RequeueNodeCmd::Option option) {
   switch (option) {
      case RequeueNodeCmd::NO_OPTION: return string();
      case RequeueNodeCmd::FORCE: return "force";
      case RequeueNodeCmd::ABORT: return "abort";
      default: assert(false); break;
   }
   return string();
}

std::ostream& RequeueNodeCmd::print(std::ostream& os) const
{
 	return user_cmd(os,CtsApi::to_string(CtsApi::requeue(paths_,to_string(option_))));
}
std::ostream& RequeueNodeCmd::print_only(std::ostream& os) const
{
   os << CtsApi::to_string(CtsApi::requeue(paths_,to_string(option_)));return os;
}

std::ostream& RequeueNodeCmd::print(std::ostream& os, const std::string& path) const
{
   std::vector<std::string> paths(1,path);
   return user_cmd(os,CtsApi::to_string(CtsApi::requeue(paths,to_string(option_))));
}

STC_Cmd_ptr RequeueNodeCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().requeue_node_++;
	assert(isWrite()); // isWrite used in handleRequest() to control check pointing

	// The clear_suspended_in_child_nodes *only* incremented for child nodes.
	// Hence we only clear suspended for child nodes.
   Node::Requeue_args args(true /* reset repeats */,
                           true /* reset_day_date_reueue_count  */,
                           0    /* clear_suspended_in_child_nodes */,
                           true /* reset_next_time_slot */,
                           true /* reset relative duration */);

   std::stringstream ss;
	size_t vec_size = paths_.size();
	for(size_t i = 0; i < vec_size; i++) {

	   node_ptr theNodeToRequeue = find_node_for_edit_no_throw(as,paths_[i]);
      if (!theNodeToRequeue.get()) {
         ss << "RequeueNodeCmd: Could not find node at path " << paths_[i] << "\n";
         LOG(Log::ERR,"RequeueNodeCmd: Could not find node at path " << paths_[i]);
         continue;
      }

      if (!theNodeToRequeue->suite()->begun()) {
         std::stringstream ss;
         ss << "RequeueNodeCmd::doHandleRequest: For node " << paths_[i] << ". The suite " << theNodeToRequeue->suite()->name() << " must be 'begun' first\n";
         throw std::runtime_error( ss.str() ) ;
      }

	   SuiteChanged0 changed(theNodeToRequeue);

	   if (option_ == RequeueNodeCmd::ABORT) {
	      // ONLY Re-queue the aborted tasks
	      std::vector<Task*> taskVec;
	      theNodeToRequeue->getAllTasks(taskVec);
	      for(auto & i : taskVec) {
	         if (i->state() == NState::ABORTED) {
	            i->requeue(args);
	            i->set_most_significant_state_up_node_tree(); // Must in loop and not outside ECFLOW-428
	         }
	      }

         // Call handleStateChange on parent, to avoid requeue same node again.
         Node* parent = theNodeToRequeue->parent();
         if (parent) parent->handleStateChange();  // ECFLOW-359
	   }
	   else if ( option_ == RequeueNodeCmd::NO_OPTION) {
	      // ONLY Re-queue if there no tasks in submitted or active states
	      std::vector<Task*> taskVec;
	      theNodeToRequeue->getAllTasks(taskVec);
	      for(auto & i : taskVec) {
	         if (i->state() == NState::SUBMITTED || i->state() == NState::ACTIVE) {
	            return PreAllocatedReply::ok_cmd();
	         }
	      }

	      // The NO_REQUE_IF_SINGLE_TIME_DEP is typically cleared at the *end* requeue, however there are cases
	      // where we need to reset it *BEFORE* re-queue. Since it is tied to missing the next time slot
	      // i.e take this case:
	      //     time 10:00  15:00 00:30
	      // If we force complete, we set NO_REQUE_IF_SINGLE_TIME_DEP, which is used to advance next valid time slot
	      // (i.e no reset), however when we reach the *end* time i.e 15:00
	      // calling force complete now leaves node in a complete state and with NO_REQUE_IF_SINGLE_TIME_DEP set.
	      // Therefore *any* *MANUAL* re-queue afterward will NOT reset the next valid time slot.
	      // To overcome this manual re-queue will always clear NO_REQUE_IF_SINGLE_TIME_DEP and hence reset next valid time slot
	      theNodeToRequeue->requeue(args);
	      theNodeToRequeue->set_most_significant_state_up_node_tree();

         // Call handleStateChange on parent, to avoid requeue same node again.
         Node* parent = theNodeToRequeue->parent();
         if (parent) parent->handleStateChange();  // ECFLOW-359
	   }
	   else if ( option_ == RequeueNodeCmd::FORCE) {

	      as->zombie_ctrl().add_user_zombies(theNodeToRequeue,CtsApi::requeueArg());

	      // Please note: that if any tasks under theNodeToRequeue are in
	      // active or submitted states, then we will have created zombie jobs
	      // The GUI: that calls this command should call a separate request
	      // the returns the active/submitted tasks first. This can then be
	      // presented to the user, who can elect to kill them if required.
	      theNodeToRequeue->requeue(args);
	      theNodeToRequeue->set_most_significant_state_up_node_tree();

	      // Call handleStateChange on parent, to avoid requeue same node again.
         Node* parent = theNodeToRequeue->parent();
         if (parent) parent->handleStateChange();  // ECFLOW-359
	   }
	}

   std::string error_msg = ss.str();
   if (!error_msg.empty()) {
      throw std::runtime_error( error_msg ) ;
   }

   // Do an immediate job submission, so that any re-queued tasks are submitted
   return doJobSubmission(as);
}

bool RequeueNodeCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& cmd) const
{
   return do_authenticate(as,cmd,paths_);
}

const char* RequeueNodeCmd::arg()  { return CtsApi::requeueArg();}
const char* RequeueNodeCmd::desc() {
   return
            "Re queues the specified node(s)\n"
            "  If any child of the specified node(s) is in a suspended state, this state is cleared\n"
            "Repeats are reset to their starting values, relative time attributes are reset.\n"
            "  arg1 = (optional) [ abort | force ]\n"
            "         abort  = re-queue only aborted tasks below node\n"
            "         force  = Force the re-queueing even if there are nodes that are active or submitted\n"
            "         <null> = Checks if any tasks are in submitted or active states below the node\n"
            "                  if so does nothing. Otherwise re-queues the node.\n"
            "  arg2 = list of node paths. The node paths must begin with a leading '/' character\n\n"
            "Usage:\n"
            "  --requeue=abort /suite/f1  # re-queue all aborted tasks of /suite/f1\n"
            "  --requeue=force /suite/f1  # forcibly re-queue /suite/f1 and all its children.May cause zombies.\n"
            "  --requeue=/s1/f1/t1 /s1/t2 # Re-queue node '/suite/f1/t1' and '/s1/t2'"
            ;
}

void RequeueNodeCmd::addOption(boost::program_options::options_description& desc) const{
	desc.add_options()( RequeueNodeCmd::arg(), po::value< vector<string> >()->multitoken(), RequeueNodeCmd::desc() );
}
void RequeueNodeCmd::create( 	Cmd_ptr& cmd,
								boost::program_options::variables_map& vm,
								AbstractClientEnv* ac) const
{
	vector<string> args = vm[ RequeueNodeCmd::arg() ].as< vector<string> >();

	if (ac->debug()) dumpVecArgs(RequeueNodeCmd::arg(),args);

	if (args.size() < 1 ) {
		std::stringstream ss;
		ss << "RequeueNodeCmd: At least 1 argument(path to node) expected. Please specify one of:\n";
		ss << RequeueNodeCmd::arg() << " pathToNode\n";
		ss << RequeueNodeCmd::arg() << " abort pathToNode\n";
		ss << RequeueNodeCmd::arg() << " force pathToNode\n";
	 	throw std::runtime_error( ss.str() );
	}

   std::vector<std::string> options,paths;
   split_args_to_options_and_paths(args,options,paths); // relative order is still preserved
   if (paths.empty()) {
      std::stringstream ss;
      ss << "RequeueNodeCmd: No paths specified. At least one path expected. Paths must begin with a leading '/' character\n" << RequeueNodeCmd::desc() << "\n";
      throw std::runtime_error( ss.str() );
   }

   RequeueNodeCmd::Option option = RequeueNodeCmd::NO_OPTION;
   size_t vec_size = options.size();
   for(size_t i = 0; i < vec_size; i++) {
      if (options[i] == "abort") {
         option = RequeueNodeCmd::ABORT;
         if (ac->debug()) cout << "  ABORT selected\n";
      }
      else if (options[i] == "force") {
         option = RequeueNodeCmd::FORCE;
         if (ac->debug())  cout << "  FORCE selected\n";
      }
      else {
         std::stringstream ss;
         ss << "RequeueNodeCmd: RequeueNodeCmd: Expected : [force | abort ] paths.\n" << RequeueNodeCmd::desc() << "\n";
         throw std::runtime_error( ss.str() );
      }
   }
   if (options.size() > 1) {
      std::stringstream ss;
      ss << "RequeueNodeCmd: Expected only a single option i.e [ force | abort ]\n" << RequeueNodeCmd::desc() << "\n";
      throw std::runtime_error( ss.str() );
   }

 	cmd = std::make_shared<RequeueNodeCmd>(paths,option);
}

std::ostream& operator<<(std::ostream& os, const RequeueNodeCmd& c) { return c.print(os); }
