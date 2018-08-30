//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #204 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <cassert>
#include <sstream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/exception.hpp"
#include <boost/lexical_cast.hpp>

#include "Task.hpp"
#include "Defs.hpp"
#include "PrintStyle.hpp"
#include "Suite.hpp"
#include "SuiteChanged.hpp"
#include "NodeTreeVisitor.hpp"
#include "File.hpp"
#include "Stl.hpp"
#include "Str.hpp"
#include "Indentor.hpp"
#include "Log.hpp"
#include "ExprAst.hpp"
#include "JobsParam.hpp"
#include "Ecf.hpp"
#include "DefsDelta.hpp"
#include "TaskScriptGenerator.hpp"
#include "Extract.hpp"
#include "JobProfiler.hpp"

namespace fs = boost::filesystem;
using namespace ecf;
using namespace std;
using namespace boost;

//#define DEBUG_TASK_LOCATION 1
void Task::copy(const Task& rhs)
{
   size_t theSize = rhs.aliases_.size();
   for(size_t s = 0; s < theSize; s++) {
      alias_ptr alias_copy = std::make_shared<Alias>( *rhs.aliases_[s] );
      alias_copy->set_parent(this);
      aliases_.push_back( alias_copy );
   }
}

Task::Task(const Task& rhs)
: Submittable(rhs),
  order_state_change_no_(0),
  add_remove_state_change_no_(0),
  alias_change_no_(0),
  alias_no_(rhs.alias_no_)
{
   copy(rhs);
}

node_ptr Task::clone() const
{
   return std::make_shared<Task>( *this );
}

bool Task::check_defaults() const
{
   if (order_state_change_no_ != 0) throw std::runtime_error("Task::check_defaults(): order_state_change_no_ != 0");
   if (add_remove_state_change_no_ != 0) throw std::runtime_error("Task::check_defaults(): add_remove_state_change_no_ != 0");
   if (alias_change_no_ != 0) throw std::runtime_error("Task::check_defaults(): alias_change_no_ != 0");
   if (alias_no_ != 0) throw std::runtime_error("Task::check_defaults(): alias_no_ != 0");
   return Submittable::check_defaults();
}

Task& Task::operator=(const Task& rhs)
{
   if (this != &rhs) {
      Submittable::operator=(rhs);
      aliases_.clear();
      alias_no_ = rhs.alias_no_;
      copy(rhs);

      order_state_change_no_ = 0;
      alias_change_no_ = 0;
      add_remove_state_change_no_ = Ecf::incr_state_change_no();
   }
   return *this;
}

Task::~Task()
{
   if (!Ecf::server()) {
       notify_delete();
   }
}

task_ptr Task::create(const std::string& name)
{
	return std::make_shared<Task>( name );
}

std::ostream& Task::print(std::ostream& os) const
{
   Indentor in;
   Indentor::indent(os) << "task " << name();
   if (!PrintStyle::defsStyle()) {
      std::string st = write_state();
      if (!st.empty()) os << " #" << st;
   }
   os << "\n";

   Node::print(os);

   // Generated variable are not persisted since they are created on demand
   // There *NO* point in printing them they will always be empty

   // Alias are not printed, but are check point able.
   if (!PrintStyle::defsStyle()) {
      Indentor in2;
      size_t node_vec_size = aliases_.size();
      for(size_t t = 0; t < node_vec_size; t++) { aliases_[t]->print( os ); }
      if (node_vec_size != 0) {
         Indentor in3;
         Indentor::indent(os) << "endalias\n";
      }
   }

   // if ( PrintStyle::defsStyle() ) Indentor::indent(os) << "endtask\n";
   return os;
}

std::string Task::write_state() const
{
   // *IMPORTANT* we *CANT* use ';' character, since is used in the parser, when we have
   //             multiple statement on a single line i.e.
   //                 task a; task b;
   std::string ret;
   if (alias_no_ != 0) { ret += " alias_no:"; ret += boost::lexical_cast<std::string>(alias_no_);}
   ret += Submittable::write_state();
   return ret;
}

void Task::read_state(const std::string& line, const std::vector<std::string>& lineTokens) {

   // task t1 # alias_no:0 passwd:_DJP_
   std::string token;
   for(size_t i = 3; i < lineTokens.size(); i++) {
      token.clear();
      if (lineTokens[i].find("alias_no:") != std::string::npos ) {
         if (!Extract::split_get_second(lineTokens[i],token)) throw std::runtime_error( "Task::read_state could not read alias_no for task " + name());
         alias_no_ = Extract::theInt(token,"Task::read_state: invalid alias_no specified : " + line);
         break;
      }
   }
   Submittable::read_state(line,lineTokens);
}

