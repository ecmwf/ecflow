/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #305 $ 
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
#include <assert.h>
#include <deque>

#include <boost/bind.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Task.hpp"
#include "Extract.hpp"

#include "NodeState.hpp"
#include "NodePath.hpp"
#include "Stl.hpp"
#include "Str.hpp"
#include "Indentor.hpp"
#include "ExprAst.hpp"
#include "Log.hpp"
#include "PrintStyle.hpp"
#include "JobsParam.hpp"
#include "ExprAstVisitor.hpp"
#include "Ecf.hpp"
#include "SuiteChanged.hpp"
#include "CmdContext.hpp"

using namespace ecf;
using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

///////////////////////////////////////////////////////////////////////////////////////////
//#define DEBUG_DEPENDENCIES 1
//#define DEBUG_REQUEUE 1
//#define DEBUG_FIND_REFERENCED_NODE 1

Node::Node(const std::string& n)
: parent_(NULL),name_(n),
  suspended_(false),
  state_( std::make_pair(NState(),time_duration(0,0,0,0)) ),
  completeExpr_(NULL),
  triggerExpr_(NULL),
  lateAttr_(NULL),
  autoCancel_(NULL),
  time_dep_attrs_(NULL),
  child_attrs_(NULL),
  misc_attrs_(NULL),
  inLimitMgr_(this),
  state_change_no_(0),variable_change_no_(0),suspended_change_no_(0),
  graphic_ptr_(0)
{
   string msg;
   if (!Str::valid_name(n, msg)) {
      throw std::runtime_error("Invalid node name : " + msg);
   }
}

Node::Node()
: parent_(NULL),
  suspended_(false),
  state_( std::make_pair(NState(),time_duration(0,0,0,0)) ),
  completeExpr_(NULL),
  triggerExpr_(NULL),
  lateAttr_(NULL),
  autoCancel_(NULL),
  time_dep_attrs_(NULL),
  child_attrs_(NULL),
  misc_attrs_(NULL),
  inLimitMgr_(this),
  state_change_no_(0),variable_change_no_(0),suspended_change_no_(0),
  graphic_ptr_(0)
{}

Node::~Node() {
   delete completeExpr_;
   delete triggerExpr_;
   delete lateAttr_;
   delete autoCancel_;
   delete time_dep_attrs_;
   delete child_attrs_;
   delete misc_attrs_;
}

bool Node::isParentSuspended() const
{
   Node* theParent = parent();
   if (theParent) {
      if (theParent->isSuspended()) return true;
      return theParent->isParentSuspended();
   }
   return defs()->isSuspended(); // obtained from the server states
}

void Node::resume()
{
   if ( suspended_) {
      clearSuspended();
   }
}

void Node::clearSuspended()
{
   /// Guard against unnecessary creation of memento's
   if (suspended_) {
      suspended_ = false;
      suspended_change_no_ = Ecf::incr_state_change_no();
   }
}

void Node::suspend()
{
   // Typically called via user action or via defstatus
   suspended_ = true;
   suspended_change_no_ = Ecf::incr_state_change_no();
}

void Node::begin()
{
   // Set the state without causing any side effects
   initState(0);

   clearTrigger();
   clearComplete();

   flag_.reset();
   repeat_.reset();         // if repeat is empty reset() does nothing

   if (lateAttr_) lateAttr_->reset();
   if (child_attrs_) child_attrs_->begin();
   if (misc_attrs_) misc_attrs_->begin();
   for(size_t i = 0; i < limitVec_.size(); i++)   { limitVec_[i]->reset(); }

   // Let time base attributes use, relative duration if applicable
   if (time_dep_attrs_) {
      time_dep_attrs_->begin();
      time_dep_attrs_->markHybridTimeDependentsAsComplete();
   }

   // DO *NOT* call update_generated_variables(). Called on a type specific bases, for begin
   // Typically we need only call update_generated_variables() for a task, at job creation time.
   // so that ECF_OUT, ECF_TRYNO, ECF_JOBOUT, ECF_PASS(jobsPassword_) can be updated.
   // However the generated variables are used when within job generation and can be referenced by AST
   // Hence to avoid excessive memory consumption, they are created on demand
}

void Node::requeue(
         bool resetRepeats,
         int clear_suspended_in_child_nodes,
         bool do_reset_next_time_slot)
{
#ifdef DEBUG_REQUEUE
   LOG(Log::DBG,"      Node::requeue() " << absNodePath() << " resetRepeats = " << resetRepeats);
#endif
   /// Note: we don't reset verify attributes as they record state stat's

   // Set the state without causing any side effects
   initState(clear_suspended_in_child_nodes);

   clearTrigger();
   clearComplete();

   if (resetRepeats) repeat_.reset(); // if repeat is empty reset() does nothing


   /// If a job takes longer than it slots, then that slot is missed, and next slot is used
   /// Note we do *NOT* reset for requeue as we want to advance the valid time slots.
   /// *NOTE* Update calendar will *free* time dependencies *even* time series. They rely
   /// on this function to clear the time dependencies so they *HOLD* the task.
   if ( time_dep_attrs_ ) {

      /// Requeue has several contexts:
      ///   1/ manual requeue
      ///   2/ automated requeue due to repeats
      ///   3/ automated requeue due to time dependencies
      /// For manual and automated reueue due to repeat's we always clear Flag::NO_REQUE_IF_SINGLE_TIME_DEP
      /// since in those context we do NOT want miss any time slots
      bool reset_next_time_slot = true;
      if (do_reset_next_time_slot) {
         reset_next_time_slot = true;
      }
      else {
         if (flag().is_set(Flag::NO_REQUE_IF_SINGLE_TIME_DEP)) {
            /// If we have done an interactive run or complete, *dont* increment next_time_slot_
            /// allow next time on time based attributes to be incremented and *not* reset,
            /// when force and run commands used
            reset_next_time_slot = false;
         }
      }

      time_dep_attrs_->requeue(reset_next_time_slot);
      time_dep_attrs_->markHybridTimeDependentsAsComplete();
   }


   // reset the flags, however remember if edit were made
   bool edit_history_set = flag().is_set(ecf::Flag::MESSAGE);
   flag_.reset();   // will CLEAR NO_REQUE_IF_SINGLE_TIME_DEP
   if (edit_history_set) flag().set(ecf::Flag::MESSAGE);


   if (lateAttr_) lateAttr_->reset();
   if (child_attrs_) child_attrs_->requeue();

   for(size_t i = 0; i < limitVec_.size(); i++) { limitVec_[i]->reset(); }

   // ECFLOW-196, ensure the re-queue release tokens held by Limits higher up the tree.
   // Note: Its safe to call decrementInLimit, even when no limit consumed
   std::set<Limit*> limitSet;     // ensure local limit have preference over parent
   decrementInLimit(limitSet);    // will recurse up, expensive but needed
}


void Node::requeue_time_attrs()
{
   // Note: we *dont* mark hybrid time dependencies as complete.
   //       i.e. since this is called during alter command, it could be that
   //        the task is in a submitted or active state.
   if (time_dep_attrs_) time_dep_attrs_->requeue(true);
}

void Node::requeue_labels()
{
   if (child_attrs_) child_attrs_->requeue_labels();
}

void Node::miss_next_time_slot()
{
   // Why do we need to set NO_REQUE_IF_SINGLE_TIME_DEP flag ?
   // This is required when we have time based attributes, which we want to miss.
   //    time 10:00
   //    time 12:00
   // Essentially this avoids an automated job run, *IF* the job was run manually for a given time slot.
   // If we call this function before 10:00, we want to miss the next time slot (i.e. 10:00)
   // and want to *requeue*, for 12:00 time slot. However at re-queue, we need to ensure
   // we do *not* reset the 10:00 time slot. hence by setting NO_REQUE_IF_SINGLE_TIME_DEP
   // we allow requeue to query this flag, and hence avoid resetting the time based attribute
   // Note: requeue will *always* clear NO_REQUE_IF_SINGLE_TIME_DEP afterwards.
   //
   // In the case above when we reach the last time slot, there is *NO* automatic requeue, and
   // hence, *no* clearing of NO_REQUE_IF_SINGLE_TIME_DEP flag.
   // This will then be up to any top level parent that has a Repeat/cron to force a requeue
   // when all the children are complete. *or* user does a manual re-queue
   //
   // Additionally if the job *aborts*, we clear NO_REQUE_IF_SINGLE_TIME_DEP if it was set.
   // Otherwise if manually run again, we will miss further time slots.
   if ( time_dep_attrs_) {

      /// Handle abort
      /// The flag: NO_REQUE_IF_SINGLE_TIME_DEP is *only* set when doing an interactive force complete or run command.
      /// What happens if the job aborts during the run command ?
      ///     time 10:00
      ///     time 11:00
      /// If at 9.00am we used the run command, we want to miss the 10:00 time slot.
      /// However if the run at 9.00 fails, and we run again, we also miss 11:00 time slot.
      /// During the run the flag is still set.
      /// Hence *ONLY* miss the next time slot *IF* Flag::NO_REQUE_IF_SINGLE_TIME_DEP is NOT set
      if (!flag().is_set(Flag::NO_REQUE_IF_SINGLE_TIME_DEP)) {

         SuiteChanged0 changed(shared_from_this());
         flag().set(Flag::NO_REQUE_IF_SINGLE_TIME_DEP);

         time_dep_attrs_->miss_next_time_slot();
      }
   }
}

