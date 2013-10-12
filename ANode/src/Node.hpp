#ifndef NODE_HPP_
#define NODE_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #251 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
// Node: The node class does NOT serialise the triggers and complete.
//       These are created on demand in the server.
//       However when the client code loads the definition file, the Defs::check
//       will create AST for trigger and complete expressions.
//       Because:
//          1/ the AST are created, so that any parser errors can be _reported_ to the user
//          2/ References in the AST expressions are resolved, and errors flagged.
//       This information could have been saved, however was _not_.
//       Because:
//          1/ problems on AIX
//          2/ Cut down on IPC load between client/server
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <ostream>
#include <set>
#include <limits>

#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>         // no need to include <vector>
#include <boost/serialization/string.hpp>         // no need to include <string>
#include <boost/serialization/shared_ptr.hpp>     // no need to include shared_ptr
#include <boost/foreach.hpp>                      // used so often just placed here for convenience

#include "DState.hpp"
#include "NOrder.hpp"
#include "NodeAttr.hpp"
#include "Limit.hpp"
#include "InLimit.hpp"
#include "Variable.hpp"
#include "LateAttr.hpp"
#include "RepeatAttr.hpp"
#include "AutoCancelAttr.hpp"
#include "Expression.hpp"
#include "InLimitMgr.hpp"
#include "TimeDepAttrs.hpp"
#include "ChildAttrs.hpp"
#include "MiscAttrs.hpp"
#include "NodeFwd.hpp"
#include "Flag.hpp"

namespace ecf { class Simulator; class SimulatorVisitor; class DefsAnalyserVisitor; class FlatAnalyserVisitor; } // forward declare for friendship
namespace ecf { class Calendar; class NodeTreeVisitor; } // forward declare class

class Node : public boost::enable_shared_from_this<Node>, private boost::noncopyable {
protected:
   Node(const std::string& name);
   Node();
public:
   virtual ~Node();

   /// The Parent Must set the parent pointer. For a Suite however this will be NULL
   void set_parent(Node* p) { parent_ = p; }

   static bool doDelete(Node*);

   // Server called functions:
   /// Required when we have time attributes, when time related attribute are free they stay free
   virtual void calendarChanged(const ecf::Calendar&, std::vector<node_ptr>& auto_cancelled_nodes);

   /// resolving dependencies means we look at day,date,time and triggers and check to
   /// to see if a node is free or still holding. When a node if free of its dependencies and limits
   /// Its state is changed to submitted. When a task is in a the submitted state its
   /// associated ecf file can be submitted.
   /// ************************************************************************************
   /// There is no point in resolving child dependencies if the parent is not FREE
   //  We will return a bool, true means we are free, false for holding
   // *************************************************************************************
   virtual bool resolveDependencies(JobsParam& jobsParam);

   /// Command related functions:
   /// suspend generation of jobs. Below this node.
   /// Note: When a node is suspended. time/date dependencies are still handled
   ///       A node which is free of time is marked.
   /// Note: suspended is *NOT* a node state. It just an *attribute*. NState::State
   /// represents all the life cycle change a Node can go through. Suspended does
   /// not real fit into this and has been separated out.
   bool isSuspended() const { return suspended_; }
   bool isParentSuspended() const;
   void suspend();

   /// resume generation of jobs, and kick of an immediate job generation
   /// If not previously suspended does nothing
   void resume();

   /// Kill the task if it is active. For NodeContainers do it hierarchically
   /// will throw std::runtime_error for any errors
   virtual void kill(const std::string& zombie_pid = "") = 0;

   /// Show status of a node. For NodeContainers do it hierarchically
   /// will throw std::runtime_error for any errors
   virtual void status() = 0;

   /// Order the node using the second parameter
   virtual void order(Node*/*immediateChild*/, NOrder::Order) {}

   /// For suites it allows dependencies to be resolved, and changes state to defStatus
   virtual void begin();