std::ostream& operator<<(std::ostream& os, const Task& d)  { return d.print(os); }

bool Task::operator==(const Task& rhs) const
{
   if (alias_no_ != rhs.alias_no_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Task::operator==  alias_no_(" << alias_no_ << ")  != rhs.alias_no_(" << rhs.alias_no_ << ") : " << absNodePath() << "\n";
      }
#endif
      return false;
   }

   size_t vec_size = aliases_.size();
   if ( vec_size != rhs.aliases_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Task::operator==  aliases_.size() != rhs.aliases_.size() " << absNodePath() << "\n";
         std::cout << "   aliases_.size() = " << vec_size << "  rhs.aliases_.size() = " << rhs.aliases_.size() << "\n";
      }
#endif
      return false;
   }

   for(size_t i =0; i < vec_size; ++i) {

      if ( !( *aliases_[i] == *rhs.aliases_[i] )) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "Task::operator==  !( *aliases_[i] == *rhs.aliases_[i] : " << absNodePath() << "\n";
         }
#endif
         return false;
      }
   }

   return Submittable::operator==(rhs);
}

alias_ptr Task::add_alias(std::vector<std::string>& user_file_contents,const NameValueVec& user_variables,bool create_directory)
{
   // Create directory
   std::string dir_to_create;
   if (create_directory) {

      if (user_file_contents.empty()) {
         std::stringstream ss;
         ss << "Task::add_alias: No .usr file contents specified. Alias creation failed for task " << absNodePath();
         throw std::runtime_error(ss.str());
      }

      findParentUserVariableValue( Str::ECF_HOME(), dir_to_create);
      dir_to_create += absNodePath();
      if (!File::createDirectories(dir_to_create)) {
         throw std::runtime_error("Task::add_alias: could not create directory " +  dir_to_create);
      }
   }

   // create alias
   std::string alias_name = "alias" + boost::lexical_cast<std::string>(alias_no_);
   alias_ptr alias = Alias::create( alias_name );
   alias->set_parent(this);

   // create .usr file
   if (create_directory) {
      std::string file_path = dir_to_create + "/" + alias_name + alias->script_extension();
      std::string error_msg;
      if (!File::create(file_path,user_file_contents,error_msg)) {
         std::stringstream ss; ss << "Task::add_alias: could not create .usr file at path(" << file_path <<"): " <<  error_msg.c_str();
         throw std::runtime_error(ss.str());
      }
   }

   // copy over events, meters, labels, but clear state (ECFLOW-1278)
   BOOST_FOREACH(Meter meter, meters()) { meter.reset(); alias->addMeter(meter); }
   BOOST_FOREACH(Event event, events()) { event.reset(); alias->addEvent(event); }
   BOOST_FOREACH(Label label, labels()) { label.reset(); alias->addLabel(label); }

   // Add user_variables as variables. Note: to reduce memory we could choose
   // to only add those variable that have been changed/added. However this
   // would mean an alias could be affected by changed to an inherited variable.
   // Hence kept as existing sms functionality
   //
   // The variables may be **different** to normal variables in that they may contain a ":" & $
   // This is **not** allowed in normal variables.
   // i.e it allows for  %A:1%, %A:2%, %A:3%
   // This is not really recommended but its what the old system supported.
   // **** Hence add_alias_variable by passes variable name checking ***
   auto theEnd = user_variables.end();
   for(auto i = user_variables.begin(); i!=theEnd; ++i) {
      alias->add_alias_variable((*i).first, (*i).second);
   }

   // increment alias number and store, alias in vector
   alias_no_++;  // Alias number must be set to next valid alias number
   aliases_.push_back(alias);

   alias_change_no_ = Ecf::incr_state_change_no();
   add_remove_state_change_no_ = alias_change_no_;
   return alias;
}

alias_ptr Task::add_alias_only()
{
   std::vector<std::string> empty_user_file_contents;
   NameValueVec empty_user_variables;
   return add_alias(empty_user_file_contents,empty_user_variables,false/* don't create directory or .usr file*/);
}