void Node::calendarChanged(
         const ecf::Calendar& c,
         std::vector<node_ptr>& auto_cancelled_nodes)
{
   if (time_dep_attrs_) {
      time_dep_attrs_->calendarChanged(c);
   }

   checkForLateness(c);

   if (checkForAutoCancel(c)) {
      auto_cancelled_nodes.push_back(shared_from_this());
   }
}

void Node::checkForLateness(const ecf::Calendar& c)
{
   if (lateAttr_) {
      lateAttr_->checkForLateness(state_, c);
      if (lateAttr_->isLate()) {
         flag().set(ecf::Flag::LATE);
      }
   }
}

void Node::initState(int clear_suspended_in_child_nodes)
{
   if (defStatus_ == DState::SUSPENDED) {
      /// Note: DState::SUSPENDED is not a real state, its really a user interaction
      /// Replace with suspend, and set underlying state as queued
      suspend();
      setStateOnly( NState::QUEUED );
   }
   else {

      if (clear_suspended_in_child_nodes > 0) {
         clearSuspended();
      }

      // convert DState --> NState.
      // NOTE::  NState does *NOT* have SUSPENDED
      setStateOnly( DState::convert( defStatus_.state())  );
   }
}

void Node::requeueOrSetMostSignificantStateUpNodeTree()
{
   // Get the computed state of my immediate children
   // *** A family can be marked as complete, via complete trigger when not all its children
   // *** are complete, hence computedState() *MUST* first check the immediate state  ,
   // *** before considering its immediate children
   NState::State computedStateOfImmediateChildren = computedState(Node::IMMEDIATE_CHILDREN);

#ifdef DEBUG_REQUEUE
   LogToCout toCoutAsWell; cout << "\n";
   Indentor indent;
   LOG(Log::DBG,"requeueOrSetMostSignificantStateUpNodeTree() " << debugNodePath() << "(" << NState::toString(state()) << ") computed(" << NState::toString(computedStateOfImmediateChildren) << ")");
#endif

   if (computedStateOfImmediateChildren == NState::COMPLETE ) {

       // set most significant state of my immediate children
       // Record: That Suite/Family completed.
       if ( computedStateOfImmediateChildren !=  state() )  {
          setStateOnly( computedStateOfImmediateChildren );
       }

      // For automated re-queue do *not* clear suspended state in *child* nodes
      int clear_suspended_in_child_nodes = -1;

      if (!repeat_.empty()) {

         repeat_.increment();

         // If the repeat is still valid, re-queue the node
#ifdef DEBUG_REQUEUE
         LOG(Log::DBG,"requeueOrSetMostSignificantStateUpNodeTree " << debugNodePath() << " for repeat " << repeat_.toString());
#endif
         if ( repeat_.valid() ) {
#ifdef DEBUG_REQUEUE
            LOG(Log::DBG,"requeueOrSetMostSignificantStateUpNodeTree() VALID for requeue " << debugNodePath() << " for repeat at " << repeat_.toString());
#endif

            /// reset relative duration down the hierarchy from this point. Only valid when we have repeats
            /// Note: Going down hierarchy is wasted if there are no relative time attributes
            resetRelativeDuration();

            // Remove effects of RUN and Force complete interactive commands
            // For automated re-queue *DUE* to Repeats, *CLEAR* any user interaction that would miss the next time slots. *Down* the hierarchy
            // This handles the case where a user, has manually intervened (i.e via run or complete) and we had a time attribute
            // That time attribute will have expired, typically we show next day. In the case where we have a parent repeat
            // we need to clear the flag, otherwise the task/family with time based attribute would wait for next day.
            requeue( false /* don't reset repeats */,
                     clear_suspended_in_child_nodes,
                     true /* reset_next_time_slot */ );
            set_most_significant_state_up_node_tree();
            return;
         }
      }

      /// If user has *INTERACTIVLY* forced changed in state to complete *OR* run the job.
      /// This would cause Node to miss the next time slot. i.e expire the time slot
      /// In which case testTimeDependenciesForRequeue should return false for a single time/today dependency
      /// and not requeue the node.
      if (time_dep_attrs_ && time_dep_attrs_->testTimeDependenciesForRequeue()) {

         // This is the only place we do not explicitly reset_next_time_slot
         bool reset_next_time_slot = false;

         // Remove effects of RUN and Force complete interactive commands, *BUT* only if *not* applied to this cron
         if (!time_dep_attrs_->crons().empty()) {
            if (!flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP)) {
                reset_next_time_slot = true ;
            }
         }

         requeue( false /* don't reset repeats */,
                  clear_suspended_in_child_nodes,
                  reset_next_time_slot  );
         set_most_significant_state_up_node_tree();
         return;
      }
   }

   // In case compute state is other that COMPLETE, update. i,e for Family/Suite
   if ( computedStateOfImmediateChildren !=  state() )  {
      setStateOnly( computedStateOfImmediateChildren );
   }


   // recurse up the node tree
   Node* theParentNode = parent();
   if (theParentNode) {
      theParentNode->requeueOrSetMostSignificantStateUpNodeTree();
   }
   else {
      // No parent, hence next level is the root, ie the Defs
      // Reflect the status of all the suite's
      // **** This should not recurse down, just reflect status of suites
      defs()->set_most_significant_state();
   }
}

void Node::set_most_significant_state_up_node_tree()
{
   if (isTask()) {
      parent()->set_most_significant_state_up_node_tree();
      return;
   }

   // set most significant state  of my immediate children
   NState::State computedStateOfImmediateChildren = computedState(Node::IMMEDIATE_CHILDREN);
   if ( computedStateOfImmediateChildren !=  state() )  {
      setStateOnly( computedStateOfImmediateChildren );
   }

   // recurse up the node tree
   Node* theParentNode = parent();
   if (theParentNode) {
      theParentNode->set_most_significant_state_up_node_tree();
   }
   else {
      // No parent, hence next level is the root, ie the Defs
      // Reflect the status of all the suite's
      // **** This should not recurse down, just reflect status of suites
      defs()->set_most_significant_state();
   }
}
void Node::resetRelativeDuration()
{
   if (time_dep_attrs_) time_dep_attrs_->resetRelativeDuration();
}


// Returning false, *STOPS* further traversal *DOWN* the node tree
bool Node::resolveDependencies(JobsParam& jobsParam)
{
   // This function is called:
   //    a/ Periodically by the server, i.e every minute
   //    b/ Asyncrousnly, after child command, via job submission
#ifdef DEBUG_DEPENDENCIES
   LogToCout toCoutAsWell; cout << "\n";
   LOG(Log::DBG,"   " << debugNodePath() << "::resolveDependencies " << NState::toString(state()) << " AT " << suite()->calendar().toString());
#endif

   // Improve the granularity for the check for lateness (during job submission). See SUP-873 "late" functionality
   if (lateAttr_) {
      // since the suite() traverse up the tree, only call when have a late attribute
      checkForLateness(suite()->calendar());
   }

   if (isSuspended()) {
#ifdef DEBUG_DEPENDENCIES
      LOG(Log::DBG,"   Node::resolveDependencies() " << absNodePath() << " HOLDING as node state " << NState::toString(state()) << " is SUSPENDED " );
#endif
      return false;
   }

   if (state() == NState::COMPLETE) {
#ifdef DEBUG_DEPENDENCIES
      LOG(Log::DBG,"   Node::resolveDependencies() " << absNodePath() << " HOLDING as node state " << NState::toString(state()) << " is not valid for job submission" );
#endif
      return false;
   }

   if (time_dep_attrs_ && !time_dep_attrs_->timeDependenciesFree()) {
#ifdef DEBUG_DEPENDENCIES
      const Calendar& calendar = suite()->calendar();
      LOG(Log::DBG,"   Node::resolveDependencies() " << absNodePath() << " HOLDING due to time dependencies at " << calendar.toString());
#endif
      return false;
   }

   // Complete *MUST* be evaluated before trigger. As it can affect the other
   // i.e A state change to COMPLETE, in which case no need to submit tasks
   // However if the complete does *not* evaluate it should *NOT* hold the node.
   if ( evaluateComplete() ) {
      if (completeAst()) {

         flag().set(ecf::Flag::BYRULE);

         // If we are a parent sets the state first. then set state on all the children
         set_state_hierarchically( NState::COMPLETE,false); // reset try no and decrement inlimit resources & update repeats

#ifdef DEBUG_DEPENDENCIES
         LOG(Log::DBG,"   Node::resolveDependencies() " << absNodePath() << " HOLDING since evaluation of COMPLETE");
#endif
         // We have a complete By returning false here, we stop dependency evaluation for any children
         return false;
      }
   }
   // No complete, or we have a complete  that does not evaluate.(this should not hold the node)


   if ( evaluateTrigger() ) {
      // WE only get here **IF** :
      // 1/ There is no trigger
      // 2/ WE have a trigger and it evaluates to true
#ifdef DEBUG_DEPENDENCIES
      LOG(Log::DBG,"   Node::resolveDependencies() " << absNodePath() << " FREE of TRIGGER");
#endif
      return true;
   }

   // We *have* a trigger and it does not evaluate, hold the node
#ifdef DEBUG_DEPENDENCIES
   LOG(Log::DBG,"   Node::resolveDependencies() " << absNodePath() << " HOLDING due to TRIGGER");
#endif
   return false;
}