   /// re queue this node. States are reset to defStatus
   /// Typically resetRepeats is set to true, when called from user command
   /// or when re-queue is called from NodeContainer/parent
   ///
   /// With user interaction : we need to clear the suspended state for *child* nodes
   /// To distinguish between child and parent, we use a integer 'level'
   /// This was done so that if user had suspended a task deep in the hierarchy. (which is then
   /// not displayed in the GUI), and then chooses to re-queue at a *high* level,
   /// we need to clear child suspended state.(Principle of least surprise)
   /// However we will preserve def status.
   /// If a user re-queues a node that is suspended then it stays suspended
   /// We use -1 to mean leave suspended state as is
   virtual void requeue(bool resetRepeats, int clear_suspended_in_child_nodes);

   /// Set a flag that ensures that a node with a single time dependency is NOT automatically re-queued
   /// This is applied *up* the hierarchy until we *hit* a node with a repeat or Cron attribute
   /// This is done since repeat and cron can be used to reset the NO_REQUE_IF_SINGLE_TIME_DEP flags
   /// This functionality is only required during interactive force or run
   void set_no_requeue_if_single_time_dependency();

   /// Recursively run the tasks under this node, ignore suspend,limits,triggers, and time dependencies
   /// if force is set, run even if task is submitted or active. (will create zombies)
   virtual bool run(JobsParam& jobsParam, bool force) = 0;

   /// Recursively determines why the node is not running.
   virtual void top_down_why(std::vector<std::string>& theReasonWhy) const;
   void bottom_up_why(std::vector<std::string>& theReasonWhy) const;

   void freeTrigger() const;
   void clearTrigger() const;
   void freeComplete() const;
   void clearComplete() const;
   void freeHoldingDateDependencies();
   void freeHoldingTimeDependencies();

   // Used in the force cmd
   bool set_event( const std::string& event_name_or_number);
   bool clear_event( const std::string& event_name_or_number);
   void setRepeatToLastValue();
   virtual void setRepeatToLastValueHierarchically() { setRepeatToLastValue(); }

   /// find all %VAR% and replaces with variable values, returns false on the
   /// first variable that can't be found, cmd will be left half processed.
   /// Will search for ECF_MICRO, if not found assumes % as the micro char
   bool variableSubsitution(std::string& cmd) const;

   bool variable_substitution(std::string& cmd, const NameValueMap& user_edit_variables, char micro = '%') const;

   /// Find all %VAR% and add to the list, there can be more than one. i.e %ECF_FILES% -I %ECF_INCLUDE%"
   bool find_all_used_variables(std::string& cmd, NameValueMap& used_variables, char micro = '%') const;

   /// Find all environment variables, in the input string and substitute.
   /// with correspondingly named variable value.
   /// i.e search for ${ENV} and replace
   bool enviromentSubsitution(std::string& cmd);

   /// Resolve inlimit references to limits, and check trigger and complete expression
   virtual bool check(std::string& errorMsg,std::string& warningMsg) const;

   /// Generated variables. Suites can have thousands of tasks. If the generated variables associated with
   /// tasks are sent to the client, it can amount to a very large network traffic.
   /// To minimise this bandwidth the generated variables for tasks/families are not persisted.
   /// However client can demand create the generated  variable by calling this function.
   virtual void update_generated_variables() const = 0;

   ///  generates job file independent of dependencies
   virtual void check_job_creation( job_creation_ctrl_ptr jobCtrl) = 0;

   node_ptr remove(); // gets the parent then calls removeChild
   virtual node_ptr removeChild( Node* child) = 0;
   virtual bool addChild( node_ptr child,size_t position = std::numeric_limits<std::size_t>::max()) = 0; // return false if child of same name exist, leak!!!
   virtual bool isAddChildOk( Node* child, std::string& errorMsg) const = 0; // return false if child of same name

   virtual void verification(std::string& errorMsg) const;

   /// See defs.hpp
   virtual void generate_scripts( const std::map<std::string,std::string>& override) const = 0;

   // standard functions: ==============================================
   virtual std::ostream& print(std::ostream&) const;
   bool operator==(const Node& rhs) const;
   virtual bool checkInvariants(std::string& errorMsg) const;

   /// Implements the visitor pattren
   virtual void accept(ecf::NodeTreeVisitor&) = 0;
   virtual void acceptVisitTraversor(ecf::NodeTreeVisitor&) = 0; // Visitor does the traversal

// state related functions: ========================================

