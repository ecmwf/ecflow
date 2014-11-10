//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #270 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <assert.h>
#include <sstream>
#include <fstream>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Log.hpp"
#include "PrintStyle.hpp"
#include "NodeTreeVisitor.hpp"
#include "Jobs.hpp"
#include "Str.hpp"
#include "Extract.hpp"
#include "NodePath.hpp"
#include "Stl.hpp"
#include "Ecf.hpp"
#include "NodeState.hpp"
#include "ExprAst.hpp"       // required for persistence
#include "Serialization.hpp" // collates boost archive includes
#include "JobCreationCtrl.hpp"
#include "ResolveExternsVisitor.hpp"
#include "DefsDelta.hpp"
#include "ChangeMgrSingleton.hpp"
#include "ExprDuplicate.hpp"
#include "Version.hpp"
#include "Indentor.hpp"

using namespace ecf;
using namespace std;

//#define DEBUG_JOB_SUBMISSION 1
//#define DEBUG_MEMENTO 1

Defs::Defs() :
   state_change_no_(0),
   modify_change_no_( 0 ),
   updateCalendarCount_(0),
   order_state_change_no_(0),
   save_edit_history_(false),
   client_suite_mgr_(this)
{
}

defs_ptr Defs::create()
{
	return boost::make_shared<Defs>();
}

Defs::~Defs()
{
//    cout << "   Deleting defs "\n";
   // Don't create the ChangeMgrSingleton during destruct sequence. (i.e in unit cases)
   // Since that will cause a memory leak
   if (!Ecf::server() && ChangeMgrSingleton::exists()) {
      ChangeMgrSingleton::instance()->notify_delete( this );
   }

   // Duplicate AST are held in a static map. Delete them, to avoid valgrind complaining
   ExprDuplicate reclaim_cloned_ast_memory;
}

///// State relation functions: ==================================================
NState::State Defs::state() const
{
	return state_.state();
}

void Defs::set_state_only(NState::State the_new_state)
{
   state_.setState( the_new_state ); // this will update state_change_no
}

void Defs::set_state(NState::State the_new_state)
{
   set_state_only( the_new_state ); // this will update state_change_no

  	// Log the state change
   //           " " +  submitted(max) + ": /"
   // reserve : 1   +  9              + 3      = 13
   std::string log_state_change; log_state_change.reserve(13);
   log_state_change += " ";
   log_state_change += NState::toString(the_new_state);
   log_state_change += ": /";
   ecf::log(Log::LOG,log_state_change);
}

void Defs::set_most_significant_state()
{
   NState::State computedStateOfImmediateChildren = ecf::theComputedNodeState(suiteVec_, true /* immediate children only */ );
   if (computedStateOfImmediateChildren != state_.state() )
      set_state(  computedStateOfImmediateChildren );
}

/// Others ======================================================================
void Defs::check_job_creation(  job_creation_ctrl_ptr jobCtrl )
{
   /// Job generation checking. is done via the python API
   /// As such it done directly on the Defs.
   /// However Job generation checking will end up changing the states of the DEFS
   /// If this defs is loaded into the server the state of each node may be surprising. (i.e submitted)
   /// Hence we need to reset the state.

   if (!jobCtrl.get()) {
      throw std::runtime_error("Defs::check_job_creation: NULL JobCreationCtrl passed");
   }

   // This function should NOT really change the data model
   // The changed state is reset, hence we need to preserve change and modify numbers
   EcfPreserveChangeNo preserveChangeNo;

   // Do *not* modify suspended state of child nodes
   int clear_suspended_in_child_nodes = -1;

 	if (jobCtrl->node_path().empty()) {

 		size_t theSize = suiteVec_.size();
 		for(size_t s = 0; s < theSize; s++) {
 		   /// begin will cause creation of generated variables. The generated variables
 		   /// are use in client scripts and used to locate the ecf files
 			suiteVec_[s]->begin();
 			suiteVec_[s]->check_job_creation( jobCtrl ) ;

 			/// reset the state
         suiteVec_[s]->requeue(true,clear_suspended_in_child_nodes,true);
         suiteVec_[s]->reset_begin();
         suiteVec_[s]->setStateOnlyHierarchically( NState::UNKNOWN );
 		}
	}
	else {

		node_ptr node =  findAbsNode( jobCtrl->node_path() );
		if (node.get()) {
		   /// begin will cause creation of generated variables. The generated variables
		   /// are use in client scripts and used to locate the ecf files
		   node->suite()->begin();
			node->check_job_creation( jobCtrl );

			/// reset the state
         node->requeue(true,clear_suspended_in_child_nodes,true);
         node->suite()->reset_begin();
         node->setStateOnlyHierarchically( NState::UNKNOWN );
		}
		else {
 		    std::stringstream ss;
		    ss << "Defs::check_job_creation: failed as node path '"  << jobCtrl->node_path() << "' does not exist.\n";
 		    jobCtrl->error_msg() =  ss.str();
 		}
	}
}

void Defs::do_generate_scripts( const std::map<std::string,std::string>& override) const
{
   size_t theSize = suiteVec_.size();
   for(size_t s = 0; s < theSize; s++) {
      suiteVec_[s]->generate_scripts(override);
   }
}
void Defs::generate_scripts() const
{
   std::map<std::string,std::string> override;
   do_generate_scripts(override);
}


void Defs::updateCalendar( const ecf::CalendarUpdateParams & calUpdateParams)
{
	/// Collate any auto cancelled nodes as a result of calendar update
	std::vector<node_ptr> auto_cancelled_nodes;

	// updateCalendarCount_ is only used in *test*
	updateCalendarCount_++;

	size_t theSize = suiteVec_.size();
	for(size_t s = 0; s < theSize; s++) {
		suiteVec_[s]->updateCalendar( calUpdateParams, auto_cancelled_nodes);
	}

	// Permanently remove any auto-cancelled nodes.
 	if ( !auto_cancelled_nodes.empty() ) {
 		std::vector<node_ptr>::iterator theNodeEnd = auto_cancelled_nodes.end();
 		string msg;
 		for(std::vector<node_ptr>::iterator n = auto_cancelled_nodes.begin(); n != theNodeEnd; ++n) {
 		   msg.clear(); msg = "autocancel "; msg += (*n)->debugNodePath();
 			ecf::log(Log::MSG,msg);
 			(*n)->remove();
  		}
 	}
}