void Node::freeTrigger() const
{
   if (triggerExpr_) triggerExpr_->setFree();
}

void Node::clearTrigger() const
{
   if (triggerExpr_) triggerExpr_->clearFree();
}

void Node::freeComplete() const
{
   if (completeExpr_) completeExpr_->setFree();
}

void Node::clearComplete() const
{
   if (completeExpr_) completeExpr_->clearFree();
}

void Node::freeHoldingDateDependencies()
{
   if (time_dep_attrs_) time_dep_attrs_->freeHoldingDateDependencies();
}

void Node::freeHoldingTimeDependencies()
{
   if (time_dep_attrs_) time_dep_attrs_->freeHoldingTimeDependencies();
}

bool Node::checkForAutoCancel(const ecf::Calendar& calendar) const
{
   if ( autoCancel_ && state() == NState::COMPLETE) {
      if (autoCancel_->isFree(calendar,state_.second)) {

         /// *Only* delete this node if we don't create zombies
         /// anywhere for our children
         vector<Task*> taskVec;
         getAllTasks(taskVec);
         BOOST_FOREACH(Task* t, taskVec) {
            if (t->state() == NState::ACTIVE || t->state() == NState::SUBMITTED) {
               return false;
            }
         }
         return true;
      }
   }
   return false;
}

bool Node::evaluateComplete() const
{
   // Complete *MUST* be evaluate before trigger. As it can affect the other
   AstTop* theCompeteAst = completeAst();
   if (theCompeteAst) {
      // *NOTE* if we have a non NULL complete ast, we must have complete expression
      // The freed state is stored on the expression ( i.e not on the ast)
      // ISSUE: Complete expression can not be by-passed in the GUI
      if (completeExpr_->isFree() || theCompeteAst->evaluate()) {

         // Note: if a task has been set complete, the use may decide to place into queued state( via GUI)
         //       In which case, we *want* this complete expression to be re-evaluated.
         //       Hence the old code below has been commented out.
         // >>old: Set the complete as free, until begin()/requeue, // Only update state change no, if state has changed.
         // >>old:if (!completeExpr_->isFree()) freeComplete();

         // ECFLOW-247 Family goes complete despite active child
         // if computedState state is:
         //    NState::ABORTED   -> don't complete if any of the children are aborted  -> ECFLOW-247
         //                         This can hide active/submitted nodes, as abort has higher priority
         //    NState::ACTIVE    -> can cause zombies
         //    NState::SUBMITTED -> can cause zombies
         // hence only allow complete is we are in a NState::QUEUED state
         NState::State theComputedState = computedState(Node::HIERARCHICAL);
         if ( theComputedState != NState::QUEUED ) {
#ifdef DEBUG_DEPENDENCIES
            LOG(Log::DBG,"   Node::evaluateComplete() " << absNodePath() << " AST evaluation succeeded *BUT* " << debugType() << " has children in ACTIVE or SUBMITTED States" );
#endif
            return false;
         }

#ifdef DEBUG_DEPENDENCIES
         LOG(Log::DBG,"   Node::evaluateComplete() " << debugNodePath() << " FREE, COMPLETE AST evaluation succeeded " );
#endif
         return true;
      }
      else {
         /// *IMPORTANT* When a complete does not evaluate, it should *NOT* stop further tree walking
#ifdef DEBUG_DEPENDENCIES
         LOG(Log::DBG,"   Node::evaluateComplete() " << debugNodePath() << " HOLDING, COMPLETE AST evaluation failed" );
#endif
         return false;
      }
   }
   return true;
}

bool Node::evaluateTrigger() const
{
   AstTop* theTriggerAst = triggerAst();
   if (theTriggerAst) {

      // Note 1: A trigger can be freed by the ForceCmd
      // Note 2: if we have a non NULL trigger ast, we must have trigger expression
      // Note 3: The freed state is stored on the expression ( i.e *NOT* on the ast (abstract syntax tree) )
      if (triggerExpr_->isFree() || theTriggerAst->evaluate()) {

         // *ALWAYS* evaluate trigger expression unless user has forcibly removed trigger dependencies
         // ******** This allows force queued functionality, to work as expected, since trigger's will be honoured
         // The old code below has been commented out.
         // >> old: Set the trigger as free, until begin()/requeue.  Only update state change no, when required
         // >> old: if (!triggerExpr_->isFree()) freeTrigger();

#ifdef DEBUG_DEPENDENCIES
         LOG(Log::DBG,"   Node::evaluateTrigger() " << debugNodePath() << " FREE, TRIGGER AST evaluation succeeded" );
#endif
         return true;
      }

#ifdef DEBUG_DEPENDENCIES
      LOG(Log::DBG,"   Node::evaluateTrigger() " << debugNodePath() << " HOLDING  TRIGGER AST evaluation failed" );
#endif
      return false; // evaluation failed. this Node holds
   }

#ifdef DEBUG_DEPENDENCIES
   LOG(Log::DBG,"   Node::evaluateTrigger() " << debugNodePath() << " FREE NO TRIGGER defined" );
#endif
   return true;
}

const std::string& Node::abortedReason() const { return Str::EMPTY(); }

void Node::set_state(NState::State s, bool force, const std::string& additional_info_to_log)
{
   setStateOnly(s,false,additional_info_to_log);

   // Handle any state change specific functionality. This will update any repeats
   // This is a virtual function, since we want different behaviour during state change
   handleStateChange();
}