   /// If the task was aborted provide the reason, default returns empty string
   /// Only the task will return the reason for abort.
   virtual const std::string& abortedReason() const;

   /// This state added as an convenience, it includes Suspended attribute returned as enum
   DState:: State dstate() const;

   /// This represents the persisted/saved state // First = NState, Second = time_duration
   /// The State represent the life cycle changes of a node.
   NState::State state() const { return state_.first.state(); }

   /// return the state and duration time when the state change happened
   std::pair<NState,boost::posix_time::time_duration> get_state() const { return state_;}

   /// Set the state, this can have side affects. To handle state changes
   ///	  Should family triggers use saved state, or computed state.
   ///	  *** If we use computed state, this may be wrong, since computed state
   ///	  *** does not take repeats, or time dependencies into account
   ///	  *** Hence after each state change, we bubble up node tree, work out if
   ///	  *** Node is free of repeat, and time dependencies,
   /// 	** Every time we set the state on a nodecontainer, we call handleStateChange
   /// 	** This by default works out the most significant state of the children
   /// 	** ie. the computed state. Hence setting the state on Suite/Family is really
   /// 	** meaningless, since it will always be the computed state.
   void set_state(NState::State s, bool force = false);
   virtual void set_state_hierarchically(NState::State s, bool force) { set_state(s,force); }

   /// Set state only, has no side effects
   void setStateOnly(NState::State s, bool force = false);
   virtual void setStateOnlyHierarchically(NState::State s, bool force = false) { setStateOnly(s,force); }

   /// Sets the default status the node should have when the begin/re-queue is called
   /// *Distinguish* between adding a def status and changing it.
   /// Changing via defStatus_.setState(s);  should alter state_change_no
   void addDefStatus(DState::State s ) { defStatus_ = DState(s); }
   DState::State defStatus() const { return defStatus_.state(); }

   // Query functions: =========================================================
   /// returns my parent, for suite will return NULL;
   Node* parent() const { return parent_;}
   virtual Suite* suite() const = 0;
   virtual Defs* defs() const = 0;

   // Performance hack, to avoid casts
   virtual Task* isTask() const   { return NULL;}
   virtual Alias* isAlias() const { return NULL;}
   virtual Submittable* isSubmittable() const { return NULL;}
   virtual NodeContainer* isNodeContainer() const { return NULL;}
   virtual Family* isFamily() const { return NULL;}
   virtual Suite* isSuite() const  { return NULL;}

   ///  returns the absolute node path
   std::string absNodePath() const;
   virtual const std::string& debugType() const = 0;

   /// returns abs node path preceded by the type of the node
   std::string debugNodePath() const;

   /// returns true if this node OR any of its children
   /// has cron,time,day,date or today time dependencies
   virtual bool hasTimeDependencies() const { return (time_dep_attrs_) ? true : false; }


   /// A hierarchical function
   virtual bool hasAutoCancel() const { return (autoCancel_) ? true : false;}

   // Access functions: ======================================================
   const std::string& name() const { return name_; }
   std::string toString() const    { return name_; }  // keep ecflowview/ecf_node.h happy
   const Repeat& repeat() const    { return repeat_;} // can be empty()
   const std::vector<Variable>&        variables()const { return varVec_;}
   const std::vector<limit_ptr>&       limits()   const { return limitVec_;}
   const std::vector<InLimit>&         inlimits() const { return inLimitMgr_.inlimits(); }
   const std::vector<Meter>&           meters()    const;
   const std::vector<Event>&           events()    const;
   const std::vector<Label>&           labels()   const;
   const std::vector<ecf::TimeAttr>&   timeVec()  const;
   const std::vector<ecf::TodayAttr>&  todayVec() const;
   const std::vector<DateAttr>&        dates()    const;
   const std::vector<DayAttr>&         days()     const;
   const std::vector<ecf::CronAttr>&   crons()    const;
   const std::vector<VerifyAttr>&      verifys()  const;
   const std::vector<ZombieAttr>&      zombies()  const;
   ecf::LateAttr* get_late() const { return lateAttr_;}
   ecf::AutoCancelAttr*  get_autocancel() const { return autoCancel_;}
   ecf::Flag&       flag()           { return flag_;}
   const ecf::Flag& get_flag() const { return flag_;}