void Defs::absorb(Defs* input_defs, bool force)
{
	// Dont absorb myself.
	if (input_defs == this) {
		return;
	}

	// updateCalendarCount_ is *only* used in test, reset whenever a new defs is loaded
	updateCalendarCount_ = 0;

	// We must make a copy, otherwise we are iterating over a vector that is being deleted
	std::vector<suite_ptr> suiteVecCopy = input_defs->suiteVec();
	size_t theSize = suiteVecCopy.size();
	for(size_t s = 0; s < theSize; s++) {

		/// regardless remove the suite from the input defs
		suite_ptr the_input_suite = input_defs->removeSuite(suiteVecCopy[s]);

		if (force) {
			/// The suite of the same name exists. remove it from *existing* defs
		   suite_ptr existing_suite = findSuite( the_input_suite->name() );
		   if (existing_suite.get()) {
 				removeSuite( existing_suite );
 			}
		}

		/// Add the suite. Will throw if suite of same name already exists.
		/// This stops accidental overwrite
		addSuite( the_input_suite  );
	}
  	LOG_ASSERT( input_defs->suiteVec().empty(),"Defs::absorb");

  	// Copy over server user variables
   set_server().add_or_update_user_variables( input_defs->server().user_variables() );

  	// This only works on the client side. since server does not store externs
  	const set<string>& ex = input_defs->externs();
  	for(set<string>::const_iterator i = ex.begin(); i != ex.end(); ++i) {
  	   add_extern(*i);
  	}
}

void Defs::accept(ecf::NodeTreeVisitor& v)
{
	v.visitDefs(this);
	size_t theSuiteVecSize = suiteVec_.size();
	for(size_t i = 0; i < theSuiteVecSize; i++) { suiteVec_[i]->accept(v); }
}

void Defs::acceptVisitTraversor(ecf::NodeTreeVisitor& v)
{
	LOG_ASSERT(v.traverseObjectStructureViaVisitors(),"");
	v.visitDefs(this);
}

bool Defs::verification(std::string& errorMsg) const
{
	size_t theSuiteVecSize = suiteVec_.size();
	for(size_t i = 0; i < theSuiteVecSize; i++) { suiteVec_[i]->verification(errorMsg); }
	return errorMsg.empty();
}

suite_ptr Defs::add_suite(const std::string& name)
{
   if (findSuite(name).get()) {
      std::stringstream ss;
      ss << "Add Suite failed: A Suite of name '" << name << "' already exist";
      throw std::runtime_error( ss.str() );
   }
   suite_ptr the_suite = Suite::create(name);
   add_suite_only( the_suite , std::numeric_limits<std::size_t>::max());
   return the_suite;
}

void Defs::addSuite(suite_ptr s, size_t position)
{
	if (findSuite(s->name()).get()) {
 		std::stringstream ss;
		ss << "Add Suite failed: A Suite of name '" << s->name() << "' already exist";
		throw std::runtime_error( ss.str() );
	}
	add_suite_only( s , position);
}

void Defs::add_suite_only(suite_ptr s, size_t position)
{
   if (s->defs()) {
      std::stringstream ss;
      ss << "Add Suite failed: The suite of name '" << s->name() << "' already owned by another Defs ";
      throw std::runtime_error( ss.str() );
   }

   s->set_defs(this);
   if (position >= suiteVec_.size()) {
      suiteVec_.push_back(s);
   }
   else {
      suiteVec_.insert( suiteVec_.begin() + position, s);
   }
   Ecf::incr_modify_change_no();
   client_suite_mgr_.suite_added_in_defs(s);
}

suite_ptr Defs::removeSuite(suite_ptr s)
{
	std::vector<suite_ptr>::iterator i = std::find(suiteVec_.begin(), suiteVec_.end(),s);
 	if ( i != suiteVec_.end()) {
 	   s->set_defs(NULL);              // allows suite to added to different defs
		suiteVec_.erase(i);             // iterator invalidated
	 	Ecf::incr_modify_change_no();
	 	client_suite_mgr_.suite_deleted_in_defs(s); // must be after Ecf::incr_modify_change_no();
		return s; // transfer ownership of suite
	}

 	// Something serious has gone wrong. Can not find the suite
 	cout << "Defs::removeSuite: assert failure:  suite '" << s->name() << "' suiteVec_.size() = " << suiteVec_.size() << "\n";
	for(unsigned i = 0; i < suiteVec_.size(); ++i) { cout << i << " " << suiteVec_[i]->name() << "\n";}
 	LOG_ASSERT(false,"Defs::removeSuite the suite not found");
	return suite_ptr();
}

size_t Defs::child_position(const Node* child) const
{
   size_t vecSize = suiteVec_.size();
   for(size_t t = 0; t < vecSize; t++)     {
      if (suiteVec_[t].get() == child) {
         return t;
      }
   }
   return std::numeric_limits<std::size_t>::max();
}

node_ptr Defs::removeChild(Node* child)
{
  	size_t vecSize = suiteVec_.size();
 	for(size_t t = 0; t < vecSize; t++)     {
 		if (suiteVec_[t].get() == child) {
 		 	Ecf::incr_modify_change_no();
 		   suiteVec_[t]->set_defs(NULL); // Must be set to NULL, allows suite to be added to different defs
 		 	client_suite_mgr_.suite_deleted_in_defs(suiteVec_[t]); // must be after Ecf::incr_modify_change_no();
 			node_ptr node = boost::dynamic_pointer_cast<Node>(suiteVec_[t]);
 			suiteVec_.erase( suiteVec_.begin() + t);
 			return node ;
 		}
  	}

 	// Something has gone wrong.
	cout << "Defs::removeChild: assert failed:  suite '" << child->name() << "' suiteVec_.size() = " << suiteVec_.size() << "\n";
	for(unsigned i = 0; i < suiteVec_.size(); ++i) { cout << i << " " << suiteVec_[i]->name() << "\n";}
 	LOG_ASSERT(false,"Defs::removeChild,the suite not found");
	return node_ptr();
}

bool Defs::addChild( node_ptr child, size_t position)
{
	LOG_ASSERT(child.get(),"");
	LOG_ASSERT(child->isSuite(),"");

	// *** CANT construct shared_ptr from a raw pointer, must use dynamic_pointer_cast,
	// *** otherwise the reference counts will get messed up.
	// If the suite of the same exists, it is deleted first
	addSuite( boost::dynamic_pointer_cast<Suite>( child ), position );
	return true;
}

void Defs::add_extern(const std::string& ex )
{
   if (ex.empty()) {
      throw std::runtime_error("Defs::add_extern: Can not add empty extern");
   }
   externs_.insert(ex);
}

void Defs::auto_add_externs(bool remove_existing_externs_first)
{
	if (remove_existing_externs_first) {
		externs_.clear();
	}
	/// Automatically add externs
	ResolveExternsVisitor visitor(this);
	acceptVisitTraversor(visitor);
}