alias_ptr Task::add_alias(const std::string& name)
{
   // Do not update alias_no, since that will be read in
   alias_ptr alias = Alias::create( name );
   alias->set_parent(this);
   aliases_.push_back(alias);
   return alias;
}

alias_ptr Task::find_alias(const std::string& name) const
{
   size_t vec_size = aliases_.size();
   for(size_t i = 0; i < vec_size; i++) {
      if (aliases_[i]->name() == name) {
         return aliases_[i];
      }
   }
   return alias_ptr();
}

void Task::reset_alias_number()
{
   alias_no_ = 0;
   alias_change_no_ = Ecf::incr_state_change_no();
}

node_ptr Task::findImmediateChild(const std::string& name, size_t& child_pos) const
{
   child_pos = std::numeric_limits<std::size_t>::max();
   size_t vec_size = aliases_.size();
    for(size_t i = 0; i < vec_size; i++) {
       if (aliases_[i]->name() == name) {
          child_pos = i;
          return aliases_[i];
       }
    }
    return node_ptr();
}

void Task::reset()
{
   if (aliases_.empty()) {
      if (alias_no_ != 0) {
         reset_alias_number();
      }
   }
   Submittable::reset();
}

void Task::begin()
{
   if (aliases_.empty()) {
      if (alias_no_ != 0) {
         reset_alias_number();
      }
   }

	Submittable::begin();

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "Task::begin()\n";
#endif
}

void Task::requeue(Requeue_args& args)
{
   if (aliases_.empty()) {
      if (alias_no_ != 0) {
         reset_alias_number();
      }
   }

	Submittable::requeue(args);

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "Task::requeue\n";
#endif
}

void Task::accept(ecf::NodeTreeVisitor& v)
{
	v.visitTask(this);
}

void Task::acceptVisitTraversor(ecf::NodeTreeVisitor& v)
{
	v.visitTask(this);
}

const std::string& Task::debugType() const { return ecf::Str::TASK();}

void Task::getAllNodes(std::vector<Node*>& vec) const
{
   // See notes: getAllSubmittables, about reserve
   size_t vec_size = aliases_.size();
   for(size_t i = 0; i < vec_size; i++) {
      vec.push_back( aliases_[i].get() );
   }
}

void Task::immediateChildren(std::vector<node_ptr>& vec) const
{
   size_t vec_size = aliases_.size();
   vec.reserve(vec.size() + vec_size);
   for(size_t i = 0; i < vec_size; i++) {
      vec.push_back( std::dynamic_pointer_cast<Node>(aliases_[i]) );
   }
}

void Task::getAllTasks(std::vector<Task*>& vec) const
{
	vec.push_back(const_cast<Task*>(this));
}

void Task::getAllSubmittables(std::vector<Submittable*>& vec) const
{
   // *DO NOT reserve here, as it dominate time , for very large defs */
   // * Previously we had::
   //    vec.reserve(vec.size() + vec_size + 1);
   // * This took 47 seconds when delete the full defs, i.e when check for active tasks

   vec.push_back(const_cast<Task*>(this));
   size_t vec_size = aliases_.size();
   for(size_t i = 0; i < vec_size; i++) {
      vec.push_back( aliases_[i].get() );
   }
}

node_ptr Task::find_node_up_the_tree(const std::string& name) const
{
   size_t vec_size = aliases_.size();
   for(size_t i = 0; i < vec_size; i++) {
      if (aliases_[i]->name() == name ) {
         return aliases_[i];
      }
   }
   Node* the_parent = parent();
   if (the_parent) return the_parent->find_node_up_the_tree(name);
   return node_ptr();
}

void Task::get_all_active_submittables(std::vector<Submittable*>& vec) const
{
   // See notes: getAllSubmittables, about reserve
   if (state() == NState::ACTIVE || state() == NState::SUBMITTED) {
      vec.push_back(const_cast<Task*>(this));
   }
   size_t vec_size = aliases_.size();
   for(size_t i = 0; i < vec_size; i++) {
      if (aliases_[i]->state() == NState::ACTIVE || aliases_[i]->state() == NState::SUBMITTED) {
         vec.push_back( aliases_[i].get() );
      }
   }
}

void Task::get_all_tasks(std::vector<task_ptr>& vec) const
{
   vec.push_back(std::dynamic_pointer_cast<Task>(non_const_this()));
}