   virtual void gen_variables(std::vector<Variable>&) const;
   bool getLabelValue(const std::string& name, std::string& value) const;

   // Use get_trigger()/get_complete() for determining if we have trigger
   // and complete expressions. This is many times faster than calling
   // triggerAst()/completeAst() as this will force a parse and construction
   // of Abstract syntax tree, first time it is called.
   Expression* get_trigger()    const { return triggerExpr_;}
   Expression* get_complete()   const { return completeExpr_;}
   AstTop* completeAst() const;   // Will create AST on demand
   AstTop* triggerAst() const;    // Will create AST on demand
   std::string completeExpression() const;
   std::string triggerExpression() const;

   virtual void get_all_tasks(std::vector<task_ptr>&) const = 0;
   virtual void get_all_nodes(std::vector<node_ptr>&) const = 0;
   virtual void get_all_aliases(std::vector<alias_ptr>&) const = 0;
   virtual void getAllTasks(std::vector<Task*>&) const = 0;
   virtual void getAllSubmittables(std::vector<Submittable*>&) const = 0;
   virtual void get_all_active_submittables(std::vector<Submittable*>&) const = 0;
   virtual void getAllNodes(std::vector<Node*>&) const = 0;
   virtual void getAllAstNodes(std::set<Node*>&) const;

   /// returns the immediate children
   virtual void immediateChildren(std::vector<node_ptr>&) const {}

   /// retrieve _ALL_ children by hierarchically traversing down the node tree
   virtual void allChildren(std::set<Node*>&) const {}

   // Add functions: ===============================================================
   void addVerify( const VerifyAttr& );  // for testing and verification Can throw std::runtime_error
   void addVariable(const Variable& );   // will throw std::runtime_error if duplicate
   void add_variable(const std::string& name, const std::string& value );// will write to std:out if duplicates
   void add_variable_int(const std::string& name, int);// will throw std::runtime_error if duplicate

   void add_trigger(const std::string&);            // use for short complete expressions,Can throw std::runtime_error
   void add_complete(const std::string&);           // use for short complete expressions,Can throw std::runtime_error
   void add_trigger_expr(const Expression&);        // Can throw std::runtime_error
   void add_complete_expr(const Expression&);       // Can throw std::runtime_error
   void add_part_trigger(const PartExpression& );   // for adding multiple and/or expression,Can throw std::runtime_error
   void add_part_complete(const PartExpression& );  // for adding multiple and/or expression,Can throw std::runtime_error

   void addTime( const ecf::TimeAttr& );
   void addToday( const ecf::TodayAttr& );
   void addDate( const DateAttr& );
   void addDay( const DayAttr& );
   void addCron( const ecf::CronAttr& );

   void addLimit(const Limit& );        // will throw std::runtime_error if duplicate
   void addInLimit(const InLimit& l)         { inLimitMgr_.addInLimit(l);}   // will throw std::runtime_error if duplicate
   void auto_add_inlimit_externs(Defs* defs) { inLimitMgr_.auto_add_inlimit_externs(defs);}
   void addEvent( const Event& );       // will throw std::runtime_error if duplicate
   void addMeter( const Meter& );       // will throw std::runtime_error if duplicate
   void addLabel( const Label& );       // will throw std::runtime_error if duplicate
   void addAutoCancel( const ecf::AutoCancelAttr& );
   void addLate( const ecf::LateAttr& );
   void addRepeat( const Repeat& );      // will throw std::runtime_error if duplicate
   void addZombie( const ZombieAttr& );  // will throw std::runtime_error if duplicate


   // Delete functions: can throw std::runtime_error ===================================
   // if name argument is empty, delete all attributes of that type
   // Can throw std::runtime_error of the attribute can not be found
   void deleteTime(const std::string& name );
   void delete_time( const ecf::TimeAttr&  );
   void deleteToday(const std::string& name);
   void delete_today(const ecf::TodayAttr&);
   void deleteDate(const std::string& name);
   void delete_date(const DateAttr&);
   void deleteDay(const std::string& name);
   void delete_day(const DayAttr&);
   void deleteCron(const std::string& name);
   void delete_cron(const ecf::CronAttr&);