void Defs::beginSuite(suite_ptr suite)
{
   if (!suite.get()) throw std::runtime_error( "Defs::beginSuite: Begin failed as suite is not loaded" );

   if (!suite->begun()) {
      // Hierarchical set the state. Handle case where we have children that are all defstatus complete
      // and hence needs parent set to complete. See Simulator/good_defs/operations/naw.def
      //	  family naw
      //	    family general
      //	      time 06:00
      //	      task metgrams
      //	        defstatus complete
      //	      task equipot
      //	        defstatus complete
      //	    endfamily
      suite->begin();

      set_most_significant_state();
   }
   else {
      LOG(Log::WAR,"Suite " << suite->name() << " has already begun");
   }
}

void Defs::beginAll()
{
   bool at_least_one_suite_begun = false;
	size_t theSuiteVecSize = suiteVec_.size();
	for(size_t s = 0; s < theSuiteVecSize; s++) {
	   if ( !suiteVec_[s]->begun() ) {
	      suiteVec_[s]->begin();
	      at_least_one_suite_begun = true;
	   }
	}

	if ( at_least_one_suite_begun ) {
	   set_most_significant_state();
	}
}

void Defs::reset_begin()
{
   std::for_each(suiteVec_.begin(),suiteVec_.end(),boost::bind(&Suite::reset_begin,_1));
}

void Defs::requeue()
{
   bool edit_history_set = flag().is_set(ecf::Flag::MESSAGE);
   flag_.reset();
   if (edit_history_set) flag().set(ecf::Flag::MESSAGE);

   int clear_suspended_in_child_nodes = 0;
   size_t theSuiteVecSize = suiteVec_.size();
   for(size_t s = 0; s < theSuiteVecSize; s++) {
      suiteVec_[s]->requeue( true /* reset repeats */,
                             clear_suspended_in_child_nodes,
                             true /* reset_next_time_slot */);
   }

   set_most_significant_state();
}


void Defs::check_suite_can_begin(suite_ptr suite) const
{
   NState::State suiteState = suite->state();
   if (!suite->begun() && suiteState != NState::UNKNOWN && suiteState != NState::COMPLETE) {
      int count = 0;
      std::vector<Task*> tasks;
      getAllTasks(tasks);
      std::stringstream ts;
      for(size_t i =0; i < tasks.size(); i++) {
         if (tasks[i]->state() == NState::ACTIVE || tasks[i]->state() == NState::SUBMITTED) {
            ts << "   " << tasks[i]->absNodePath() << "\n";
            count++;
         }
      }
      /// allow suite to begin even its aborted provide no tasks in active or submitted states
      if (count > 0) {
         std::stringstream ss;
         ss << "Begin failed as suite "
                  << suite->name() << "(computed state=" << NState::toString(suiteState)
         << ") can only begin if its in UNKNOWN or COMPLETE state\n";
         ss << "Found " << count << " tasks with state 'active' or 'submitted'\n";
         ss << ts.str();
         ss << "Use the force argument to bypass this check, at the risk of creating zombies\n";
         throw std::runtime_error( ss.str() );
      }
   }
}

bool Defs::hasTimeDependencies() const
{
	size_t theSuiteVecSize = suiteVec_.size();
	for(size_t s = 0; s < theSuiteVecSize; s++) {
		if ( suiteVec_[s]->hasTimeDependencies() ) return true;
	}
	return false;
}

std::ostream& Defs::print(std::ostream& os) const
{
   os << "# " << ecf::Version::raw() << "\n";
	if (!PrintStyle::defsStyle()) {
	   os << write_state();
	}

	set<string>::const_iterator extern_end = externs_.end();
	for(set<string>::const_iterator i = externs_.begin(); i != extern_end; ++i) {
      os << "extern " << *i << "\n";
	}
	size_t the_size = suiteVec_.size();
	for(size_t s = 0; s < the_size; s++) {
	   os << *suiteVec_[s];
	}
	return os;
}

std::string Defs::write_state() const
{
   // *IMPORTANT* we *CANT* use ';' character, since is used in the parser, when we have
   //             multiple statement on a single line i.e.
   //                 task a; task b;
   // *IMPORTANT* make sure name are unique, i.e can't have state: and server_state:
   // Otherwise read_state() will mess up
   std::stringstream os;
   os << "defs_state";
   os << " " << PrintStyle::to_string();
   if (state_ != NState::UNKNOWN) os << " state>:" << NState::toString(state_); // make <state> is unique
   if (flag_.flag() != 0) os << " flag:" << flag_.to_string();
   if (state_change_no_ != 0) os << " state_change:" << state_change_no_;
   if (modify_change_no_ != 0) os << " modify_change:" << modify_change_no_;
   if (server().get_state() != ServerState::default_state()) os << " server_state:" << SState::to_string(server().get_state());
   os << "\n";

   // This read by the DefsStateParser
   const std::vector<Variable>& theServerEnv = server().user_variables();
   for(size_t i = 0; i < theServerEnv.size(); ++i) {
      theServerEnv[i].print(os);
   }

   // READ by Defs::read_history()
   // We need to define a separator for the message, will to allow it to be re-read
   // This separator can not be :
   // ' ' space, used in the messages
   // %  Used in job submission
   // :  Used in time, and name (:ma0)
   // [] Used in time
   // integers used in the time.
   // -  Used in commands
   if (PrintStyle::getStyle() == PrintStyle::MIGRATE || save_edit_history_) {
      Indentor in;
      std::map<std::string, std::deque<std::string> >::const_iterator i;
      for(i=edit_history_.begin(); i != edit_history_.end(); ++i) {
         Indentor::indent( os ) << "history " << (*i).first << " ";// node path
         const std::deque<std::string>& vec = (*i).second;   // list of requests
         for(std::deque<std::string>::const_iterator c = vec.begin(); c != vec.end(); ++c) {
            os << "\b" << *c;
         }
         os << "\n";
      }
      save_edit_history_ = false;
   }
   return os.str();
}

