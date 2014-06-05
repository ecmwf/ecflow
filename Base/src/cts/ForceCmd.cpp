/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #36 $ 
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
#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "CtsApi.hpp"
#include "Defs.hpp"
#include "Node.hpp"
#include "Str.hpp"
#include "SuiteChanged.hpp"
#include "Extract.hpp"
#include "Log.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

// ===================================================================================

bool ForceCmd::equals(ClientToServerCmd* rhs) const
{
	ForceCmd* the_rhs = dynamic_cast<ForceCmd*>(rhs);
	if (!the_rhs)  return false;
	if ( paths_                != the_rhs->paths()) { return false;}
	if ( stateOrEvent_         != the_rhs->stateOrEvent()) { return false; }
	if ( recursive_            != the_rhs->recursive()) { return false; }
	if ( setRepeatToLastValue_ != the_rhs->setRepeatToLastValue()) { return false; }
	return UserCmd::equals(rhs);
}

std::ostream& ForceCmd::print(std::ostream& os) const
{
   return user_cmd(os,CtsApi::to_string(CtsApi::force(paths_,stateOrEvent_,recursive_,setRepeatToLastValue_)));
}

STC_Cmd_ptr ForceCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().force_++;

	assert(isWrite()); // isWrite used in handleRequest() to control check pointing

   bool is_event_state = Event::isValidState(stateOrEvent_);
   bool is_node_state = NState::isValid(stateOrEvent_);
 	if (!is_node_state && !is_event_state) {
		std::stringstream ss;
		ss << "ForceCmd: failed. Invalid node state or event " << stateOrEvent_ << " expected one of "
		   << "[ unknown | complete | queued | submitted | active | aborted | clear | set]";
 		throw std::runtime_error( ss.str() ) ;
 	}

   std::stringstream ss;
 	size_t vec_size = paths_.size();
 	for(size_t i = 0; i < vec_size; i++) {

 	   string the_path = paths_[i];
 	   string the_event;
 	   if ( is_event_state ) {
 	      Extract::pathAndName(paths_[i],the_path, the_event);
 	      if ( the_path.empty() || the_event.empty() ) {
 	         std::stringstream ss;
 	         ss << "ForceCmd: When 'set' or 'clear' is specified the path needs to include name of the event i.e\n";
 	         ss << " --force=/path/to_task:event_name set\n";
 	         ecf::log(Log::ERR,ss.str());
 	         continue;
 	      }
 	   }

 	   node_ptr node = find_node_for_edit_no_throw(as,the_path);
 	   if (!node.get()) {
         ss << "ForceCmd: Could not find node at path " << the_path << "\n";
 	      LOG(Log::ERR,"ForceCmd: Could not find node at path " << the_path);
 	      continue;
 	   }
 	   SuiteChanged0 changed(node); // Cater for suites in handles

 	   if (is_node_state) {
 	      /// We want this to have side effects. i.e bubble up state and re-queue if complete and has repeat's
 	      /// **** However if state is SET to complete, we want to MISS the next time slot.
 	      /// **** we need to mark the time dependency as *expired*, otherwise, it will be automatically reset to QUEUED state
 	      /// **** Additionally whenever we have set state to complete, need to miss the next time slot.
 	      NState::State new_state = NState::toState(stateOrEvent_);
 	      if (new_state == NState::COMPLETE)  {
 	         node->miss_next_time_slot();
 	      }

 	      if (recursive_) node->set_state_hierarchically( new_state, true /* force */ );
 	      else            node->set_state( new_state, true /* force */  );
 	   }
 	   else {
 	      // The recursive option is *NOT* applicable to events, hence ignore. According to Axel !!!!
 	      if ( stateOrEvent_ == Event::SET() )  {
 	         if (!node->set_event(the_event)) {
 	            std::stringstream ss;
 	            ss << "ForceCmd: force set: failed for node(" << node->absNodePath() << ") can not find event(" << the_event << ")";
 	            ecf::log(Log::ERR,ss.str());
 	            continue;
 	         }
 	      }
 	      else if ( stateOrEvent_ == Event::CLEAR() ) {
 	         if (!node->clear_event(the_event)) {
 	            std::stringstream ss;
 	            ss << "ForceCmd: force clear: failed for node(" << node->absNodePath() << ") can not find event(" << the_event << ")";
 	            ecf::log(Log::ERR,ss.str());
 	            continue;
 	         }
 	      }
 	      else  throw std::runtime_error("ForceCmd: Invalid parameter") ;
 	   }

 	   if ( recursive_ && setRepeatToLastValue_) {
 	      node->setRepeatToLastValueHierarchically();
 	   }
 	}

   std::string error_msg = ss.str();
   if (!error_msg.empty()) {
      throw std::runtime_error( error_msg ) ;
   }

	/// Change of state, do immediate job generation
   return doJobSubmission( as );
}