   void delete_zombie(const ecf::Child::ZombieType);
   void deleteVariable( const std::string& name);
   void deleteEvent(const std::string& name);
   void deleteMeter(const std::string& name);
   void deleteLabel(const std::string& name);
   void deleteTrigger();
   void deleteComplete();
   void deleteRepeat();
   void deleteLimit(const std::string& name);
   void delete_limit_path(const std::string& limit_name, const std::string& limit_path);
   void deleteInlimit(const std::string& name);
   void deleteZombie(const std::string& type); // string must be one of [ user | ecf | path ]

   // Change functions: ================================================================
   /// returns true the change was made else false, Can throw std::runtime_error for parse errors
   void changeVariable(const std::string& name,const std::string& value);
   void changeClockType(const std::string& theType);
   void changeClockDate(const std::string& theDate);
   void changeClockGain(const std::string& theIntGain);
   void changeEvent(const std::string& name,const std::string& setOrClear = "");
   void changeEvent(const std::string& name,bool value);
   void changeMeter(const std::string& name,const std::string& value);
   void changeMeter(const std::string& name,int value);
   void changeLabel(const std::string& name,const std::string& value);
   void changeTrigger(const std::string& expression);
   void changeComplete(const std::string& expression);
   void changeRepeat(const std::string& value);
   void changeLimitMax(const std::string& name,const std::string& maxValue);
   void changeLimitMax(const std::string& name,int maxValue);
   void changeLimitValue(const std::string& name,const std::string& value);
   void changeLimitValue(const std::string& name,int value);
   void changeDefstatus(const std::string& state);

   bool set_meter(const std::string& name,int value); // does not throw if meter not found
   bool set_event(const std::string& name,bool value);  // does not throw if event not found

   void increment_repeat(); // used in test only

   // mementos functions:
   /// Collect all the state changes, so that only small subset is returned to client
   virtual void collateChanges(DefsDelta& ) const = 0;
   void incremental_changes(DefsDelta&, compound_memento_ptr& comp) const;

   void set_memento(const StateMemento* );
   void set_memento(const NodeDefStatusDeltaMemento* );
   void set_memento(const SuspendedMemento* );
   void set_memento(const NodeEventMemento* );
   void set_memento(const NodeMeterMemento* );
   void set_memento(const NodeLabelMemento* );
   void set_memento(const NodeTriggerMemento* );
   void set_memento(const NodeCompleteMemento* );
   void set_memento(const NodeRepeatMemento* );
   void set_memento(const NodeLimitMemento* );
   void set_memento(const NodeInLimitMemento* );
   void set_memento(const NodeVariableMemento* );
   void set_memento(const NodeLateMemento* );
   void set_memento(const NodeTodayMemento* );
   void set_memento(const NodeTimeMemento* );
   void set_memento(const NodeDayMemento* );
   void set_memento(const NodeCronMemento* );
   void set_memento(const NodeDateMemento* );
   void set_memento(const NodeZombieMemento* );
   void set_memento(const NodeVerifyMemento* );
   void set_memento(const FlagMemento* );

   // Find functions: ============================================================
   // Will search for a node by name(ie not a path) first on siblings, then on a parent
   // then up the node tree, will stop at the suite .
   virtual node_ptr find_node_up_the_tree(const std::string& name) const = 0;

   // This is used to find relative nodes.
   virtual node_ptr find_relative_node(const std::vector<std::string>& pathToNode) = 0;

   /// Look for user,generated and repeat variables
   /// Find variable corresponding to the given name, by search up the parent hierarchy
   /// ** We need to distinguish between a variable the exists, but has an empty value
   /// ** hence we return true if variable is found, and false otherwise
   bool findParentVariableValue(const std::string& name, std::string& theValue) const;

   /// Look for the parent generated variable only
   bool find_parent_gen_variable_value(const std::string& name, std::string& theValue) const;

   /// Only looks at user variables
   /// Find variable corresponding to the given name, by search up the parent hierarchy
   /// Use when we know that variable is user defined. This is more efficient than
   /// calling findParentVariableValue.
   /// *** We need to distinguish between a variable the exists, but has an empty value
   /// *** hence we return true if variable is found, and false otherwise
   bool findParentUserVariableValue(const std::string& name, std::string& theValue) const;