void Defs::read_state(const std::string& line,const std::vector<std::string>& lineTokens)
{
//   cout << "line = " << line << "\n";
   std::string token;
   for(size_t i = 2; i < lineTokens.size(); i++) {
      token.clear();
      if (lineTokens[i].find("state>:") != std::string::npos) {
         if (!Extract::split_get_second(lineTokens[i],token)) throw std::runtime_error( "Defs::read_state: state extraction failed : " + lineTokens[i] );
         if (!NState::isValid(token)) throw std::runtime_error( "Defs::read_state: invalid state specified : " + token );
         set_state_only(NState::toState(token));
      }
      else if (lineTokens[i].find("flag:") != std::string::npos) {
         if (!Extract::split_get_second(lineTokens[i],token))throw std::runtime_error( "Defs::read_state: Invalid flag specified : " + line );
         flag().set_flag(token); // this can throw
      }
      else if (lineTokens[i].find("state_change:") != std::string::npos) {
         if (!Extract::split_get_second(lineTokens[i],token)) throw std::runtime_error( "Defs::read_state: Invalid state_change specified : " + line );
         int sc = Extract::theInt(token,"Defs::read_state: invalid state_change specified : " + line);
         set_state_change_no(sc);
      }
      else if (lineTokens[i].find("modify_change:") != std::string::npos) {
         if (!Extract::split_get_second(lineTokens[i],token)) throw std::runtime_error( "Defs::read_state: Invalid modify_change specified : " + line );
         int mc = Extract::theInt(token,"Defs::read_state: invalid state_change specified : " + line);
         set_modify_change_no(mc);
      }
      else if (lineTokens[i].find("server_state:") != std::string::npos) {
         if (!Extract::split_get_second(lineTokens[i],token)) throw std::runtime_error( "Defs::read_state: Invalid server_state specified : " + line );
         if (!SState::isValid(token)) throw std::runtime_error( "Defs::read_state: Invalid server_state specified : " + line );
         set_server().set_state(SState::toState(token));
      }
   }
}

void Defs::read_history(const std::string& line,const std::vector<std::string>& lineTokens)
{
   // expect:
   // history <node_path> \bmsg1\bmsg2
   // The message can contain spaces,
   // Multiple spaces will be lost !!
   if ( lineTokens.size() < 2 ) throw std::runtime_error( "Defs::read_history: Invalid history " + line );

   DefsHistoryParser parser;
   parser.parse(line);

   const std::vector<std::string>& parsed_messages =  parser.parsed_messages();
   for(size_t i = 0; i < parsed_messages.size(); i++) {
      add_edit_history(lineTokens[1],parsed_messages[i]);
   }
}

bool Defs::compare_edit_history(const Defs& rhs) const
{
   if (edit_history_ != rhs.edit_history_) return false;
   return true;
}


bool Defs::operator==(const Defs& rhs) const
{
	if ( state() != rhs.state()) {
#ifdef DEBUG
		if (Ecf::debug_equality()) {
			std::cout << "Defs::operator==  state(" << NState::toString(state()) << ") != rhs.state(" << NState::toString(rhs.state()) << ")) \n";
		}
#endif
 		return false;
	}

	if ( server_ != rhs.server() ) {
#ifdef DEBUG
		if (Ecf::debug_equality()) {
			std::cout << "Defs::operator== server_ != rhs.server())\n";
		}
#endif
 		return false;
	}

   if ( flag_ != rhs.flag_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Defs::operator== ( flag_ != rhs.flag_) : '" << flag_.to_string() << "' != '" << rhs.flag_.to_string() << "'\n";
      }
#endif
      return false;
   }

	/// Note:: WE specifically exclude testing of externs.
	/// Externs are not persisted, hence can not take part in comparison
	/// Externs only live on the client side.

	if ( suiteVec_.size() != rhs.suiteVec_.size()) {
#ifdef DEBUG
		if (Ecf::debug_equality()) {
			std::cout << "Defs::operator==    suiteVec_.size(" << suiteVec_.size() << ") != rhs.suiteVec_.size( " << rhs.suiteVec_.size() << ") \n";
		}
#endif
 		return false;
	}
	for(unsigned i =0; i < suiteVec_.size(); ++i) {
		if ( !( *(suiteVec_[i]) == *(rhs.suiteVec_[i]) )) {
#ifdef DEBUG
			if (Ecf::debug_equality()) {
				std::cout << "Defs::operator==    !( *(suiteVec_[i]) == *(rhs.suiteVec_[i]) )\n";
			}
#endif
			return false;
		}
	}
 	return true;
}

node_ptr Defs::findAbsNode(const std::string& pathToNode) const
{
//	std::cout << "Defs::findAbsNode " << pathToNode << "\n";
 	// The pathToNode is of the form:
	//     /suite
 	//     /suite/family
 	//     /suite/family/task
 	//     /suite/family/family/family/task

 	std::vector<std::string> theNodeNames; theNodeNames.reserve(4);
 	NodePath::split(pathToNode,theNodeNames);
	if ( theNodeNames.empty() ) {
		return node_ptr();
	}


	size_t child_pos = 0 ; // unused
   size_t pathSize = theNodeNames.size();
   size_t theSuiteVecSize = suiteVec_.size();
   for(size_t s = 0; s < theSuiteVecSize; s++)  {

      size_t index = 0;
      if (theNodeNames[index] == suiteVec_[s]->name()) {

         node_ptr the_node = suiteVec_[s];
         if (pathSize == 1) return the_node;
         index++; // skip over suite,

         while (index < pathSize) {
            the_node = the_node->findImmediateChild(theNodeNames[index],child_pos);
            if (the_node) {
               if (index == pathSize - 1) return the_node;
               index++;
            }
            else {
               return node_ptr();
            }
         }
         return node_ptr();
      }
	}

 	return node_ptr();
}

node_ptr Defs::find_closest_matching_node(const std::string& pathToNode) const
{
	std::vector<std::string> theNodeNames;
 	NodePath::split(pathToNode,theNodeNames);
	if ( theNodeNames.empty() )  return node_ptr();

	node_ptr closest_matching_node;
	int index = 0;
	size_t theSuiteVecSize = suiteVec_.size();
	for(size_t s = 0; s < theSuiteVecSize; s++)  {
		suiteVec_[s]->find_closest_matching_node(theNodeNames,index,closest_matching_node);
		if (closest_matching_node.get()) return closest_matching_node;
	}
 	return node_ptr();
}


bool Defs::find_extern( const std::string& pathToNode , const std::string& node_attr_name ) const
{
   if (externs_.empty()) {
      return false;
   }

   if (node_attr_name.empty()) {
      if (externs_.find(pathToNode) != externs_.end()) {
         return true;
      }
      return false;
   }

   std::string extern_path = pathToNode;
   extern_path += Str::COLON();
   extern_path += node_attr_name;

   if (externs_.find(extern_path) != externs_.end()) {
      return true;
   }
   return false;
}


suite_ptr Defs::findSuite(const std::string& name) const
{
	size_t theSuiteVecSize = suiteVec_.size();
	for(size_t s = 0; s < theSuiteVecSize; s++) {
		if (suiteVec_[s]->name() == name) {
			return suiteVec_[s];
		}
 	}
	return suite_ptr();
}

bool Defs::check(std::string& errorMsg,std::string& warningMsg) const
{
	size_t theSuiteVecSize = suiteVec_.size();
	for(size_t s = 0; s < theSuiteVecSize; s++) { suiteVec_[s]->check(errorMsg,warningMsg); }
	return errorMsg.empty();
}