void Node::setStateOnly(NState::State newState, bool force, const std::string& additional_info_to_log)
{
   Suite* theSuite =  suite();
   const Calendar& calendar = theSuite->calendar();

#ifdef DEBUG_JOB_SUBMISSION_INTERVAL
   // check sub submission interval/calendar increment for tasks only
   // The job submission interval is set/obtained from the server environment and
   // is configurable for testing.
   // **** This is only used when jobSubmissionInterval is less than 60 ****
   // We are attempting to refine job submission interval such that it is just
   // greater than time taken for state change from submit->active->complete
   // There by speeding up the test where we generate .ecf

   // Ignore this during simulation, ie defs()->server().jobSubmissionInterval() = 0; for simulation
   int jobSubmissionInterval = theSuite->defs()->server().jobSubmissionInterval();
   if (isSubmittable() && jobSubmissionInterval != 0 && jobSubmissionInterval < 60) {

      if (newState == NState::SUBMITTED)  submit_to_complete_duration_ = Calendar::second_clock_time();
      else if (newState == NState::COMPLETE && !submit_to_complete_duration_.is_special())  {

         // Under HYBRID we can go from UNKNOWN->COMPLETE, missing out SUBMITTED
         // hence: submit_to_complete_duration_ is never initialised
         // ie when we have a,date,cron dependency that relies on a day change

         time_duration td = (Calendar::second_clock_time() - submit_to_complete_duration_);
         // cout << debugNodePath() << " submit->active->complete time = " << td.total_seconds()  << " seconds.\n";

         // Avoid this check if we have meters. as we wait a second between each meter update
         if ( td.total_seconds() >= jobSubmissionInterval && ((!child_attrs_)  || child_attrs_->meters().empty())) {

            const Variable& do_check = theSuite->findVariable("CHECK_TASK_DURATION_LESS_THAN_SERVER_POLL");
            if (!do_check.empty()) {
               // 				cout << "Calendar::second_clock_time() = " << to_simple_string(Calendar::second_clock_time()) << "\n";
               // 				cout << "submit_to_complete_duration_ = " << to_simple_string(submit_to_complete_duration_) << "\n";
               // 				cout << "(Calendar::second_clock_time() - submit_to_complete_duration_) = " << to_simple_string(td) << "\n";
               cout << "Testing::" << debugNodePath() << " For each job submission interval of " << jobSubmissionInterval
                        << " seconds, the calendar is increment by 60 seconds.\n"
                        << " The job submission interval is too small as it takes " << td.total_seconds()
                        << " seconds for state change of submit->active->complete, for an empty job.\n"
                        << " Please increase the job submission interval to at least " << (td.total_seconds() + 1) << " seconds.\n";
            }
         }
      }
   }
#endif

   // Change format is significant it is used in verification of log files
   // Please change/update LogVerification::extractNodePathAndState() all verification relies on this one function
   //           " " +  submitted(max) + ": " + path(estimate)  + " try-no: " + try_no(estimate)  + " reason: " + reason(estimate)
   // reserve : 1   +  9              + 2    + 100             + 9           + 3                 + 9           + 12   = 145
   std::string log_state_change; log_state_change.reserve(145 + additional_info_to_log.size());
   log_state_change += " ";
   log_state_change += NState::toString(newState);
   log_state_change += ": ";
   log_state_change += absNodePath();
   if (!additional_info_to_log.empty()) {
      log_state_change += " ";
      log_state_change += additional_info_to_log;
   }

   if ( newState == NState::ABORTED) {
      if (force) flag().set(ecf::Flag::FORCE_ABORT);
      Submittable* submittable = isSubmittable();
      if ( submittable ) {
         flag().set(ecf::Flag::TASK_ABORTED);
         log_state_change += " try-no: ";
         log_state_change += submittable->tryNo();
         log_state_change += " reason: ";
         log_state_change += abortedReason();
      }
   }
   else {
      flag().clear(ecf::Flag::TASK_ABORTED);
      flag().clear(ecf::Flag::FORCE_ABORT);
   }

   // SUP-408 what does submitted mean in log?
   // We want to mimimize calls to create a new time stamp in the log file.
   // A time stamp is automatically created, whenever a *new* client request is received, & then cached
   // However we can get a change in state, during tree traversal, when a node is free of its dependencies
   // If we were to just log the message it would use the last cached time stamp. Giving misleading info:
   // Since state changes are bubbled up, we need only update the time stamp for tasks, when not in a command
   if (!CmdContext::in_command() && isTask() && Log::instance()) {
       //std::cout << "!!!!! NOT in cmd context updating time stamp before logging\n";
       Log::instance()->cache_time_stamp();
   }

   ecf::log(Log::LOG,log_state_change);  // Note: log type, must be same for debug & release for test, i.e for log file verification

   state_.first.setState(newState);      // this will update state_change_no
   state_.second = calendar.duration();  // record state change duration for late, autocancel,etc

   // Record state changes for verification
   if (misc_attrs_) {
      size_t theSize = misc_attrs_->verifys_.size();
      for(size_t i = 0; i < theSize; i++ ) {
         if (misc_attrs_->verifys_[i].state() == newState)  misc_attrs_->verifys_[i].incrementActual();
      }
   }
}

boost::posix_time::ptime Node::state_change_time() const
{
   const Calendar& calendar = suite()->calendar();
   boost::posix_time::ptime the_state_change_time = calendar.begin_time();
   the_state_change_time += state_.second; // state_.second is calendar duration relative to calendar begin_time
   return the_state_change_time;
}


DState::State Node::dstate() const {

   // ECFLOW-139, check for suspended must be done first
   if (isSuspended()) return DState::SUSPENDED;

   switch ( state() ) {
      case NState::COMPLETE:  return DState::COMPLETE; break;
      case NState::ABORTED:   return DState::ABORTED; break;
      case NState::ACTIVE:    return DState::ACTIVE; break;
      case NState::SUBMITTED: return DState::SUBMITTED; break;
      case NState::QUEUED:    return DState::QUEUED; break;
      case NState::UNKNOWN:   return DState::UNKNOWN; break;
   }
   return DState::UNKNOWN;
}

bool Node::set_event( const std::string& event_name_or_number)  {
   if (child_attrs_) return child_attrs_->set_event(event_name_or_number);
   return false;
}
bool Node::clear_event(const std::string& event_name_or_number ){
   if (child_attrs_) return child_attrs_->clear_event(event_name_or_number);
   return false;
}

void Node::setRepeatToLastValue()
{
   repeat_.setToLastValue(); // no op for empty repeat
   repeat_.increment();      // make repeat invalid
}

bool Node::check_in_limit_up_node_tree() const
{
   if (!inLimitMgr_.inLimit()) return false;

   Node* theParent = parent();
   while (theParent) {
      if (!theParent->inLimitMgr_.inLimit())  return false;
      theParent = theParent->parent();
   }
   return true;
}

void Node::incrementInLimit(std::set<Limit*>& limitSet) const
{
   //	cout << "Node::incrementInLimit " << absNodePath() << endl;
   std::string the_abs_node_path = absNodePath();
   inLimitMgr_.incrementInLimit(limitSet,the_abs_node_path);

   Node* theParent = parent();
   while (theParent) {
      theParent->inLimitMgr_.incrementInLimit(limitSet,the_abs_node_path);
      theParent = theParent->parent();
   }
}

void Node::decrementInLimit(std::set<Limit*>& limitSet) const
{
   //	cout << "Node::decrementInLimit " << absNodePath() << endl;
   std::string the_abs_node_path = absNodePath();
   inLimitMgr_.decrementInLimit(limitSet,the_abs_node_path);

   Node* theParent = parent();
   while (theParent) {
      theParent->inLimitMgr_.decrementInLimit(limitSet,the_abs_node_path);
      theParent = theParent->parent();
   }
}

static bool search_user_edit_variables( const std::string& name, std::string& value, const NameValueMap& user_edit_variables )
{
   NameValueMap::const_iterator i = user_edit_variables.find(name);
   if (i != user_edit_variables.end()) {
      if (((*i).second).empty()) {
         // when we call --edit_script submit file, before a job is submitted the values
         // of generated variables like  ECF_RID, ECF_TRYNO, ECF_NAME, ECF_PASS, ECF_JOB, ECF_JOBOUT, ECF_SCRIPT
         // will be empty. In this case return false, so that we pick up the values from the node.
         return false;
      }
      value = (*i).second;
      return true;
   }
   return false;
}

//#define DEBUG_S 1
bool Node::variableSubsitution(std::string& cmd) const
{
   char micro = '%';
   std::string micro_char;
   findParentUserVariableValue(Str::ECF_MICRO(),micro_char);
   if (!micro_char.empty() && micro_char.size() == 1) {
      micro = micro_char[0];
   }

   NameValueMap user_edit_variables;
   return variable_substitution(cmd,user_edit_variables,micro);
}