   /// This function should be used, when we don't care about the distinctions between
   /// a variable that exist but has a empty value, and variable not found.(still return empty string)
   /// Useful when we want to return by reference
   const std::string& find_parent_user_variable_value(const std::string& name) const;

   virtual node_ptr findImmediateChild(const std::string& /*name*/, size_t& /*child_pos*/) const { return node_ptr();}
   const Variable& findVariable(const std::string& name) const;
   virtual const Variable& findGenVariable(const std::string& name) const;
   bool findVariableValue( const std::string& name, std::string& returnedValue) const;
   bool findGenVariableValue( const std::string& name, std::string& returnedValue) const;

   bool findVerify(const VerifyAttr& ) const;
   bool findLimit(const Limit& ) const;
   bool findLabel(const std::string& name) const;
   const Label& find_label(const std::string& name) const;
   limit_ptr find_limit(const std::string& name) const;
   limit_ptr findLimitUpNodeTree(const std::string& name) const;
   Limit* findLimitViaInLimit(const InLimit& l) const { return  inLimitMgr_.findLimitViaInLimit(l); }         // used in *test* only
   bool findInLimitByNameAndPath(const InLimit& l) const { return inLimitMgr_.findInLimitByNameAndPath(l); }  // use name,path,token,
   const Repeat& findRepeat(const std::string& name) const;
   const Meter& findMeter(const std::string& name) const;
   Meter& find_meter(const std::string& name);
   const Event& findEvent(const Event&) const;
   const Event& findEventByNameOrNumber( const std::string& name) const;

   const ZombieAttr& findZombie( ecf::Child::ZombieType ) const;
   bool findParentZombie(ecf::Child::ZombieType, ZombieAttr&) const;

   /// Finds the referenced node. The node path can be relative or absolute or a extern path
   /// however if its an extern path, and corresponding suite is loaded, but we still
   /// can't find the path, then an error is returned
   node_ptr findReferencedNode(const std::string& nodePath, std::string& errorMsg) const;
   node_ptr findReferencedNode(const std::string& nodePath, const std::string& externObj, std::string& errorMsg) const;

   /// return true if we can find a event, meter, user, repeat or generated variable with the given name
   bool findExprVariable( const std::string& name);  // update event & meter as used in trigger for simulator

   /// The status of family/suite is the inherited most significant status of all its children
   enum TraverseType { IMMEDIATE_CHILDREN, HIERARCHICAL };
   virtual NState::State computedState(Node::TraverseType) const = 0;

   /// Sets the most significant state up the node tree. Ignores tasks
   void set_most_significant_state_up_node_tree();

   /// For use with GUI only
   void set_graphic_ptr(void* p) { graphic_ptr_ = p; }
   void* graphic_ptr() const { return  graphic_ptr_;}

   /// returns the position of this node relative to its peers
   /// If not attached to parent returns std::numeric_limits<std::size_t>::max();
   size_t position() const;

protected:
   /// Used in conjunction with Node::position()
   /// returns std::numeric_limits<std::size_t>::max() if child not found
   virtual size_t child_position(const Node*) const = 0;

   /// The set_state_only() requires a correctly formed tree, ie since it needs suite()/calendar
   /// to initialise the duration. We need a way set the state directly. For initialization
   void set_state_only(NState::State s) { state_.first.setState(s);}

   /// based on the *current* state increment or decrements the limits
   /// Should *only* be called within a task
   virtual void update_limits() = 0;

   /// After job submission we need to increment the in limit, to indicate that a
   /// resource is consumed. The set ensure we only update once during a traversal
   void incrementInLimit(std::set<Limit*>& limitSet) const;

   /// After job aborts or completes we need to decrement the in limit, to indicate that
   /// additional resource is available. The set ensure we only update once during a traversal
   void decrementInLimit(std::set<Limit*>& limitSet) const;

   friend class InLimitMgr;
   bool check_in_limit() const { return inLimitMgr_.inLimit(); }
   bool check_in_limit_up_node_tree() const;


   friend class Defs;
   friend class Family;
   friend class NodeContainer;
   virtual bool doDeleteChild(Node* child) { return false;}