void Defs::getAllTasks(std::vector<Task*>& tasks) const
{
	size_t theSuiteVecSize = suiteVec_.size();
	for(size_t s = 0; s < theSuiteVecSize; s++) { suiteVec_[s]->getAllTasks(tasks);}
}

void Defs::getAllSubmittables(std::vector<Submittable*>& tasks) const
{
   size_t theSuiteVecSize = suiteVec_.size();
   for(size_t s = 0; s < theSuiteVecSize; s++) { suiteVec_[s]->getAllSubmittables(tasks);}
}

void Defs::get_all_active_submittables(std::vector<Submittable*>& tasks) const
{
   size_t theSuiteVecSize = suiteVec_.size();
   for(size_t s = 0; s < theSuiteVecSize; s++) { suiteVec_[s]->get_all_active_submittables(tasks);}
}

void Defs::get_all_tasks(std::vector<task_ptr>& tasks) const
{
   size_t theSuiteVecSize = suiteVec_.size();
   for(size_t s = 0; s < theSuiteVecSize; s++) { suiteVec_[s]->get_all_tasks(tasks);}
}

void Defs::get_all_nodes(std::vector<node_ptr>& nodes) const
{
   size_t theSuiteVecSize = suiteVec_.size();
   for(size_t s = 0; s < theSuiteVecSize; s++) { suiteVec_[s]->get_all_nodes(nodes);}
}

void Defs::get_all_aliases(std::vector<alias_ptr>& aliases) const
{
   size_t theSuiteVecSize = suiteVec_.size();
   for(size_t s = 0; s < theSuiteVecSize; s++) { suiteVec_[s]->get_all_aliases(aliases);}
}

void Defs::getAllFamilies(std::vector<Family*>& vec) const
{
	size_t theSuiteVecSize = suiteVec_.size();
	for(size_t s = 0; s < theSuiteVecSize; s++) { suiteVec_[s]->getAllFamilies(vec);}
}

void Defs::getAllNodes(std::vector<Node*>& vec) const
{
	size_t theSuiteVecSize = suiteVec_.size();
   vec.reserve(vec.size() + theSuiteVecSize);
	for(size_t s = 0; s < theSuiteVecSize; s++) {
	   vec.push_back(suiteVec_[s].get());
	   suiteVec_[s]->getAllNodes(vec);
	}
}

void Defs::getAllAstNodes(std::set<Node*>& theSet) const
{
	size_t theSuiteVecSize = suiteVec_.size();
	for(size_t s = 0; s < theSuiteVecSize; s++) { suiteVec_[s]->getAllAstNodes(theSet);}
}

bool Defs::deleteChild(Node* nodeToBeDeleted)
{
  	Node* parent = nodeToBeDeleted->parent();
  	if (parent)  return parent->doDeleteChild(nodeToBeDeleted);
 	return doDeleteChild(nodeToBeDeleted);
}

bool Defs::doDeleteChild(Node* nodeToBeDeleted)
{
//	std::cout << "Defs::doDeleteChild nodeToBeDeleted   = " << nodeToBeDeleted->debugNodePath() << "\n";

	std::vector<suite_ptr>::iterator theSuiteEnd = suiteVec_.end();
 	for(std::vector<suite_ptr>::iterator s = suiteVec_.begin(); s!=theSuiteEnd; ++s) {
 		if ( (*s).get() == nodeToBeDeleted) {
  		 	Ecf::incr_modify_change_no();
  		 	client_suite_mgr_.suite_deleted_in_defs(*s); // must be after Ecf::incr_modify_change_no();
  		 	(*s)->set_defs(NULL); // Must be set to NULL, allows re-added to a different defs
  			suiteVec_.erase(s);
  			set_most_significant_state(); // must be after suiteVec_.erase(s);
  			return true;
 		}
 	}

 	// recurse down only if we did not remove the suite
 	for(std::vector<suite_ptr>::iterator s = suiteVec_.begin(); s!=theSuiteEnd; ++s) {
 		// SuiteChanged is called within doDeleteChild
 		if ((*s)->doDeleteChild(nodeToBeDeleted)) {
 			return true;
 		}
 	}
	return false;
}