void Task::get_all_nodes(std::vector<node_ptr>& nodes) const
{
   nodes.push_back( non_const_this() );
   size_t vec_size = aliases_.size();
   for(size_t i = 0; i < vec_size; i++) {
      aliases_[i]->get_all_nodes(nodes);
   }
}

void Task::get_all_aliases(std::vector<alias_ptr>& destinationVec) const
{
   destinationVec.reserve(destinationVec.size() + aliases_.size());
   std::copy(aliases_.begin(),aliases_.end(),std::back_inserter(destinationVec));
}

bool Task::resolveDependencies(JobsParam& jobsParam)
{
   if (jobsParam.timed_out_of_job_generation()) return false;
   JobProfiler profile_me(this,jobsParam,JobProfiler::task_threshold());
   if (jobsParam.timed_out_of_job_generation()) return false;


   // Calling Submittable::resolveDependencies(jobsParam) up front can be expensive.
   // Due to trigger and complete evaluations. Hence low cost state checks first

 	// Do state checking for tasks only. Note: container nodes inherit the most significant state
 	// from the children, hence we can't use the same same algorithm for containers nodes and leaf
 	// nodes like task.
	NState::State task_state = state();
	if ( task_state == NState::COMPLETE || task_state == NState::ACTIVE || task_state == NState::SUBMITTED || task_state == NState::UNKNOWN ) {
#ifdef DEBUG_DEPENDENCIES
		LOG(Log::DBG,"   Task::resolveDependencies() " << absNodePath() << " HOLDING as task state " << NState::toString(state()) << " is not valid for job submission" );
#endif
		return false;
	}
	else if (task_state == NState::ABORTED) {

	   /// If we have been forcibly aborted by the user. Do not resubmit jobs, until *begin* or *re-queue*. ECFLOW-344
	   if (get_flag().is_set(ecf::Flag::FORCE_ABORT)) {
#ifdef DEBUG_DEPENDENCIES
	      LOG(Log::DBG,"   Task::resolveDependencies() " << absNodePath() << " HOLDING as task state " << NState::toString(state()) << " has been forcibly aborted." );
#endif
	      return false;
	   }

      /// If we have been killed by the user. Do not resubmit jobs, until *begin* or *re-queue*
      if (get_flag().is_set(ecf::Flag::KILLED)) {
#ifdef DEBUG_DEPENDENCIES
         LOG(Log::DBG,"   Task::resolveDependencies() " << absNodePath() << " HOLDING as task state " << NState::toString(state()) << " has been killed." );
#endif
         return false;
      }

      // Job cmd failed. Do not resubmit jobs, until *begin* or *re-queue*. ECFLOW-1216
      if (get_flag().is_set(ecf::Flag::EDIT_FAILED)) {
         return false; // pre-processing, variable subs, create directory, change job file permission failed
      }
      if (get_flag().is_set(ecf::Flag::NO_SCRIPT)) {
         return false; // .ecf file location failed
      }
      if (get_flag().is_set(ecf::Flag::JOBCMD_FAILED)) {
         return false; // variable substituion on JOB cmd failed
      }


      // If the task was aborted, and we have not exceeded ECF_TRIES, then resubmit
      // otherwise ONLY in state QUEUED can we submit jobs
      std::string varValue;
      if (findParentUserVariableValue( Str::ECF_TRIES(), varValue ))  {
         // std::cout << "tryNo_ = " << tryNo_ << " ECF_TRIES = " <<  varValue << "\n";
         try {
            auto ecf_tries = boost::lexical_cast< int > (varValue);
            if ( try_no() >= ecf_tries ) {
#ifdef DEBUG_DEPENDENCIES
               LOG(Log::DBG,"   Task::resolveDependencies() " << absNodePath() << " HOLDING as tryNo_(" << tryNo_ ") >= ECF_TRIES(" << ecf_tries << ") state = " << NState::toString(state()));
#endif
               return false;
            }
         }
         catch ( boost::bad_lexical_cast& ) {
            LOG(Log::ERR,"Variable ECF_TRIES must be convertible to an integer. Can not resubmit job for task:" << absNodePath());
            return false;
         }
      }
   }
#ifdef DEBUG
	else {
	   /// Only one state left
	   assert(task_state == NState::QUEUED);
	}
#endif

   /// If we have been forcibly aborted by the user. Do not resubmit jobs, until *begin* or *re-queue*
	/// This can be set via ALTER, so independent of state.
   if (get_flag().is_set(ecf::Flag::FORCE_ABORT)) {
#ifdef DEBUG_DEPENDENCIES
      LOG(Log::DBG,"   Task::resolveDependencies() " << absNodePath() << " HOLDING as task state " << NState::toString(state()) << " has been forcibly aborted." );
#endif
      return false;
   }


	if ( ! Node::resolveDependencies(jobsParam) ) {

#ifdef DEBUG_JOB_SUBMISSION
		LOG(Log::DBG, "   Task::resolveDependencies " << absNodePath() << " could not resolve dependencies, may have completed");
		cout << "Task::resolveDependencies " << absNodePath() << " could not resolve dependencies may have completed" << endl;
#endif
		return false;
	}

	/// By default node tree traversal is top down. hence we only check in limits, at *that* level.
	/// However *EACH* job submission can *affect* the in limits, hence we *must* check we are in
	/// limit *up* the node tree. Done last and only in this function (as opposed to Node) as an optimisation
	if (!check_in_limit_up_node_tree()) {
#ifdef DEBUG_DEPENDENCIES
		LOG(Log::DBG,"   Task::resolveDependencies() " << absNodePath() << " FREE of TRIGGER and inLIMIT");
#endif
		return false;
	}

   // call just before job submission, reset data members, update try_no, and generate variable
	// *PLACED* outside of submitJob() so that we can configure job generation file ECF_JOB for test/python
   increment_try_no(); // will increment state_change_no

	if ( jobsParam.createJobs() ) {
		// The task are ready for job submission.Clear process id and remote id (ECF_RID)
		// Locate the ecf files corresponding to the task. Pre-process
		// them(i.e expand includes, remove comments,manual) and perform
		// variable substitution. This will then form the jobs file.
		// If the job file already exist it is overridden
		submit_job_only( jobsParam );
	}
	else {
		// *************************************************************************************
		// Debug/test path only... Enabled for testing when we don't want to create/spawn jobs
		// ** Simulate ** job submission as closely as possible. For testing
		// *************************************************************************************
		jobsParam.push_back_submittable( this );

		// follow normal life cycle queued->submitted->active. In real life there may be a noticeable
		// time delay between process creation (via a user command, which could do anything)
		// and when created process start talking back to the server.
		// *** Setting state to SUBMITTED will increment any inlimit/Limit via handleStateChange
 		set_state( NState::SUBMITTED );

		// The spawned process will typically call this, via client api. Set task into active state
 		// *** Test path, we take the hit of calling handleStateChange again.
		init(Submittable::DUMMY_PROCESS_OR_REMOTE_ID());
	}
	return true;
}