   /// called at the end of state change function
   /// ** Every time we set the state on a nodecontainer, we call handleStateChange
   /// ** This by default works out the most significant state of the children
   /// ** ie. the computed state. Hence setting the state on Suite/Family is really
   /// ** meaningless, since it will always be the computed state.
   /// ** For Aliases we only update the limits, and do not bubble up state changes
   virtual void handleStateChange() = 0; // can end up changing state

   /// This function is called as a part of handling state change.
   /// When a suite completes it can be re-queued due to:
   ///   o repeat attribute
   ///     When going up the hierarchy we should not reset Repeats
   ///     The inner repeat must complete before parent repeat (if any) is incremented
   ///     (i.e Mimics a nested loops)
   ///  o Time, Today, cron attributes
   ///
   ///  Otherwise we need to traverse up the node tree and set the most significant state
   void requeueOrSetMostSignificantStateUpNodeTree();
   virtual void resetRelativeDuration();

   node_ptr non_const_this() const;

   void update_repeat_genvar() const;

   // returns node state without trailing new lines
   virtual std::string write_state() const;
   virtual void read_state(const std::string& line,const std::vector<std::string>& lineTokens);

private:
   void why(std::vector<std::string>& theReasonWhy) const;
   /// Function used as a part of trigger and complete expressions.
   /// The search pattern is event,meter,user-variable,repeat, generated-variable
   int findExprVariableValue( const std::string& name) const;
   int findExprVariableValueAndPlus(const std::string& name, int val) const;
   int findExprVariableValueAndMinus(const std::string& name, int val) const;
   int findExprVariableValueAndType( const std::string& name, std::string& varType) const;
   void findExprVariableAndPrint( const std::string& name, std::ostream& os) const;
   friend class VariableHelper;

private:
   bool checkForAutoCancel(const ecf::Calendar& c) const;

   void add_trigger_expression(const Expression&);     // Can throw std::runtime_error
   void add_complete_expression(const Expression&);    // Can throw std::runtime_error
   const Event& findEventByNumber(int number) const;
   const Event& findEventByName( const std::string& name) const;
   bool set_meter_used_in_trigger(const std::string& name);
   bool set_event_used_in_trigger(const std::string& name);


   /// When the begin/re-queue is called this function will initialise the state
   /// on the node. If node has a default state this is applied to the node, and
   /// hierarchically to all the children
   /// Can also clear suspended see re-queue()
   void initState(int clear_suspended_in_child_nodes);

   // Clear the node suspended and update state change number, no other side effects
   void clearSuspended();

private: // alow simulator access
   friend class ecf::DefsAnalyserVisitor;
   friend class ecf::FlatAnalyserVisitor;
   friend class ecf::SimulatorVisitor;
   friend class ecf::Simulator;
   std::vector<Meter>&  ref_meters();// allow simulator set meter value
   std::vector<Event>&  ref_events();// allow simulator set event value
   Repeat& ref_repeat()              { return repeat_;} // allow simulator to modify repeat

   /// Note: If the complete expression evaluation fails. we should continue resolving dependencies
   ///       If the complete expression evaluation evaluates, then we set node to complete
   bool evaluateComplete() const;
   bool evaluateTrigger() const;
   bool timeDependenciesFree() const;