bool Defs::replaceChild(const std::string& path,
	               const defs_ptr& clientDefs,
	               bool createNodesAsNeeded,
	               bool force,
	               std::string& errorMsg)
{
	node_ptr clientNode =  clientDefs->findAbsNode( path );
	if (! clientNode.get() ) {
		errorMsg = "Can not replace node since path "; errorMsg += path;
		errorMsg += " does not exist on the client definition";
		return false;
	}

	node_ptr serverNode = findAbsNode( path ) ;
	if (!force && serverNode.get()) {
		// Check if serverNode has child tasks in submitted or active states
		vector<Task*> taskVec;
		serverNode->getAllTasks(taskVec); // taskVec will be empty if serverNode is a task
 		int count = 0;
		BOOST_FOREACH(Task* t, taskVec) { if (t->state() == NState::ACTIVE || t->state() == NState::SUBMITTED)  count++;}
		if (count != 0) {
			std::stringstream ss;
			ss << "Can not replace node " << serverNode->debugNodePath() << " because it has " << count << " tasks which are active or submitted\n";
			ss << "Please use the 'force' option to bypass this check, at the expense of creating zombies\n";
			errorMsg += ss.str();
			return false;
 		}
	}

	/// REPLACE ===========================================================
 	if (!createNodesAsNeeded || serverNode.get()) {
		// Then the child must exist in the server defs (i.e. this)
		if (! serverNode.get() ) {
			errorMsg = "Can not replace child since path "; errorMsg += path;
			errorMsg += " does not exist on the server definition. Please use <parent> option";
			return false;
		}
		// HAVE a FULL match in the server

		// Copy over begun and suspended states
 		if (serverNode->suite()->begun())  clientNode->begin();
 		if (serverNode->isSuspended())     clientNode->suspend();

 		// Find the position of the server node relative to its peers
 		// We use this to re-add client node at the same position
 		size_t child_pos = serverNode->position();

 		// Delete node on the server, Must recurse down
		Node* parentNodeOnServer = serverNode->parent();
		deleteChild(serverNode.get());

		// Remove reference in the client defs to clientNode and detach from its parent
		// transfer ownership to the server
		bool addOk = false;
		node_ptr client_node_to_add = clientNode->remove();
	 	if ( parentNodeOnServer ) addOk = parentNodeOnServer->addChild( client_node_to_add , child_pos);
		else                      addOk = addChild( client_node_to_add , child_pos);
	 	LOG_ASSERT(addOk,"");

	 	client_node_to_add->set_most_significant_state_up_node_tree();

	 	// The changes have been made, do a sanity test, check trigger expressions
	 	std::string warning_msg;
	 	return client_node_to_add->suite()->check(errorMsg,warning_msg);
 	}


 	// ADD ======================================================================
	// Create/Add nodes as needed for a *PARTIAL* match
	// If the path is /a/b/c/d/e/f it may be that path /a/b already exists
	// hence we need only create the missing nodes   c, d, e, f
	LOG_ASSERT( serverNode == NULL, "" );
	node_ptr server_parent;
	Node* last_client_child = clientNode.get(); // remember the last child
 	Node* client_parent = clientNode->parent();
	while (client_parent) {
	   server_parent = findAbsNode( client_parent->absNodePath() );
      if (server_parent ) {
         break;
      }
      last_client_child = client_parent;
      client_parent = client_parent->parent();
	}
	if (server_parent.get() == NULL) {
		// NOT EVEN A PARTIAL path match, hence move over WHOLE suite, detach from client and add to server
      node_ptr client_suite_to_add = clientNode->suite()->remove();
 		bool addOk = addChild( client_suite_to_add  );
 		LOG_ASSERT( addOk ,"");
 		client_suite_to_add->set_most_significant_state_up_node_tree();

      // The changes have been made, do a sanity test, check trigger expressions
 	   std::string warning_msg;
 	   return client_suite_to_add->suite()->check(errorMsg,warning_msg);
	}


	// PARTIAL PATH MATCH,
   LOG_ASSERT( last_client_child ,"");
   LOG_ASSERT( client_parent ,"");
   LOG_ASSERT( last_client_child->parent() == client_parent ,"");
	LOG_ASSERT( client_parent->absNodePath() == server_parent->absNodePath() ,"");

	/// If the child of same name exist we *replace* at the same position otherwise we *add* it to the end
	size_t client_child_pos = last_client_child->position();

	size_t server_child_pos; // will be set to  std::numeric_limits<std::size_t>::max(), if child not found
	node_ptr server_child = server_parent->findImmediateChild(last_client_child->name(),server_child_pos);
	if (server_child.get()) {

	   // Copy over suspended state
	   if (server_child->isSuspended()) {
	      last_client_child->suspend();
	   }

	   // Child of same name exist on the server, hence remove it
	   deleteChild(server_child.get());
	}

	/// copy over begin/queued status
	if (server_parent->suite()->begun())  {
	   last_client_child->begin();
	}

	node_ptr client_node_to_add = last_client_child->remove();
	bool addOk = server_parent->addChild( client_node_to_add , client_child_pos);
	LOG_ASSERT( addOk,"" );
	client_node_to_add->set_most_significant_state_up_node_tree();

   // The changes have been made, do a sanity test, check trigger expressions
	std::string warning_msg;
	return client_node_to_add->suite()->check(errorMsg,warning_msg);
}

void Defs::save_as_checkpt(const std::string& the_fileName,ecf::Archive::Type at) const
{
   // only_save_edit_history_when_check_pointing or if explicitly requested
   save_edit_history_ = true;   // this is reset after edit_history is saved

	/// Can throw archive exception
 	ecf::save(the_fileName,*this,at);
}

void Defs::save_checkpt_as_string(std::string& output) const
{
   // only_save_edit_history_when_check_pointing or if explicitly requested
   save_edit_history_ = true;   // this is reset after edit_history is saved

   ecf::save_as_string(output,*this);
}

void Defs::restore_from_checkpt(const std::string& the_fileName,ecf::Archive::Type at)
{
//	cout << "Defs::restore_from_checkpt " << the_fileName << "\n";

	if (the_fileName.empty())  return;

	// deleting existing content first. *** Note: Server environment left as is ****
	clear();

	ecf::restore(the_fileName, (*this), at);

	// Reset the state and modify numbers, **After the restore**
   state_change_no_ = Ecf::state_change_no();
   modify_change_no_ = Ecf::modify_change_no();

//	cout << "Restored: " << suiteVec_.size() << " suites\n";
}

void Defs::clear()
{
   // Duplicate AST are held in a static map.
   ExprDuplicate reclaim_cloned_ast_memory;

   // *** Note: Server environment left as is ****
   suiteVec_.clear();
   externs_.clear();
   client_suite_mgr_.clear();
   state_.setState(NState::UNKNOWN);
   edit_history_.clear();
   save_edit_history_ = false;
   Ecf::incr_modify_change_no();
}

bool Defs::checkInvariants(std::string& errorMsg) const
{
	size_t vecSize = suiteVec_.size();
	for(size_t s = 0; s < vecSize; s++) {
		if (suiteVec_[s]->defs() != this) {
		   std::stringstream ss;
		   ss << "Defs::checkInvariants suite->defs() function not correct. Child suite parent ptr not correct\n";
		   ss << "For suite " << suiteVec_[s]->name();
			errorMsg += ss.str();
			return false;
		}
		if (!suiteVec_[s]->checkInvariants(errorMsg)) {
			return false;
		}
 	}

   if (Ecf::server()) {
      /// The change no should NOT be greater than Ecf::state_change_no()

      if (state_change_no_ > Ecf::state_change_no() ) {
         std::stringstream ss;
         ss << "Defs::checkInvariants: state_change_no(" << state_.state_change_no() << ") > Ecf::state_change_no(" << Ecf::state_change_no() << ")\n";
         errorMsg += ss.str();
         return false;
      }
      if (modify_change_no_ > Ecf::modify_change_no() ) {
         std::stringstream ss;
         ss << "Defs::checkInvariants: modify_change_no_(" << modify_change_no_ << ") > Ecf::modify_change_no(" << Ecf::modify_change_no() << ")\n";
         errorMsg += ss.str();
         return false;
      }

      if (flag_.state_change_no() > Ecf::state_change_no() ) {
         std::stringstream ss;
         ss << "Defs::checkInvariants: flag.state_change_no()(" << flag_.state_change_no() << ") > Ecf::state_change_no(" << Ecf::state_change_no() << ")\n";
         errorMsg += ss.str();
         return false;
      }


      if (state_.state_change_no() > Ecf::state_change_no() ) {
         std::stringstream ss;
         ss << "Defs::checkInvariants: state_.state_change_no()(" << state_.state_change_no() << ") > Ecf::state_change_no(" << Ecf::state_change_no() << ")\n";
         errorMsg += ss.str();
         return false;
      }

      if (server_.state_change_no() > Ecf::state_change_no() ) {
         std::stringstream ss;
         ss << "Defs::checkInvariants: server_.state_change_no()(" << server_.state_change_no() << ") > Ecf::state_change_no(" << Ecf::state_change_no() << ")\n";
         errorMsg += ss.str();
         return false;
      }
   }
	return true;
}