bool Node::variable_substitution(std::string& cmd, const NameValueMap& user_edit_variables, char micro) const
{
   // scan the command for variables, and substitute
   // edit ECF_FETCH "/home/ma/map/sms/smsfectch -F %ECF_FILES% -I %ECF_INCLUDE%"
   // We can also have
   //
   // "%<VAR>:<substitute>% i.e if VAR exist use it, else use substitute
   //
   // ************************************************************************************************************
   // Special case handling for user variables, and generated variables, which take precedence over node variables
   // ************************************************************************************************************
   //
   // i.e VAR is defined as BILL
   //  %VAR:fred --f%  will either be "BILL" or if VAR is not defined "fred --f"
   //
   // Infinite recursion. Its possible to end up with infinite recursion:
   //   	edit hello '%hello%'  # string like %hello% will cause infinite recursion
   //   	edit fred '%bill%'
   //	 	edit bill '%fred%'   # should be 10
   // To prevent this we will use a simple count
#ifdef DEBUG_S
   cout << "cmd  = " << cmd << "\n";
#endif
   bool double_micro_found = false;
   std::string::size_type pos = 0;
   int count = 0;
   while ( 1 ) {
      // A while loop here is used to:
      //		a/ Allow for multiple substitution on a single line. i.e %ECF_FILES% -I %ECF_INCLUDE%"
      //    b/ Allow for recursive substitution. %fred% -> %bill%--> 10

      size_t firstPercentPos = cmd.find( micro, pos );
      if ( firstPercentPos == string::npos ) break;

      size_t secondPercentPos = cmd.find( micro, firstPercentPos + 1 );
      if ( secondPercentPos == string::npos ) break;

      if ( secondPercentPos - firstPercentPos <= 1 ) {
         // handle %% with no characters in between, skip over
         // i.e to handle "printf %%02d %HOUR:00%" --> "printf %02d 00"   i.e if HOUR not defined
         pos = secondPercentPos + 1;
         double_micro_found = true;
         continue;
      }
      else pos = 0;

      string percentVar( cmd.begin() + firstPercentPos+1, cmd.begin() + secondPercentPos );
#ifdef DEBUG_S
      cout << "   Found percentVar " << percentVar << "\n";
#endif


      // ****************************************************************************************
      // Look for generated variables first:
      // Variable like ECF_PASS can be overridden, i.e. with FREE_JOBS_PASSWORD
      // However for job file generation we should use use the generated variables first.
      // if the user removes ECF_PASS then we are stuck with the wrong value in the script file
      // FREE_JOBS_PASSWORD is left for the server to deal with
      bool generated_variable = false;
      if ( percentVar.find("ECF_") != std::string::npos) {
         if ( percentVar.find(Str::ECF_PASS())         != std::string::npos) generated_variable = true;
         else if ( percentVar.find(Str::ECF_TRYNO())   != std::string::npos) generated_variable = true;
         else if ( percentVar.find(Str::ECF_JOB())     != std::string::npos) generated_variable = true;
         else if ( percentVar.find(Str::ECF_JOBOUT())  != std::string::npos) generated_variable = true;
         else if ( percentVar.find(Str::ECF_PORT())    != std::string::npos) generated_variable = true;
         else if ( percentVar.find(Str::ECF_NODE())    != std::string::npos) generated_variable = true;
         else if ( percentVar.find(Str::ECF_NAME())    != std::string::npos) generated_variable = true;
      }

      // First search user variable (*ONLY* set when doing user edit's the script)
      // Handle case: cmd = "%fred:bill% and where we have user variable "fred:bill"
      // Handle case: cmd = "%fred%      and where we have user variable "fred"
      // If we fail to find the variable we return false.
      // Note: When a variable is found, it can have an empty value  which is still valid
      std::string varValue;
      if (search_user_edit_variables(percentVar,varValue,user_edit_variables)) {
         cmd.replace( firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue );
      }
      else if (generated_variable && find_parent_gen_variable_value(percentVar,varValue)) {

         cmd.replace( firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue );
      }
      else if (findParentVariableValue( percentVar ,varValue)) {
         // For alias we could have added variables with %A:0%, %A:1%. Aliases allow variables with ':' in the name
         cmd.replace( firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue );
      }
      else {

         size_t firstColon = percentVar.find( ':' );
         if (firstColon != string::npos) {

            string var(percentVar.begin(), percentVar.begin() + firstColon);
#ifdef DEBUG_S
            cout << "   var " << var << "\n";
#endif

            if (search_user_edit_variables(var,varValue,user_edit_variables)) {
#ifdef DEBUG_S
               cout << "   user var value = " << varValue << "\n";
#endif
               cmd.replace(firstPercentPos,secondPercentPos-firstPercentPos+1,varValue);
            }
            else if (generated_variable && find_parent_gen_variable_value(var,varValue)) {
#ifdef DEBUG_S
               cout << "   generated var value = " << varValue << "\n";
#endif
                cmd.replace( firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue );
            }
            else if (findParentVariableValue( var, varValue ))  {
               // Note: variable can exist, but have an empty value
#ifdef DEBUG_S
               cout << "   var value = " << varValue << "\n";
#endif
               // replace the "%VAR:fred --f%" with var
               cmd.replace(firstPercentPos,secondPercentPos-firstPercentPos+1,varValue);
            }
            else {
               string substitute (percentVar.begin()+ firstColon+1, percentVar.end());
#ifdef DEBUG_S
               cout << "  substitute value = " << substitute << "\n";
#endif
               cmd.replace(firstPercentPos,secondPercentPos-firstPercentPos+1,substitute);
            }
#ifdef DEBUG_S
            cout << "   cmd = " << cmd << "\n";
#endif
         }
         else {
            // No Colon, Can't find in user variables, or node variable, hence can't go any further
            return false;
         }
      }

      // Simple Check for infinite recursion
      if (count > 100)  return false;
      count++;
   }

   if (double_micro_found) {
      // replace all double micro with a single micro, this must be a single parse
      // date +%%Y%%m%%d" ==> date +%Y%m%d
      // %%%%             ==> %%            // i.e single parse
      std::string doubleEcfMicro;
      doubleEcfMicro += micro;
      doubleEcfMicro += micro;
      size_t last_pos = 0;
      while ( 1 ) {
          string::size_type ecf_double_micro_pos = cmd.find( doubleEcfMicro , last_pos);
          if ( ecf_double_micro_pos != std::string::npos ) {
             cmd.erase( cmd.begin() + ecf_double_micro_pos );
             last_pos = ecf_double_micro_pos + 1;
          }
          else break;
       }
   }

   return true;
}

bool Node::find_all_used_variables(std::string& cmd, NameValueMap& used_variables, char micro) const
{
#ifdef DEBUG_S
   cout << "cmd  = " << cmd << "\n";
#endif
   int count = 0;
   while ( 1 ) {
      // A while loop here is used to:
      //		a/ Allow for multiple substitution on a single line. i.e %ECF_FILES% -I %ECF_INCLUDE%"
      //    b/ Allow for recursive substitution. %fred% -> %bill%--> 10

      size_t firstPercentPos = cmd.find( micro );
      if ( firstPercentPos == string::npos ) break;
      size_t secondPercentPos = cmd.find( micro, firstPercentPos + 1 );
      if ( secondPercentPos == string::npos ) break;
      if ( secondPercentPos - firstPercentPos <= 1 ) break; // handle %% with no characters in between


      string percentVar( cmd.begin() + firstPercentPos+1, cmd.begin() + secondPercentPos );
#ifdef DEBUG_S
      cout << "   Found percentVar " << percentVar << "\n";
#endif

      size_t firstColon = percentVar.find( ':' );
      if (firstColon != string::npos) {

         string var (percentVar.begin(), percentVar.begin() + firstColon);
#ifdef DEBUG_S
         cout << "   var " << var << "\n";
#endif

         std::string varValue;
         if (findParentVariableValue( var, varValue ))  {
            // Note: variable can exist, but have an empty value
#ifdef DEBUG_S
            cout << "   var value = " << theFoundVariable.value() << "\n";
#endif
            // %VAR:fred% --->  name("VAR:fred") value(theFoundVariable.value())
            used_variables.insert( std::make_pair(percentVar,varValue) );

            // replace the "%VAR:fred --f%" with variable value, so that we dont process it again
            cmd.replace(firstPercentPos,secondPercentPos-firstPercentPos+1,varValue);
         }
         else {
            string substitute (percentVar.begin()+ firstColon+1, percentVar.end());
#ifdef DEBUG_S
            cout << "  substitute value = " << substitute << "\n";
#endif

            // %VAR:fred% --->  name("VAR:fred") value(fred)
            used_variables.insert( std::make_pair(percentVar,substitute));

            cmd.replace(firstPercentPos,secondPercentPos-firstPercentPos+1,substitute);
         }
#ifdef DEBUG_S
         cout << "   cmd = " << cmd << "\n";
#endif
      }
      else {

         // If we fail to find the variable we return false.
         // Note: When a variable is found, it can have an empty value
         //       which is still valid
         std::string varValue;
         if (!findParentVariableValue( percentVar ,varValue)) return false;

         used_variables.insert( std::make_pair(percentVar,varValue) );
         cmd.replace( firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue );
      }

      // Simple Check for infinite recursion
      if (count > 100)  return false;
      count++;
   }
   return true;
}


bool Node::enviromentSubsitution(std::string& cmd)
{
   // scan command for environment variables, and substitute
   // edit ECF_INCLUDE $ECF_HOME/include

   while ( 1 ) {
      size_t firstPos = cmd.find( '$' );
      if ( firstPos == string::npos ) break;

      size_t secondPos = cmd.find_first_not_of( Str::ALPHANUMERIC_UNDERSCORE(), firstPos + 1 );
      if ( secondPos == string::npos )  secondPos = cmd.size();
      if ( secondPos - firstPos <= 1 ) break; // handle $/ with no characters in between

      string env( cmd.begin() + firstPos+1, cmd.begin() + secondPos );
      //cout << "find env " << env << "\n";

      std::string envValue;
      if (! findParentVariableValue( env,envValue )) {
         //cout << " could not find " << env << "\n";
         return false;
      }

      cmd.replace( firstPos, secondPos - firstPos , envValue );
   }
   return true;
}