void Task::generate_scripts( const std::map<std::string,std::string>& override) const
{
   TaskScriptGenerator ecf(this);
   ecf.generate(override);
}

node_ptr Task::removeChild(Node* child)
{
#ifdef DEBUG
   assert(child);
   assert(child->isAlias());
#endif
   SuiteChanged1 changed(suite());
   size_t node_vec_size = aliases_.size();
   for(size_t t = 0; t < node_vec_size; t++)     {
      if (aliases_[t].get() == child) {
         child->set_parent(nullptr);
         node_ptr node = std::dynamic_pointer_cast<Alias>(aliases_[t]);
         aliases_.erase( aliases_.begin() + t);
         add_remove_state_change_no_ = Ecf::incr_state_change_no();
         return node ;
      }
   }
   // Should never happen
   LOG_ASSERT(false,"Task::removeChild: Could not remove child");
   return node_ptr();
}

bool Task::doDeleteChild(Node* child)
{
   SuiteChanged1 changed(suite());
   auto the_end = aliases_.end();
   for(auto t = aliases_.begin(); t!=the_end; ++t) {
      if ( (*t).get() == child) {
         if (child && child->parent()) child->set_parent(nullptr);
         aliases_.erase(t);
         add_remove_state_change_no_ = Ecf::incr_state_change_no();
         return true;
      }
   }
   return false;
}

bool Task::addChild( node_ptr, size_t)
{
   // Only used during PLUG: aliases can't be plugged.
	LOG_ASSERT(false,"");
	return false;
}

bool Task::isAddChildOk( Node*, std::string& errorMsg) const
{
   // Only used during PLUG: aliases can't be plugged.
	errorMsg += "Can not add children to a task node.";
	return false;
}