void Defs::order(Node* immediateChild, NOrder::Order ord)
{
	switch (ord) {
		case NOrder::TOP:  {
			for(std::vector<suite_ptr>::iterator i = suiteVec_.begin(); i != suiteVec_.end(); ++i) {
				suite_ptr s = (*i);
				if (s.get() == immediateChild) {
					suiteVec_.erase(i);
					suiteVec_.insert(suiteVec_.begin(),s);
					client_suite_mgr_.update_suite_order();
					order_state_change_no_ = Ecf::incr_state_change_no();
					return;
 				}
			}
         throw std::runtime_error("Defs::order: TOP, immediate child suite not found");
		}
		case NOrder::BOTTOM:  {
			for(std::vector<suite_ptr>::iterator i = suiteVec_.begin(); i != suiteVec_.end(); ++i) {
				suite_ptr s = (*i);
				if (s.get() == immediateChild) {
					suiteVec_.erase(i);
					suiteVec_.push_back(s);
               order_state_change_no_ = Ecf::incr_state_change_no();
               client_suite_mgr_.update_suite_order();
					return;
 				}
			}
         throw std::runtime_error("Defs::order: BOTTOM, immediate child suite not found");
		}
		case NOrder::ALPHA:  {
 			std::sort(suiteVec_.begin(),suiteVec_.end(),
			            boost::bind(Str::caseInsLess,
			                          boost::bind(&Node::name,_1),
			                          boost::bind(&Node::name,_2)));
         order_state_change_no_ = Ecf::incr_state_change_no();
         client_suite_mgr_.update_suite_order();
			break;
		}
		case NOrder::ORDER:  {
			std::sort(suiteVec_.begin(),suiteVec_.end(),
			            boost::bind(Str::caseInsGreater,
			                          boost::bind(&Node::name,_1),
			                          boost::bind(&Node::name,_2)));
         order_state_change_no_ = Ecf::incr_state_change_no();
         client_suite_mgr_.update_suite_order();
			break;
		}
		case NOrder::UP:  {
		   for(size_t t = 0; t  < suiteVec_.size();t++) {
		      if ( suiteVec_[t].get() == immediateChild) {
		         if (t != 0) {
		            suite_ptr s =  suiteVec_[t];
		            suiteVec_.erase(suiteVec_.begin()+t);
		            t--;
		            suiteVec_.insert(suiteVec_.begin()+t,s);
		            order_state_change_no_ = Ecf::incr_state_change_no();
		         }
		         client_suite_mgr_.update_suite_order();
		         return;
		      }
		   }
		   throw std::runtime_error("Defs::order: UP, immediate child suite not found");
		}
		case NOrder::DOWN: {
		   for(size_t t = 0; t  < suiteVec_.size();t++) {
		      if ( suiteVec_[t].get() == immediateChild) {
		         if (t != suiteVec_.size()-1) {
		            suite_ptr s =  suiteVec_[t];
		            suiteVec_.erase(suiteVec_.begin()+t);
		            t++;
		            suiteVec_.insert(suiteVec_.begin()+t,s);
		            order_state_change_no_ = Ecf::incr_state_change_no();
		         }
               client_suite_mgr_.update_suite_order();
		         return;
		      }
		   }
         throw std::runtime_error("Defs::order: DOWN, immediate child suite not found");
		}
	}
}

void Defs::top_down_why(std::vector<std::string>& theReasonWhy) const
{
   why(theReasonWhy);
	size_t theSuiteVecSize = suiteVec_.size();
	for(size_t s = 0; s < theSuiteVecSize; s++) { suiteVec_[s]->top_down_why(theReasonWhy);}
}

void Defs::why(std::vector<std::string>& theReasonWhy) const
{
   if (isSuspended()) {
      std::string the_reason = "The server is *not* RUNNING.";
      theReasonWhy.push_back(the_reason);
   }
   else if (state() != NState::QUEUED && state() != NState::ABORTED) {
      std::stringstream ss;
      ss << "The definition state(" << NState::toString(state()) << ") is not queued or aborted.";
      theReasonWhy.push_back(ss.str());
   }
   server_.why(theReasonWhy);
}

std::string Defs::toString() const
{
   // Let the Client control the print style
	std::stringstream ss;
	ss << this;
	return ss.str();
}

// Memento functions
void Defs::collateChanges(unsigned int client_handle, DefsDelta& incremental_changes) const
{
   // Collate any small scale changes to the defs
   collate_defs_changes_only(incremental_changes);

   if (0 == client_handle) {
      // small scale changes. Collate changes over all suites.
      // Suite stores the maximum state change, over *all* its children, this is used by client handle mechanism
      // and here to avoid traversing down the hierarchy.
      // ******** We must trap all child changes under the suite. See class SuiteChanged
      // ******** otherwise some attribute sync's will be missed
      size_t theSuiteVecSize = suiteVec_.size();
      for(size_t s = 0; s <  theSuiteVecSize; s++) {
         if (suiteVec_[s]->state_change_no() > incremental_changes.client_state_change_no()  ) {
            //   *IF* node/attribute change no > client_state_change_no
            //   *THEN*
            //       Create a memento, and store in incremental_changes_
            suiteVec_[s]->collateChanges(incremental_changes);
         }
      }
   }
   else {

      // small scale changes over the suites in our handle, determine what's changed,
      // relative to each node and attributes client_state_change_no.
      //   *IF* node/attribute change no > client_state_change_no
      //   *THEN*
      //       Create a memento, and store in incremental_changes_
      client_suite_mgr_.collateChanges(client_handle,incremental_changes);
   }
}

void Defs::collate_defs_changes_only(DefsDelta& incremental_changes) const
{
   // ************************************************************************************************
   // determine if defs state changed. make sure this is in sync with defs_only_max_state_change_no()
   // ************************************************************************************************
   compound_memento_ptr comp;
   if (state_.state_change_no() > incremental_changes.client_state_change_no()) {
      if (!comp.get()) comp = boost::make_shared<CompoundMemento>(Str::ROOT_PATH());
      comp->add( boost::make_shared<StateMemento>( state_.state()) );
   }
   if (order_state_change_no_ > incremental_changes.client_state_change_no()) {
       if (!comp.get()) comp = boost::make_shared<CompoundMemento>(Str::ROOT_PATH());
       std::vector<std::string> order; order.reserve(suiteVec_.size());
       for(size_t i =0; i < suiteVec_.size(); i++)  order.push_back( suiteVec_[i]->name());
       comp->add( boost::make_shared<OrderMemento>( order ) );
   }

   // Determine if the flag changed
   if (flag_.state_change_no() > incremental_changes.client_state_change_no()) {
      if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(Str::ROOT_PATH());
      comp->add( boost::make_shared<FlagMemento>( flag_ ) );
   }

   // determine if defs server state, currently only watch server state. i.e HALTED, SHUTDOWN, RUNNING
   if (server_.state_change_no()  > incremental_changes.client_state_change_no()) {
      if (!comp.get()) comp = boost::make_shared<CompoundMemento>(Str::ROOT_PATH());
      comp->add( boost::make_shared<ServerStateMemento>( server_.get_state() ) );
   }
   if (server_.variable_state_change_no()  > incremental_changes.client_state_change_no()) {
      if (!comp.get()) comp = boost::make_shared<CompoundMemento>(Str::ROOT_PATH());
      comp->add( boost::make_shared<ServerVariableMemento>( server_.user_variables() ) );
   }

   if (comp.get() ) {
      incremental_changes.add( comp );
   }
}