std::string Node::completeExpression() const
{
   if (completeExpr_) {
      string ret = "complete ";
      ret += completeExpr_->expression();
      return ret;
   }
   return string();
}

std::string Node::triggerExpression() const
{
   if (triggerExpr_) {
      string ret = "trigger ";
      ret += triggerExpr_->expression();
      return ret;
   }
   return string();
}


static void check_expressions(const Node* node, bool trigger, std::string& errorMsg)
{
   Ast* ast = NULL;
   if (trigger) ast = node->triggerAst();
   else         ast = node->completeAst();
   if ( ast ) {
      // The expression have been parsed and we have created the abstract syntax tree
      // Try to resolve the path/node references in the expressions
      // Also resolve references to events,meter,repeats variables.
      AstResolveVisitor astVisitor(node);
      ast->accept(astVisitor);

      if ( !astVisitor.errorMsg().empty() ) {
         errorMsg += "Expression node tree references failed for ";
         if ( trigger ) errorMsg += node->triggerExpression();
         else           errorMsg += node->completeExpression();
         errorMsg += "' at ";
         errorMsg += node->absNodePath();
         errorMsg += "\n ";
         errorMsg += astVisitor.errorMsg();
      }
   }
}

bool Node::check(std::string& errorMsg, std::string& warningMsg) const
{
   //#ifdef DEBUG
   // 	cout << "Node::check " << debugNodePath() << " complete and trigger\n";
   //#endif

   /// ************************************************************************************
   /// *IMPORTANT side effec: *
   /// The simulator relies AstResolveVisitor to set usedInTriggger() for events and meters
   /// *************************************************************************************

   /// Make Sure: To sure capture parser errors:
   /// defs which fail parse errors should not be allowed to be loaded into the server
   /// Even if the code parses, check the expression for divide by zero, for divide and modulo operators
   AstTop* ctop = completeAst(errorMsg);
   if (ctop && !ctop->check(errorMsg)) {
      errorMsg += " ";
      if (completeExpr_) errorMsg += completeExpr_->expression();
      errorMsg += " on ";
      errorMsg += debugNodePath();
   }
   AstTop* ttop = triggerAst(errorMsg);
   if (ttop && !ttop->check(errorMsg)) {
      errorMsg += " ";
      if (triggerExpr_) errorMsg += triggerExpr_->expression();
      errorMsg += " on ";
      errorMsg += debugNodePath();
   }


   // capture node path resolve errors
   check_expressions(this, true,errorMsg);
   check_expressions(this, false,errorMsg);

   // check inLimit references to limits.
   // Client: Unresolved references, which are not in the externs reported as errors/warnings
   // Server: There are no exerns, all unresolved references reported as errors
   bool reportErrors = true;
   bool reportWarnings = true;
   inLimitMgr_.check(errorMsg,warningMsg,reportErrors, reportWarnings);

   return errorMsg.empty();
}

std::string Node::write_state() const
{
   // *IMPORTANT* we *CANT* use ';' character, since is used in the parser, when we have
   //             multiple statement on a single line i.e.
   //                 task a; task b;
   // If attribute correspond to the defaults don't write then out
   std::string ret;
   if (state() != NState::UNKNOWN) {
      ret += " state:";
      ret += NState::toString(state());
   }
   if (state_.second.total_seconds() != 0) {
      ret += " dur:";
      ret += to_simple_string(state_.second);
   }
   if (flag_.flag() != 0) {
      ret += " flag:";
      ret += flag_.to_string();
   }
   if (suspended_) {
      ret += " suspended:1";
   }
   return ret;
}

void Node::read_state(const std::string& line,const std::vector<std::string>& lineTokens)
{
   std::string token;
   for(size_t i = 0; i < lineTokens.size(); i++) {
      token.clear();
      if (lineTokens[i].find("state:") != std::string::npos ) {
         if (!Extract::split_get_second(lineTokens[i],token)) throw std::runtime_error( "Node::read_state Invalid state specified for suite " + name());
         if (!NState::isValid(token)) throw std::runtime_error( "Node::read_state Invalid state specified for node : " + name() );
         set_state_only(NState::toState(token));
      }
      else if (lineTokens[i].find("flag:") != std::string::npos ) {
         if (!Extract::split_get_second(lineTokens[i],token)) throw std::runtime_error( "Node::read_state invalid flags for node "  + name());
         flag().set_flag(token); // this can throw
      }
      else if (lineTokens[i].find("dur:") != std::string::npos ) {
         if (!Extract::split_get_second(lineTokens[i],token)) throw std::runtime_error( "Node::read_state invalid duration for node: " + name());
         state_.second = duration_from_string(token);
      }
      else if (lineTokens[i] == "suspended:1") suspend();
   }
}

std::ostream& Node::print(std::ostream& os) const
{
   if ( defStatus_ != DState::default_state() ) {
      Indentor in;
      Indentor::indent(os) << "defstatus " << DState::toString(defStatus_) << "\n";
   }

   if (lateAttr_) lateAttr_->print(os);

   if (completeExpr_) {
      completeExpr_-> print(os,"complete");
      if ( PrintStyle::getStyle() == PrintStyle::STATE  ) {
         Indentor in;
         if (completeExpr_->isFree()) Indentor::indent(os) << "# (free)\n";
         else                         Indentor::indent(os) << "# (holding)\n";
         if ( completeAst() ) {
            if (!defs()) {
               // Full defs is required for extern checking, and finding absolute node paths
               // Hence print will with no defs can give in-accurate information
               Indentor in;
               Indentor::indent(os) << "# Warning: Full/correct AST evaluation requires the definition\n";
            }
            completeAst()->print(os);
         }
      }
   }
   if (triggerExpr_)  {
      triggerExpr_->print(os,"trigger");
      if ( PrintStyle::getStyle() == PrintStyle::STATE  ) {
         Indentor in;
         if (triggerExpr_->isFree()) Indentor::indent(os) << "# (free)\n";
         else                        Indentor::indent(os) << "# (holding)\n";
         if ( triggerAst() ) {
            if (!defs()) {
               Indentor in;
               Indentor::indent(os) << "# Warning: Full/correct AST evaluation requires the definition\n";
            }
            triggerAst()->print(os);
         }
      }
   }
   repeat_.print(os);

   BOOST_FOREACH(const Variable& v, varVec_ )       { v.print(os); }
   BOOST_FOREACH(limit_ptr l, limitVec_)            { l->print(os); }
   inLimitMgr_.print(os);
   if (child_attrs_) child_attrs_->print(os);
   if (time_dep_attrs_) time_dep_attrs_->print(os);
   if (misc_attrs_) misc_attrs_->print(os);
   if (autoCancel_) autoCancel_->print(os);

   return os;
}

std::string Node::to_string() const
{
   std::stringstream ss;
   print(ss);
   return ss.str();
}

bool Node::operator==(const Node& rhs) const
{
   if ( name_ != rhs.name_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==( name_(" << name_ << ") != rhs.name_(" << rhs.name_ << ")) for: " << debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if ( state() != rhs.state()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  state(" << NState::toString(state()) << ") != rhs.state(" << NState::toString(rhs.state()) << ")) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if ( defStatus_ != rhs.defStatus_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  ( defStatus_ != rhs.defStatus_) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if ( suspended_ != rhs.suspended_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator== suspended_ != rhs.suspended_ " << debugNodePath() << "\n";
      }
#endif
      return false;
   }

   if ( flag_ != rhs.flag_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator== ( flag_ != rhs.flag_) : '" << flag_.to_string() << "' != '" << rhs.flag_.to_string() << "' : " << debugNodePath() << "\n";
      }