size_t Task::child_position(const Node* child) const
{
   size_t vec_size = aliases_.size();
   for(size_t t = 0; t < vec_size; t++) {
      if (aliases_[t].get() == child) {
         return t;
      }
   }
   return std::numeric_limits<std::size_t>::max();
}

void Task::order(Node* immediateChild, NOrder::Order ord)
{
   SuiteChanged1 changed(suite());
   switch (ord) {
      case NOrder::TOP:  {
         for(auto i = aliases_.begin(); i != aliases_.end(); ++i) {
            if ((*i).get() == immediateChild) {
               alias_ptr node = (*i);
               aliases_.erase(i);
               aliases_.insert(aliases_.begin(),node);
               order_state_change_no_ = Ecf::incr_state_change_no();
               return;
            }
         }
         throw std::runtime_error("Task::order TOP, immediate child not found");
      }
      case NOrder::BOTTOM:  {
         for(auto i = aliases_.begin(); i != aliases_.end(); ++i) {
            if ((*i).get() == immediateChild) {
               alias_ptr node = (*i);
               aliases_.erase(i);
               aliases_.push_back(node);
               order_state_change_no_ = Ecf::incr_state_change_no();
               return;
            }
         }
         throw std::runtime_error("Task::order BOTTOM, immediate child not found");
      }
      case NOrder::ALPHA:  {
         std::sort(aliases_.begin(),aliases_.end(),
                   [](const alias_ptr& a,const alias_ptr& b) {return Str::caseInsLess(a->name(),b->name());});
         order_state_change_no_ = Ecf::incr_state_change_no();
         break;
      }
      case NOrder::ORDER:  {
         std::sort(aliases_.begin(),aliases_.end(),
                   [](const alias_ptr& a,const alias_ptr& b) {return Str::caseInsGreater(a->name(),b->name());});
         order_state_change_no_ = Ecf::incr_state_change_no();
         break;
      }
      case NOrder::UP:  {
         for(size_t t = 0; t  < aliases_.size();t++) {
            if ( aliases_[t].get() == immediateChild) {
               if (t != 0) {
                  alias_ptr node =  aliases_[t];
                  aliases_.erase(aliases_.begin()+t);
                  t--;
                  aliases_.insert(aliases_.begin()+t,node);
                  order_state_change_no_ = Ecf::incr_state_change_no();
                }
               return;
            }
         }
         throw std::runtime_error("Task::order UP, immediate child not found");
      }
      case NOrder::DOWN: {
         for(size_t t = 0; t  < aliases_.size();t++) {
            if ( aliases_[t].get() == immediateChild) {
               if (t != aliases_.size()-1) {
                  alias_ptr node =  aliases_[t];
                  aliases_.erase(aliases_.begin()+t);
                  t++;
                  aliases_.insert(aliases_.begin()+t,node);
                  order_state_change_no_ = Ecf::incr_state_change_no();
               }
               return;
            }
         }
         throw std::runtime_error("Task::order DOWN, immediate child not found");
      }
   }
}

bool Task::checkInvariants(std::string& errorMsg) const
{
   if (!Node::checkInvariants(errorMsg)) return false;

   size_t vec_size = aliases_.size();
   for(size_t t = 0; t < vec_size; t++) {
      if (aliases_[t]->parent() != this) {
         std::stringstream ss;
         ss << "Task::checkInvariants alias(" << aliases_[t]->name() << ") parent() not correct. See task : " << absNodePath();
         errorMsg += ss.str();
         return false;
      }
      if (!aliases_[t]->checkInvariants(errorMsg)) {
         return false;
      }
   }
//   if ( vec_size > alias_no_ ) {
//      std::stringstream ss;
//      ss << "Task::checkInvariants: alias vector size " << vec_size << " should be less or equal to alias_no_ " << alias_no_ << " for task " << absNodePath() << "\n";
//      errorMsg += ss.str();
//      return false;
//   }
   return true;
}

void Task::handleStateChange()
{
   /// Increment/decrement limits based on the current state
   update_limits();

	// Check if a re queue is required, then can eventually change the state, if
	// repeats, time,today, or cron are involved, hence must be done last
	// This will recurse up the node tree, causing repeats to increment, at the parent
	// level and resetting repeats in the children. To mimic nested loops.
 	requeueOrSetMostSignificantStateUpNodeTree();

   Node::handleStateChange(); // may do a autorestore, if state is COMPLETE
}