   AstTop* completeAst(std::string& errorMsg) const;   // Will create AST on demand
   AstTop* triggerAst(std::string& errorMsg) const;    // Will create AST on demand

private: // All mementos access
   friend class CompoundMemento;
   void clear(); /// Clear *ALL* internal attributes

private: /// For use by python interface,
   friend void export_Node();
   friend void export_Task();
   friend void export_SuiteAndFamily();
   std::vector<Meter>::const_iterator meter_begin() const;
   std::vector<Meter>::const_iterator meter_end() const;
   std::vector<Event>::const_iterator event_begin() const;
   std::vector<Event>::const_iterator event_end() const;
   std::vector<Label>::const_iterator label_begin() const;
   std::vector<Label>::const_iterator label_end() const;
   std::vector<ecf::TimeAttr>::const_iterator time_begin() const;
   std::vector<ecf::TimeAttr>::const_iterator time_end() const;
   std::vector<ecf::TodayAttr>::const_iterator today_begin() const;
   std::vector<ecf::TodayAttr>::const_iterator today_end() const ;
   std::vector<DateAttr>::const_iterator date_begin() const ;
   std::vector<DateAttr>::const_iterator date_end() const;
   std::vector<DayAttr>::const_iterator day_begin() const ;
   std::vector<DayAttr>::const_iterator day_end() const;
   std::vector<ecf::CronAttr>::const_iterator cron_begin() const ;
   std::vector<ecf::CronAttr>::const_iterator cron_end() const;
   std::vector<ZombieAttr>::const_iterator zombie_begin() const;
   std::vector<ZombieAttr>::const_iterator zombie_end() const;
   std::vector<VerifyAttr>::const_iterator verify_begin() const;
   std::vector<VerifyAttr>::const_iterator verify_end() const;
   std::vector<Variable>::const_iterator variable_begin() const { return varVec_.begin();}
   std::vector<Variable>::const_iterator variable_end() const { return varVec_.end();}
   std::vector<limit_ptr>::const_iterator limit_begin() const { return limitVec_.begin();}
   std::vector<limit_ptr>::const_iterator limit_end() const { return limitVec_.end();}
   std::vector<InLimit>::const_iterator inlimit_begin() const { return inLimitMgr_.inlimit_begin();}
   std::vector<InLimit>::const_iterator inlimit_end() const { return inLimitMgr_.inlimit_end();}
   std::string to_string() const;                                  // For python interface

private:
   Node*        parent_; // *NOT* persisted must be set by the parent class
   std::string  name_;
   bool                        suspended_;
   std::pair<NState,boost::posix_time::time_duration> state_; // state and duration since suite start when state changed
   DState                      defStatus_;    // default value is QUEUED

   mutable Expression*         completeExpr_; // can only have one complete expression
   mutable Expression*         triggerExpr_;  // can only have one trigger expression

   ecf::LateAttr*              lateAttr_;     // Can only have one late attribute per node
   ecf::AutoCancelAttr*        autoCancel_;   // Can only have 1 auto cancel per node
   TimeDepAttrs*               time_dep_attrs_;
   ChildAttrs*                 child_attrs_;  // event meter & lables
   MiscAttrs*                  misc_attrs_;   // VerifyAttr(used for statistics and test verification) & Zombies
   Repeat                      repeat_;       // each node can only have one repeat. By value, since has pimpl

   std::vector<Variable>       varVec_;
   std::vector<limit_ptr>      limitVec_;    // Ptrs since many in-limits can point to a single limit
   InLimitMgr                  inLimitMgr_;  // manages the inlimit

   ecf::Flag                   flag_;

   unsigned int state_change_no_;     // *not* persisted, only used on server side,Used to indicate addition or deletion of attribute
   unsigned int variable_change_no_;  // *not* persisted, placed here rather than Variable, to save memory
   unsigned int suspended_change_no_; // *not* persisted,

#ifdef DEBUG
   boost::posix_time::ptime  submit_to_complete_duration_;  // *not* persisted
#endif

   void* graphic_ptr_;  // for use with the gui only

   friend class TimeDepAttrs;
   friend class ChildAttrs;
   friend class MiscAttrs;

private:
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & name_;
      ar & state_;
      ar & suspended_;
      ar & defStatus_;
      ar & completeExpr_;
      ar & triggerExpr_;
      ar & lateAttr_;
      ar & autoCancel_;
      ar & time_dep_attrs_;
      ar & child_attrs_;
      ar & misc_attrs_;    // VerifyAttr & Zombies
      ar & repeat_;
      ar & varVec_;
      ar & limitVec_;
      ar & inLimitMgr_;
      ar & flag_;

      if (Archive::is_loading::value) {
         if (time_dep_attrs_)  time_dep_attrs_->set_node(this);
         if (child_attrs_) child_attrs_->set_node(this);
         if (misc_attrs_) misc_attrs_->set_node(this);
         for(std::vector<limit_ptr>::iterator i = limitVec_.begin(); i!= limitVec_.end(); i++)  (*i)->set_node(this);
      }
   }
};
#endif