#endif
      return false;
   }

   if ( (triggerExpr_ && !rhs.triggerExpr_) || (!triggerExpr_ && rhs.triggerExpr_) ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  (triggerExpr_ && !rhs.triggerExpr_) || (!triggerExpr_&& rhs.triggerExpr_)  " << debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if ( triggerExpr_ && rhs.triggerExpr_ && (*triggerExpr_ != *rhs.triggerExpr_) ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  triggerExpr_ && rhs.triggerExpr_ && (*triggerExpr_ != *rhs.triggerExpr_) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }

   if ( (completeExpr_ && !rhs.completeExpr_) || (!completeExpr_ && rhs.completeExpr_) ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  (completeExpr_ && !rhs.completeExpr_) || (!completeExpr_&& rhs.completeExpr_)  " << debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if ( completeExpr_ && rhs.completeExpr_ && (*completeExpr_ != *rhs.completeExpr_) ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  completeExpr_ && rhs.completeExpr_ && (*completeExpr_ != *rhs.completeExpr_) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }


   if (varVec_.size() != rhs.varVec_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  (varVec_.size() != rhs.varVec_.size()) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(unsigned i = 0; i < varVec_.size(); ++i) {
      if (!(varVec_[i] == rhs.varVec_[i] )) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  (!(varVec_[i] == rhs.varVec_[i] )) " << debugNodePath() << "\n";
            std::cout << "     varVec_[i] name = '" << varVec_[i].name() << "' value = '" << varVec_[i].theValue() << "'\n";
            std::cout << " rhs.varVec_[i] name = '" << rhs.varVec_[i].name() << "' value = '" << rhs.varVec_[i].theValue() << "'\n";
         }
#endif
         return false;
      }
   }
   // We dont compare genvar as this is only used in server environment

   if (!(inLimitMgr_ == rhs.inLimitMgr_)) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  (!(inLimitMgr == rhs.inLimitMgr)) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }

   if (limitVec_.size() != rhs.limitVec_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  (limitVec_.size() != rhs.limitVec_.size()) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(unsigned i = 0; i < limitVec_.size(); ++i) {
      if (!(*limitVec_[i] == *rhs.limitVec_[i] )) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  (!(*limitVec_[i] == *rhs.limitVec_[i] )) " << debugNodePath() << "\n";
         }
#endif
         return false;
      }
   }

   if (( time_dep_attrs_ && !rhs.time_dep_attrs_) || ( !time_dep_attrs_ && rhs.time_dep_attrs_)){
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator== (( time_dep_attrs_ && !rhs.time_dep_attrs_) || ( !time_dep_attrs_ && rhs.time_dep_attrs_)) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if ( time_dep_attrs_ &&  rhs.time_dep_attrs_ && !(*time_dep_attrs_ == *rhs.time_dep_attrs_)) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator== ( time_dep_attrs_ &&   rhs.time_dep_attrs_ && !(*time_dep_attrs_ == *(rhs.time_dep_attrs_))) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }

   if (( child_attrs_ && !rhs.child_attrs_) || ( !child_attrs_ && rhs.child_attrs_)){
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator== (( child_attrs_ && !rhs.child_attrs_) || ( !child_attrs_ && rhs.child_attrs_)) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if ( child_attrs_ &&  rhs.child_attrs_ && !(*child_attrs_ == *rhs.child_attrs_)) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator== ( child_attrs_ && rhs.child_attrs_ && !(*child_attrs_ == *(rhs.child_attrs_))) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }

   if (( misc_attrs_ && !rhs.misc_attrs_) || ( !misc_attrs_ && rhs.misc_attrs_)){
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator== (( misc_attrs_ && !rhs.misc_attrs_) || ( !misc_attrs_ && rhs.misc_attrs_)) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if ( misc_attrs_ &&  rhs.misc_attrs_ && !(*misc_attrs_ == *rhs.misc_attrs_)) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator== ( misc_attrs_ && rhs.misc_attrs_ && !(*misc_attrs_ == *(rhs.misc_attrs_))) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }

   if (autoCancel_ && !rhs.autoCancel_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  if (autoCancel_ && !rhs.autoCancel_)  " << debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if (!autoCancel_ && rhs.autoCancel_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  if (!autoCancel_ && rhs.autoCancel_)  " << debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if (autoCancel_ && rhs.autoCancel_ && !(*autoCancel_ == *rhs.autoCancel_)) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  (autoCancel_ && rhs.autoCancel_ && !(*autoCancel_ == *rhs.autoCancel_)) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }


   if (!(repeat_ == rhs.repeat_)) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==   if (!(repeat_ == rhs.repeat_)) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }


   if (( lateAttr_ && !rhs.lateAttr_) || ( !lateAttr_ && rhs.lateAttr_)){
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator== (( lateAttr_ && !rhs.lateAttr_) || ( !lateAttr_ && rhs.lateAttr_)) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if ( lateAttr_ &&  rhs.lateAttr_ && !(*lateAttr_ == *rhs.lateAttr_)) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator== ( lateAttr_ &&   rhs.lateAttr_ && !(*lateAttr_ == *(rhs.lateAttr_))) " << debugNodePath() << "\n";
      }
#endif
      return false;
   }

   return true;
}


//#define DEBUG_WHY 1

void Node::top_down_why(std::vector<std::string>& theReasonWhy) const
{
   why(theReasonWhy);
}

void Node::bottom_up_why(std::vector<std::string>& theReasonWhy) const
{
   defs()->why(theReasonWhy);

   std::vector<Node*> vec;
   vec.push_back(const_cast<Node*>(this));
   Node* theParent = parent();
   while (theParent) {
      vec.push_back(theParent);
      theParent = theParent->parent();
   }
   vector<Node*>::reverse_iterator r_end = vec.rend();
   for(vector<Node*>::reverse_iterator r = vec.rbegin(); r!=r_end; ++r) {
      (*r)->why(theReasonWhy);
   }
}

void Node::why(std::vector<std::string>& vec) const
{
#ifdef DEBUG_WHY
   std::cout << "Node::why " << debugNodePath() << " (" << NState::toString(state()) << ")\n";
#endif
   if (isSuspended()) {
      std::string theReasonWhy = "The node '";
      theReasonWhy += debugNodePath();
      theReasonWhy += "' is suspended.";
      vec.push_back(theReasonWhy);
   }
   else if (state() != NState::QUEUED && state() != NState::ABORTED) {
      std::stringstream ss;
      ss << "The node '" << debugNodePath() << "' (" << NState::toString(state()) << ") is not queued or aborted.";
      vec.push_back(ss.str());

      // When task is active/submitted no point, going any further.
      // However for FAMILY/SUITE we still need to proceed
      if (isTask()) return;
   }

   // Check  limits using in limit manager
   inLimitMgr_.why(vec);

   // Prefix <node-type> <path> <state>
   std::string prefix = debugType();
   prefix += " ";
   prefix += absNodePath();
   prefix += " (";
   prefix += NState::toString(state());
   prefix += ") ";

   if (time_dep_attrs_) {
#ifdef DEBUG_WHY
      std::cout << "   Node::why " << debugNodePath() << " checking time dependencies\n";
#endif
      // postfix  = <attr-type dependent> <next run time > < optional current state>
      time_dep_attrs_->why(vec,prefix);
   }

   // **************************************************************************************
   // If we have a complete expression that does not evaluate then it should *NOT* hold the node.
   // The complete expression is used to set node to complete, when it evaluates and hence
   // should not prevent further tree walking. evaluate each leaf branch
   // **************************************************************************************
   AstTop* theTriggerAst = triggerAst();
   if (theTriggerAst) {
      // Note 1: A trigger can be freed by the ForceCmd
      // Note 2: if we have a non NULL trigger ast, we must have trigger expression
      // Note 3: The freed state is stored on the expression ( i.e *NOT* on the ast (abstract syntax tree) )
      if (!triggerExpr_->isFree() ) {

#ifdef DEBUG_WHY
         std::cout << "   Node::why " << debugNodePath() << " checking trigger dependencies\n";
#endif
         std::string postFix;
         if (theTriggerAst->why(postFix)) { vec.push_back(prefix + postFix); }
      }
   }
}

bool Node::checkInvariants(std::string& errorMsg) const
{
   if (time_dep_attrs_) {
      if (!time_dep_attrs_->checkInvariants(errorMsg)) {
         return false;
      }
   }
   if (!repeat_.empty()) {
      if (repeat_.name().empty()) {
         errorMsg += "Repeat name empty ???";
         return false;
      }
   }
   return true;
}

std::string Node::absNodePath() const
{
   std::vector<std::string> vec; vec.reserve(Str::reserve_16());
   vec.push_back(name());
   Node* theParent = parent();
   while (theParent) {
      vec.push_back(theParent->name());
      theParent = theParent->parent();
   }
   std::string ret; ret.reserve(Str::reserve_64());
   vector<string>::reverse_iterator r_end = vec.rend();
   for(vector<string>::reverse_iterator r = vec.rbegin(); r!=r_end; ++r) {
      ret += '/';
      ret += *r;
   }

   //	// Another algorithm broadly similar results
   //	std::string ret; ret.reserve(Str::reserve_64());
   // 	ret += '/';
   //	ret += name();
   // 	Node* theParent = parent();
   //	while (theParent) {
   //		ret.insert(0,"/");
   //		ret.insert(1,theParent->name());
   // 		theParent = theParent->parent();
   //	}

   return ret;
}