const std::string& Task::script_extension() const
{
   // Migration support, allow user to specify extension. This allows users to use '.sms'
   // Note: This should be removed in the future since there is performance hit.
   //       searching up the node tree, when most of the time we are using .ecf
   const std::string& ecf_extn = find_parent_user_variable_value(Str::ECF_EXTN());
   if (!ecf_extn.empty()) return ecf_extn;
   return File::ECF_EXTN(); // ".ecf"
}

void Task::collateChanges(DefsDelta& changes) const
{
//   std::cout << "Task::collateChanges " << debugNodePath()
//             << " changes.client_state_change_no() = " << changes.client_state_change_no()
//             << " add_remove_state_change_no_ = " << add_remove_state_change_no_
//             << " order_state_change_no_ = " << order_state_change_no_
//             << " alias_change_no_ " << alias_change_no_
//             << "\n";

   /// All changes to Task should be on ONE compound_memento_ptr
   compound_memento_ptr comp;

   /// There no point doing a OrderMemento if children have been added/delete
   if (add_remove_state_change_no_ > changes.client_state_change_no()) {
      if (!comp.get()) comp = std::make_shared<CompoundMemento>(absNodePath());
      comp->add( std::make_shared<AliasChildrenMemento>( aliases_ ) );
   }
   else if (order_state_change_no_ > changes.client_state_change_no()) {
      if (!comp.get()) comp = std::make_shared<CompoundMemento>(absNodePath());
      std::vector<std::string> order_vec; order_vec.reserve(aliases_.size());
      size_t node_vec_size = aliases_.size();
      for(size_t i =0; i < node_vec_size; i++)  order_vec.push_back( aliases_[i]->name());
      comp->add( std::make_shared<OrderMemento>( order_vec ) );
   }

   if (alias_change_no_ > changes.client_state_change_no()) {
      if (!comp.get()) comp = std::make_shared<CompoundMemento>(absNodePath());
      comp->add( std::make_shared<AliasNumberMemento>( alias_no_ ) );
   }

   // ** base class will add compound memento into changes.
   Submittable::incremental_changes(changes, comp);

   // Traversal to children
   size_t vec_size = aliases_.size();
   for(size_t t = 0; t < vec_size; t++)   { aliases_[t]->collateChanges(changes); }
}

void Task::set_memento( const OrderMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only) {
#ifdef DEBUG_MEMENTO
   std::cout << "Task::set_memento( const OrderMemento* ) " << debugNodePath() << "\n";
#endif
   if (aspect_only) {
      aspects.push_back(ecf::Aspect::ORDER);
      return;
   }
   
   // Order aliases_ according to memento ordering
   const std::vector<std::string>& order = memento->order_;
   if (order.size() != aliases_.size()) {
      // something gone wrong.
      std::cout << "Task::set_memento OrderMemento, memento.size() " << order.size() << " Not the same as aliases_size() " << aliases_.size() << "\n";
      return;
   }

   std::vector<alias_ptr> vec; vec.reserve(aliases_.size());
   size_t node_vec_size = aliases_.size();
   for(const auto & i : order) {
      for(size_t t = 0; t < node_vec_size; t++) {
          if (i == aliases_[t]->name()) {
             vec.push_back(aliases_[t]);
             break;
          }
       }
   }
   if (vec.size() !=  aliases_.size()) {
       std::cout << "Task::set_memento(const OrderMemento* memento) could not find all the names\n";
       return;
   }

   aliases_ = vec;
}

void Task::set_memento( const AliasChildrenMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only) {
#ifdef DEBUG_MEMENTO
   std::cout << "Task::set_memento( const AliasChildrenMemento* ) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      aspects.push_back(ecf::Aspect::ADD_REMOVE_NODE);
      return;
   }

   // set up alias parent pointers. since they are *NOT* serialised.
   aliases_ = memento->children_;
   size_t vec_size = aliases_.size();
   for(size_t i = 0; i < vec_size; i++) {
      aliases_[i]->set_parent(this);
   }
}

void Task::set_memento( const AliasNumberMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only) {
#ifdef DEBUG_MEMENTO
   std::cout << "Task::set_memento( const AliasNumberMemento* ) " << debugNodePath() << "\n";
#endif
   if (aspect_only) {
      aspects.push_back(ecf::Aspect::ALIAS_NUMBER);
      return;
   }

   alias_no_ = memento->alias_no_;
}

CEREAL_REGISTER_TYPE(Task);