unsigned int Defs::defs_only_max_state_change_no() const
{
   // ************************************************************************************************
   // make sure this is in sync with collate_defs_changes_only()
   // ************************************************************************************************
   unsigned int max_change_no = 0;
   max_change_no = std::max( max_change_no, state_.state_change_no());
   max_change_no = std::max( max_change_no, order_state_change_no_);
   max_change_no = std::max( max_change_no, flag_.state_change_no());
   max_change_no = std::max( max_change_no, server_.state_change_no());
   max_change_no = std::max( max_change_no, server_.variable_state_change_no());
   return max_change_no;
}

void Defs::set_memento(const StateMemento* memento) {

#ifdef DEBUG_MEMENTO
	std::cout << "Defs::set_memento(const StateMemento* memento)\n";
#endif
   ChangeMgrSingleton::instance()->add_aspect(ecf::Aspect::STATE);
	set_state( memento->state_ );
}

void Defs::set_memento( const ServerStateMemento* memento ) {
#ifdef DEBUG_MEMENTO
	std::cout << "Defs::set_memento(const ServerStateMemento* memento)\n";
#endif
   ChangeMgrSingleton::instance()->add_aspect(ecf::Aspect::SERVER_STATE);
	server_.set_state( memento->state_ );
}

void Defs::set_memento( const ServerVariableMemento* memento ) {
#ifdef DEBUG_MEMENTO
   std::cout << "Defs::set_memento(const ServerVariableMemento* memento)\n";
#endif

   if (server_.user_variables().size() != memento->serverEnv_.size()) {
      ChangeMgrSingleton::instance()->add_aspect(ecf::Aspect::ADD_REMOVE_ATTR);
   }

   ChangeMgrSingleton::instance()->add_aspect(ecf::Aspect::SERVER_VARIABLE);

   server_.set_user_variables( memento->serverEnv_);
}

void Defs::set_memento( const OrderMemento* memento ) {
#ifdef DEBUG_MEMENTO
   std::cout << "Defs::set_memento(const OrderMemento* memento)\n";
#endif
   // Order the suites

   // Order nodeVec_ according to memento ordering
   const std::vector<std::string>& order = memento->order_;

   // NOTE: When we have handles only a small subset of the suites, are returned
   //       Whereas order will always contain all the suites.
   //       Hence we need to handle the case where: order.size() != suiteVec_.size()

   std::vector<suite_ptr> vec; vec.reserve(suiteVec_.size());
   size_t node_vec_size = suiteVec_.size();
   for(size_t i = 0; i < order.size(); i++) {
      for(size_t t = 0; t < node_vec_size; t++) {
          if (order[i] == suiteVec_[t]->name()) {
             vec.push_back(suiteVec_[t]);
             break;
          }
       }
   }
   if (vec.size() !=  suiteVec_.size()) {
       std::cout << "Defs::set_memento could not find all the names\n";
       return;
   }
   ChangeMgrSingleton::instance()->add_aspect(ecf::Aspect::ORDER);
   suiteVec_ = vec;
}


void Defs::set_memento( const FlagMemento* memento ) {

#ifdef DEBUG_MEMENTO
   std::cout << "Defs::set_memento(const FlagMemento* memento)\n";
#endif
   ChangeMgrSingleton::instance()->add_aspect(ecf::Aspect::FLAG);
   flag_.set_flag( memento->flag_.flag() );
}

// =====================================================================

void Defs::add_edit_history(const std::string& path, const std::string& request)
{
   std::map<std::string, std::deque<std::string> >::iterator i = edit_history_.find(path);
   if (i == edit_history_.end()) {
      std::deque<std::string> vec; vec.push_back(request);
      edit_history_.insert( std::make_pair(path,vec) );
   }
   else {
      (*i).second.push_back(request);
      if ((*i).second.size() > 20) {
         (*i).second.pop_front();
      }
   }
}

const std::deque<std::string>& Defs::get_edit_history(const std::string& path) const
{
   std::map<std::string, std::deque<std::string> >::const_iterator i = edit_history_.find(path);
   if (i != edit_history_.end()) {
      return (*i).second;
   }
   return empty_edit_history();
}

const std::deque<std::string>& Defs::empty_edit_history()
{
   static std::deque<std::string> static_edit_history;
   return static_edit_history;
}

// =====================================================================================

std::ostream& operator<<(std::ostream& os, const Defs* d)
{
   if (d) return d->print(os);
   return os << "DEFS == NULL\n";
}
std::ostream& operator<<(std::ostream& os, const Defs& d)  { return d.print(os); }

// =========================================================================

DefsHistoryParser::DefsHistoryParser() {
   Log::get_log_types(log_types_);
}

void DefsHistoryParser::parse(const std::string& line)
{
   size_t pos = line.find("\b");
   if (pos != std::string::npos) {
      // keep compatibility with current way of writing history
      std::string requests = line.substr(pos);
      Str::split(requests,parsed_messages_,"\b");
      return;
   }

   // fallback, split line based on looking for logType like 'MSG:[' | 'LOG:['
   string::size_type first = find_log(line,0);
   if (first == std::string::npos) return;

   string::size_type next = find_log(line,first + 4);
   if (next == std::string::npos ) {
      parsed_messages_.push_back( line.substr( first ) );
      return;
   }

   while (next != std::string::npos) {
      parsed_messages_.push_back( line.substr( first, next - first ) );
      first = next;
      next = find_log(line,first + 4);

      if (next == std::string::npos ) {
         parsed_messages_.push_back( line.substr( first ) );
         return;
      }
   }
}

string::size_type DefsHistoryParser::find_log(const std::string& line, string::size_type pos) const
{
   for(size_t i = 0; i < log_types_.size(); i++) {
      std::string log_type = log_types_[i];
      log_type += ":[";
      string::size_type log_type_pos = line.find( log_type, pos );
      if (log_type_pos != std::string::npos) {
         return log_type_pos;
      }
   }
   return std::string::npos;
}