const char* ForceCmd::arg()  { return CtsApi::forceArg();}
const char* ForceCmd::desc() {
            /////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
   return
            "Force a node to a given state, or set its event.\n"
            "When a task is set to complete, it may be automatically re-queued if it has\n"
            "multiple future time dependencies. However each time we force a complete it will\n"
            "expire any time based attribute on that node. When the last time based attribute\n"
            "expires, the node will stay in a complete state.\n"
            "This behaviour allow Repeat values to be incremented interactively.\n"
            "A repeat attribute is incremented when all the child nodes are complete\n"
            "in this case the child nodes are automatically re-queued.\n"
            "  arg1 = [ unknown | complete | queued | submitted | active | aborted | clear | set ]\n"
            "  arg2 = (optional) recursive\n"
            "         Applies state to node and recursively to all its children\n"
            "  arg3 = (optional) full\n"
            "         Set repeat variables to last value, only works in conjunction\n"
            "         with recursive option\n"
            "  arg4 = path_to_node or path_to_node:<event>: paths must begin with '/'\n"
            "Usage:\n"
            "  --force=complete /suite/t1 /suite/t2   # Set task t1 & t2 to complete\n"
            "  --force=clear /suite/task:ev           # Clear the event 'ev' on task /suite/task\n"
            "  --force=complete recursive /suite/f1   # Recursively set complete all children of /suite/f1\n"
            "Effect:\n"
            "  Consider the effect of forcing complete when the current time is at 09:00\n"
            "  suite s1\n"
            "     task t1; time 12:00             # will complete straight away\n"
            "     task t2; time 10:00 13:00 01:00 # will complete on fourth attempt\n\n"
            "  --force=complete /s1/t1 /s1/t2\n"
            "  When we have a time range(i.e as shown with task t2), it is re-queued and the\n"
            "  next time slot is incremented for each complete, until it expires, and the task completes.\n"
            "  Use the Why command, to show next run time (i.e. next time slot)"
            ;
}

void ForceCmd::addOption(boost::program_options::options_description& desc) const {
	desc.add_options()( ForceCmd::arg(),po::value< vector<string> >()->multitoken(), ForceCmd::desc() );
}
void ForceCmd::create( 	Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv*  ac) const
{
	vector<string> args = vm[  arg() ].as< vector<string> >();

	if (ac->debug()) dumpVecArgs(ForceCmd::arg(),args);

	if (args.size() < 2 ) {
		std::stringstream ss;
		ss << "ForceCmd: At least two arguments expected for Force. Found " << args.size() << "\n"
		   << ForceCmd::desc() << "\n";
		throw std::runtime_error( ss.str() );
	}

	std::vector<std::string> options,paths;
   split_args_to_options_and_paths(args,options,paths); // relative order is still preserved
   if (paths.empty()) {
      std::stringstream ss;
      ss << "ForceCmd: No paths specified. Paths must begin with a leading '/' character\n" << ForceCmd::desc() << "\n";
      throw std::runtime_error( ss.str() );
   }
   if (options.empty()) {
      std::stringstream ss;
      ss << "ForceCmd: Invalid argument list. Expected of:\n"
         << "[ unknown | complete | queued | submitted | active | aborted | clear | set]\n" << ForceCmd::desc() << "\n";
      throw std::runtime_error( ss.str() );
   }

   bool is_valid_state = false;
   bool is_valid_event_state = false;
   bool setRepeatToLastValue = false;
   bool recursive = false;
   std::string stateOrEvent;
   size_t vec_size = options.size();
   for(size_t i = 0; i < vec_size; i++) {
      if (Str::caseInsCompare(options[i],"recursive"))  recursive = true;
      else if (Str::caseInsCompare( options[i],"full"))  setRepeatToLastValue = true;
      else if (NState::isValid(options[i])) { is_valid_state = true; stateOrEvent = options[i];}
      else if (Event::isValidState(options[i])) { is_valid_event_state = true;  stateOrEvent = options[i];}
      else {
         std::stringstream ss; ss << "ForceCmd: Invalid argument \n" << ForceCmd::desc() << "\n";
         throw std::runtime_error( ss.str() );
      }
   }

 	if (!is_valid_state && !is_valid_event_state) {
		std::stringstream ss;
		ss << "ForceCmd: Invalid node state or event expected one of:\n"
		   << "[ unknown | complete | queued | submitted | active | aborted | clear | set]\n";
		throw std::runtime_error( ss.str() );
	}

 	if ( is_valid_event_state ) {
 		// When set or clear used the path needs to include the name of the event:
 	   size_t vec_size = paths.size();
 	   for(size_t i = 0; i < vec_size; i++) {
 	      string the_event,the_path;
  	      Extract::pathAndName(paths[i],the_path, the_event);
  	      if ( the_path.empty() || the_event.empty() ) {
  	         std::stringstream ss;
  	         ss << "ForceCmd: When 'set' or 'clear' is specified the path needs to include name of the event i.e\n";
  	         ss << " --force=/path/to_task:event_name set\n";
  	         throw std::runtime_error( ss.str() );
  	      }
 	   }
 	}
	cmd = Cmd_ptr( new ForceCmd(paths, stateOrEvent, recursive, setRepeatToLastValue  ) );
}

std::ostream& operator<<(std::ostream& os, const ForceCmd& c) { return c.print(os); }