std::string Node::debugNodePath() const
{
   std::string ret = debugType();
   ret += Str::COLON();
   ret += absNodePath();
   return ret;
}

void Node::verification(std::string& errorMsg) const
{
   if (misc_attrs_) misc_attrs_->verification(errorMsg);
}

void Node::getAllAstNodes(std::set<Node*>& theSet) const
{
   if ( completeAst() ) {
      AstCollateNodesVisitor astVisitor(theSet);
      completeAst()->accept(astVisitor);
   }
   if ( triggerAst()  ) {
      AstCollateNodesVisitor astVisitor(theSet);
      triggerAst()->accept(astVisitor);
   }
}

AstTop* Node::completeAst() const
{
   if (completeExpr_) {
      std::string ignoredErrorMsg;
      (void) completeAst(ignoredErrorMsg);
      return completeExpr_->get_ast();
   }
   return NULL;
}

AstTop* Node::triggerAst() const
{
   if (triggerExpr_) {
      std::string ignoredErrorMsg;
      (void) triggerAst(ignoredErrorMsg);
      return triggerExpr_->get_ast();
   }
   return NULL;
}

AstTop* Node::completeAst(std::string& errorMsg) const
{
   if (completeExpr_ && completeExpr_->get_ast() == NULL) {
      completeExpr_->createAST(const_cast<Node*>(this),"complete",errorMsg);
#ifdef DEBUG
      if (errorMsg.empty()) LOG_ASSERT(completeExpr_->get_ast(),"");
#endif
      return completeExpr_->get_ast();
   }
   return NULL;
}

AstTop* Node::triggerAst(std::string& errorMsg) const
{
   if (triggerExpr_ && triggerExpr_->get_ast() == NULL) {
      triggerExpr_->createAST(const_cast<Node*>(this),"trigger",errorMsg);
#ifdef DEBUG
      if (errorMsg.empty()) LOG_ASSERT(triggerExpr_->get_ast(),"");
#endif
      return triggerExpr_->get_ast();
   }
   return NULL;
}

node_ptr Node::remove()
{
   SuiteChanged0 changed(shared_from_this());

   Node* theParent = parent();
   if ( theParent ) return theParent->removeChild( this );
   return defs()->removeChild( this );
}

bool Node::getLabelValue(const std::string& labelName, std::string& value) const
{
   if (child_attrs_) return child_attrs_->getLabelValue(labelName,value);
   return false;
}

size_t Node::position() const
{
   Node* theParent = parent();
   if (theParent) {
      return theParent->child_position(this);
   }
   else {
      Defs* theDefs = defs();
      if (theDefs) {
         return theDefs->child_position(this);
      }
   }
   return std::numeric_limits<std::size_t>::max();
}


void Node::gen_variables(std::vector<Variable>& vec) const
{
   if (!repeat_.empty()) {
      vec.push_back(repeat_.gen_variable());
   }
}

const Variable& Node::findGenVariable(const std::string& name) const
{
   if (!repeat_.empty() && repeat_.name() == name) return repeat_.gen_variable();
   return Variable::EMPTY();
}

void Node::update_repeat_genvar() const
{
   if (!repeat_.empty()) {
      repeat_.update_repeat_genvar();
   }
}

static std::vector<ecf::TimeAttr>  timeVec_;
static std::vector<ecf::TodayAttr> todayVec_;
static std::vector<DateAttr>       dates_;
static std::vector<DayAttr>        days_;
static std::vector<ecf::CronAttr>  crons_;
const std::vector<ecf::TimeAttr>&   Node::timeVec()  const { if (time_dep_attrs_) return time_dep_attrs_->timeVec(); return timeVec_; }
const std::vector<ecf::TodayAttr>&  Node::todayVec() const { if (time_dep_attrs_) return time_dep_attrs_->todayVec();return todayVec_; }
const std::vector<DateAttr>&        Node::dates()    const { if (time_dep_attrs_) return time_dep_attrs_->dates(); return dates_; }
const std::vector<DayAttr>&         Node::days()     const { if (time_dep_attrs_) return time_dep_attrs_->days(); return days_; }
const std::vector<ecf::CronAttr>&   Node::crons()    const { if (time_dep_attrs_) return time_dep_attrs_->crons(); return crons_; }
std::vector<ecf::TimeAttr>::const_iterator Node::time_begin() const   { if (time_dep_attrs_) return time_dep_attrs_->time_begin(); return timeVec_.begin();}
std::vector<ecf::TimeAttr>::const_iterator Node::time_end() const     { if (time_dep_attrs_) return time_dep_attrs_->time_end(); return timeVec_.end();}
std::vector<ecf::TodayAttr>::const_iterator Node::today_begin() const { if (time_dep_attrs_) return time_dep_attrs_->today_begin(); return todayVec_.begin();}
std::vector<ecf::TodayAttr>::const_iterator Node::today_end() const   { if (time_dep_attrs_) return time_dep_attrs_->today_end(); return todayVec_.end();}
std::vector<DateAttr>::const_iterator Node::date_begin() const {        if (time_dep_attrs_) return time_dep_attrs_->date_begin(); return dates_.begin();}
std::vector<DateAttr>::const_iterator Node::date_end() const {          if (time_dep_attrs_) return time_dep_attrs_->date_end(); return dates_.end();}
std::vector<DayAttr>::const_iterator Node::day_begin() const {          if (time_dep_attrs_) return time_dep_attrs_->day_begin(); return days_.begin();}
std::vector<DayAttr>::const_iterator Node::day_end() const {            if (time_dep_attrs_) return time_dep_attrs_->day_end(); return days_.end();}
std::vector<ecf::CronAttr>::const_iterator Node::cron_begin() const {   if (time_dep_attrs_) return time_dep_attrs_->cron_begin(); return crons_.begin();}
std::vector<ecf::CronAttr>::const_iterator Node::cron_end() const {     if (time_dep_attrs_) return time_dep_attrs_->cron_end(); return crons_.end();}

static std::vector<Meter> meters_;
static std::vector<Event> events_;
static std::vector<Label> labels_;
const std::vector<Meter>& Node::meters() const { if (child_attrs_) return child_attrs_->meters(); return meters_;}
const std::vector<Event>& Node::events() const { if (child_attrs_) return child_attrs_->events(); return events_;}
const std::vector<Label>& Node::labels() const { if (child_attrs_) return child_attrs_->labels(); return labels_;}
std::vector<Meter>&  Node::ref_meters()        { if (child_attrs_) return child_attrs_->ref_meters(); return meters_;} // allow simulator set meter value
std::vector<Event>&  Node::ref_events()        { if (child_attrs_) return child_attrs_->ref_events(); return events_;} // allow simulator set event value
std::vector<Meter>::const_iterator Node::meter_begin() const { if (child_attrs_) return child_attrs_->meter_begin(); return meters_.begin();}
std::vector<Meter>::const_iterator Node::meter_end() const {   if (child_attrs_) return child_attrs_->meter_end(); return meters_.end();}
std::vector<Event>::const_iterator Node::event_begin() const { if (child_attrs_) return child_attrs_->event_begin(); return events_.begin();}
std::vector<Event>::const_iterator Node::event_end() const {   if (child_attrs_) return child_attrs_->event_end(); return events_.end();}
std::vector<Label>::const_iterator Node::label_begin() const { if (child_attrs_) return child_attrs_->label_begin(); return labels_.begin();}
std::vector<Label>::const_iterator Node::label_end() const {   if (child_attrs_) return child_attrs_->label_end(); return labels_.end();}

static std::vector<VerifyAttr> verifys_;
static std::vector<ZombieAttr> zombies_;
const std::vector<VerifyAttr>& Node::verifys()  const { if (misc_attrs_) return misc_attrs_->verifys(); return verifys_;}
const std::vector<ZombieAttr>& Node::zombies()  const { if (misc_attrs_) return misc_attrs_->zombies(); return zombies_; }
std::vector<ZombieAttr>::const_iterator Node::zombie_begin() const { if (misc_attrs_) return misc_attrs_->zombie_begin(); return zombies_.begin();}
std::vector<ZombieAttr>::const_iterator Node::zombie_end() const {   if (misc_attrs_) return misc_attrs_->zombie_end(); return zombies_.end();}
std::vector<VerifyAttr>::const_iterator Node::verify_begin() const { if (misc_attrs_) return misc_attrs_->verify_begin(); return verifys_.begin();}
std::vector<VerifyAttr>::const_iterator Node::verify_end() const {   if (misc_attrs_) return misc_attrs_->verify_end(); return verifys_.end();}
