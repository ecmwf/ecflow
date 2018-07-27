#ifndef DEFS_HPP_
#define DEFS_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #165 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
// class defs: The root of the node tree. Holds all the suites:
// Externs are not persisted, why ?:
//   o Externs are un-resolved references to node paths in trigger expressions and inlimits
//     These references can be dynamically generated.
//   o Saves on network bandwidth and checkpoint file size
//
// Hence externs are *ONLY* used on the client side.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <ostream>
#include <set>

#include <boost/noncopyable.hpp>
#include "boost/date_time/posix_time/posix_time_types.hpp"
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>         // no need to include <vector>
#include <boost/serialization/deque.hpp>          // no need to include <deque>
#include <boost/serialization/map.hpp>            // no need to include <map>
#include <boost/serialization/string.hpp>         // no need to include <string>
#include <boost/serialization/shared_ptr.hpp>     // no need to include shared_ptr
#include <boost/foreach.hpp> // used so often just placed here for convenience

#include "ServerState.hpp"
#include "ClientSuiteMgr.hpp"
#include "NState.hpp"
#include "NOrder.hpp"
#include "NodeFwd.hpp"
#include "JobCreationCtrl.hpp"
#include "Suite.hpp"
#include "CheckPt.hpp"
#include "Archive.hpp"
#include "Attr.hpp"
#include "PrintStyle.hpp"

class Limit;
class AbstractObserver;
namespace ecf { class NodeTreeVisitor; class CalendarUpdateParams; } // forward declare

class Defs {
public:
   static defs_ptr create();
   static defs_ptr create(const std::string& port);
   Defs();
   explicit Defs(const std::string& port); // used in test, to initialise server variables
   Defs(const Defs&);
   Defs& operator=(const Defs&);

   ~Defs();

   void copy_defs_state_only(defs_ptr defs); // needed when creating defs for client handles
   bool operator==(const Defs& rhs) const;
   std::ostream& print(std::ostream&) const ;

   /// State related functions:
   /// Defs acts like the root node.
   void set_state(NState::State);
   void set_state_only(NState::State);
   NState::State state() const;
   void set_most_significant_state();

   // The defs does not need suspend()/resume() as this is controlled by the server states
   // The defs can not schedule jobs when HALTED or SHUTDOWN, i.e. not RUNNING, this is same as suspended
   bool isSuspended() const { return (server().get_state() != SState::RUNNING); }

   /// Python based checking
   /// Will create a temporary variable ECF_JOB so the job generation is done to a temporary directory
   /// For TEST ONLY. Will generated jobs from the given node path
   /// if the no path defined will generate jobs for all tasks
   /// Job generation here is INDEPENDENT of the dependencies
   /// Will call begin(). this updated generated variables which are used
   /// **during variable substitution in job generation.
   /// Note: All Tasks that fail to create jobs are returned in jobCtrl
   /// This is because some Tasks are dummy, they have no associated ecf.
   /// since they will never be run. This can be determined if they have
   /// trigger expression like "1 == 0"
   void check_job_creation( job_creation_ctrl_ptr jobCtrl);

    /// Automatically generated '.ecf' scripts for this definition
    /// It uses the contents of the definition to parameterise what gets
    /// generated, and the location of the files. Will throw std::runtime_error for errors
    /// Assumes: ECF_HOME specified and accessible for all Tasks, otherwise std::runtime_error is raised
    /// If ECF_FILES is specified, then scripts are generated under this directory
    /// otherwise ECF_HOME is used. The missing directories are automatically added.
    /// IF ECF_INCLUDE must be specified.The head.h and tail.h includes will
    /// use angle brackets, i.e %include <head.h>.
    /// If ECF_CLIENT_EXE_PATH variable is added, child command will use this, otherwise
    /// will use ecflow_client, and assume this accessible on the path.
    /// Will not generated scripts for tasks with ECF_DUMMY_TASK specified.
   void generate_scripts() const;

   /// Update calendar and time dependent variables. This must be called by the polling mechanism,
   /// This will *ONLY* have an effect after begin() command has been called
   ///
   /// Some suites can elect to ignore calendar updates when the server is not running
   /// This allows normal and relative time dependencies to be honoured, even if the
   /// server is started/stopped many times.
   /// Updating the calendar can lead to state changes in the server. i.e when time related
   /// dependencies are free'd
   void updateCalendar( const ecf::CalendarUpdateParams &);

   /// Used by simulator
   void update_calendar(suite_ptr suite, const ecf::CalendarUpdateParams& cal_update_params ); // used by simulator

   /// returns the number of times the calendar has been updated. For DEBUG
   unsigned int updateCalendarCount() const { return updateCalendarCount_;}

   // Implements visitor pattern
   void accept(ecf::NodeTreeVisitor&); // Node Tree structure does the traversal
   void acceptVisitTraversor(ecf::NodeTreeVisitor& v); // visitor does traversal

   /// Moves all the data from input defs into this definition. By default
   /// suite of the same name, are left alone, unless the force option is used.
   /// i.e with force any suites of the same name will overwrite the suite of
   /// the same name in this definition.
   /// externs and server user variables are appended to.
   /// The input defs will be left with NO suites
   void absorb(Defs*, bool force);

   std::string name() const { return "/";} /* ABO */

   /// Add a suite to the definition, will throw std::runtime_error if duplicate
   suite_ptr add_suite(const std::string& name);
   void addSuite(suite_ptr,size_t position = std::numeric_limits<std::size_t>::max());
   size_t child_position(const Node*) const;

   /// Externs refer to Nodes or, variable, events, meter, repeat, or generated variable
   /// not currently defined. If the object(variable, event,meter, repeat, or generate variable name)
   /// is empty the path can refer to a Task, Family of Suite
   /// extern can have absolute and relative paths
   /// Typically used in a trigger or complete expression
   /// Path can be off the form:
   ///  extern /suite/family
   ///  extern /suite/family:repeat_name
   ///  extern ../family:repeat_name
   void add_extern(const std::string& nodePath );
   void clear_externs() { externs_.clear();}

   /// Scan the trigger and complete expressions, and automatically add extern's
   /// i.e where the node paths, and references, to event, meters, edit and repeat variables,
   /// don't exist, in this defs.
   void auto_add_externs(bool remove_existing_externs_first = true);


   /// Flag the chosen suite, so that it can resolve dependencies. This also
   /// changes the state of all children to QUEUED/defstatus and initialise the node attributes
   /// Once a suite is begun, it stays in that state
   void beginSuite(suite_ptr);

   /// Enables all suites to resolve dependencies
   /// Once a suite is begun, it stays in that state
   void beginAll();

   /// Reset the begin state. **** ONLY to be used for test ********
   void reset_begin();

   /// throws runtime_error if suite cant begin
   void check_suite_can_begin(suite_ptr) const;

   /// Will requeue all suites. Current used in test only
   void requeue();

   /// returns true if defs has cron,time,day,date or today time dependencies
   bool hasTimeDependencies() const;

   /// recursively sort the attributes
   // expect one attr to be [ event | meter | label | limits | variable ]
   void sort_attributes(ecf::Attr::Type attr, bool recursive = true);

   /// This function is called when ALL definition has been parsed successfully
   /// Client Side:: The client side has externs, hence any references to node paths
   ///               in the triggers expression, that are un-resolved and not in
   ///               the extern;s are reported as errors/warnings
   ///               Likewise for inlimit references to Limits
   /// Server Side:: There are no externs hence, check will report all unresolved
   ///               references as errors.
   /// Note the AST will be demand loaded in the server. Since they are not persisted.
   /// we will also resolve inLimit references to Limits. Note inlimit may also
   /// reference paths, which are externs::
   // ****** Spirit based AST construction is very expensive  ********
   // ****** Need to replace at some point in future          ********
   bool check(std::string& errorMsg,std::string& warningMsg) const;

   /// Assumes input argument is of the form /suite/family/task, /suite/family/family/task
   node_ptr findAbsNode(const std::string& pathToNode) const;
   bool find_extern( const std::string& pathToNode,const std::string& node_attr_name) const;
   suite_ptr findSuite(const std::string& name) const;
   const std::vector<suite_ptr>& suiteVec() const { return suiteVec_;}

   /// Given a path, /suite/family/task, find node which is the closest
   node_ptr find_closest_matching_node(const std::string& pathToNode) const;

   void getAllFamilies(std::vector<Family*>&) const;
   void getAllNodes(std::vector<Node*>&) const;
   void getAllTasks(std::vector<Task*>&) const;
   void getAllSubmittables(std::vector<Submittable*>&) const;
   void get_all_active_submittables(std::vector<Submittable*>&) const;
   void get_all_nodes(std::vector<node_ptr>&) const;
   void get_all_tasks(std::vector<task_ptr>&) const;
   void get_all_aliases(std::vector<alias_ptr>&) const;
   void getAllAstNodes(std::set<Node*>&) const;
   const std::set<std::string>& externs() const { return externs_; }

   /// Get/set for server state.
   const ServerState& server() const { return server_;}
   ServerState& set_server() { return server_;}

   /// find all %VAR% and replaces with variable values, returns false on the
   /// first variable that can't be found, cmd will be left half processed.
   /// Will search for ECF_MICRO, if not found assumes % as the micro char
   bool variableSubsitution(std::string& cmd) const { return server_.variableSubsitution(cmd);}

   /// returns true if definition file passes its verification
   /// If the definition file contains verify attribute, this function will check
   /// expected number of state changes corresponds to the actual number of state changes
   bool verification(std::string& errorMsg) const;

   /// generic way of deleting a Node.
   /// This should always succeed else something is seriously wrong
   bool deleteChild(Node*);

   /// This called during replace. If the trigger expression(AST) has been created,  then
   /// the references to the nodes, will be invalidated.
   /// These get generated on the fly, when referenced.
   void invalidate_trigger_references() const;

   /// Adopt the child specified by 'path' from the clientDef.
   /// If node at path already exists in this instance, it is replaced.
   /// if  createNodesAsNeeded = false,  then the path must exist on this defs
   /// otherwise an error message is issued.
   /// if  createNodesAsNeeded = true, and the path does not exist on this defs
   /// then the missing path nodes are created.
   /// In both the client and this defs the trigger references and cleared first.
   /// Returns the changed node, or NULL and error message set.
   node_ptr replaceChild(const std::string& path,
            const defs_ptr& clientDef,
            bool createNodesAsNeeded,
            bool force,
            std::string& errormsg);

   // Order the suite
   void order(Node* immediateChild, NOrder::Order);

   /// determines why the node is not running.
   void top_down_why(std::vector<std::string>& theReasonWhy,bool html_tags = false) const;
   bool why(std::vector<std::string>& theReasonWhy,bool html_tags = false) const; // return true if why found

   /// Function to save/restore the defs as a checkpoint file. Can throw exception
   /// If the Defs file has content, this is deleted first, i.e. suites, externs, Can throw an exception
   void boost_save_as_checkpt(const std::string& fileName,ecf::Archive::Type = ecf::Archive::default_archive()) const;
   void boost_save_as_filename(const std::string& fileName,ecf::Archive::Type = ecf::Archive::default_archive()) const; // used in test only
   void boost_restore_from_checkpt(const std::string& fileName,ecf::Archive::Type = ecf::Archive::default_archive());

   // defs format
   void save_as_checkpt(const std::string& fileName) const;
   void save_as_filename(const std::string& fileName,PrintStyle::Type_t = PrintStyle::MIGRATE) const; // used in test only
   void save_as_string(std::string& str,PrintStyle::Type_t = PrintStyle::MIGRATE) const;
   void restore(const std::string& fileName); // will throw
   bool restore(const std::string& fileName,std::string& errorMsg, std::string& warningMsg);
   void restore_from_string(const std::string& str); // will throw
   bool restore_from_string(const std::string& str,std::string& errorMsg, std::string& warningMsg);

   /// Delete suites, externs, client handles, reset suspended, and locate state
   /// etc but Server environment left as is:
   void clear();

   ecf::Flag&       flag()           { return flag_;}
   const ecf::Flag& get_flag() const { return flag_;}

   void add_edit_history(const std::string& path, const std::string& request);
   const std::deque<std::string>& get_edit_history(const std::string& path) const;
   void save_edit_history(bool f) const { save_edit_history_ = f ;}
   static const std::deque<std::string>& empty_edit_history();
   static size_t max_edit_history_size_per_node() { return 20; }

   /// Memento functions:
   void collateChanges(unsigned int client_handle,DefsDelta&) const;
   void set_memento(const StateMemento*,std::vector<ecf::Aspect::Type>& aspects,bool f );
   void set_memento(const ServerStateMemento*,std::vector<ecf::Aspect::Type>& aspects,bool f );
   void set_memento(const ServerVariableMemento*,std::vector<ecf::Aspect::Type>& aspects,bool f );
   void set_memento(const OrderMemento*,std::vector<ecf::Aspect::Type>& aspects,bool f );
   void set_memento(const FlagMemento*,std::vector<ecf::Aspect::Type>& aspects,bool f );

   /// Find the max state change number for defs only. This includes:
   ///   o the Defs state.
   ///   o the Defs suspend state
   ///   o the Defs server state.
   unsigned int defs_only_max_state_change_no() const;

   /// Change functions, used for *transferring* change number, from server to client
   void set_state_change_no( unsigned int x )  { state_change_no_ = x;}
   unsigned int state_change_no() const        { return state_change_no_; }
   void set_modify_change_no( unsigned int x ) { modify_change_no_ = x;}
   unsigned int modify_change_no() const       { return modify_change_no_; }

   ClientSuiteMgr& client_suite_mgr() { return client_suite_mgr_;}

   // Provided for python interface
   std::string toString() const;

   // Will recurse down.
   bool doDeleteChild(Node* nodeToBeDeleted);

   bool checkInvariants(std::string& errorMsg) const;

   void read_state(const std::string& line,const std::vector<std::string>& lineTokens);
   void read_history(const std::string& line,const std::vector<std::string>& lineTokens);
   bool compare_edit_history(const Defs&) const;
   bool compare_change_no(const Defs&) const;
private:
   void do_generate_scripts( const std::map<std::string,std::string>& override) const;
   std::string write_state() const;
   void collate_defs_changes_only(DefsDelta&) const;
   void setupDefaultEnv();
   void add_suite_only(suite_ptr, size_t position);

   /// Removes the suite, from defs returned as suite_ptr, asserts if suite does not exist
   suite_ptr removeSuite(suite_ptr);
   node_ptr removeChild(Node*);
   bool addChild( node_ptr, size_t position = std::numeric_limits<std::size_t>::max());
   friend class Node;

   /// For use by python interface,
   std::vector<suite_ptr>::const_iterator suite_begin() const { return suiteVec_.begin();}
   std::vector<suite_ptr>::const_iterator suite_end() const { return suiteVec_.end();}
   std::set<std::string>::const_iterator extern_begin() const { return externs_.begin();}
   std::set<std::string>::const_iterator extern_end() const { return externs_.end();}
   std::vector<Variable>::const_iterator user_variables_begin() const { return server_.user_variables().begin();}
   std::vector<Variable>::const_iterator user_variables_end() const   { return server_.user_variables().end();}
   std::vector<Variable>::const_iterator server_variables_begin() const { return server_.server_variables().begin();}
   std::vector<Variable>::const_iterator server_variables_end() const   { return server_.server_variables().end();}

   friend void export_Defs();

private:
   /// Note: restoring from a check point file will reset, defs state and modify numbers
   unsigned int    state_change_no_;            // persisted since passed to client, however side effect, is it will be in checkpoint file
   unsigned int    modify_change_no_;           // persisted since passed to client, however side effect, is it will be in checkpoint file
   unsigned int    updateCalendarCount_;
   unsigned int    order_state_change_no_;      // *NOT* persisted
   NState          state_;                      // state & change_no, i,e attribute changed
   ServerState     server_;
   std::vector<suite_ptr> suiteVec_;
   std::map<std::string, std::deque<std::string> > edit_history_;      // path,request
   mutable bool                                    save_edit_history_; // NOT persisted
   ecf::Flag                    flag_;

   ClientSuiteMgr client_suite_mgr_;                    // NOT persisted

   /// Externs are *NEVER* loaded in the server, since they can be computed and
   /// save on network band with, and check point file size.
   std::set<std::string> externs_;                      // NOT persisted

   friend class SaveEditHistoryWhenCheckPointing;

private:
   /// Observer notifications Start. Allow client to query if they are in syncing with server
   void notify_start() { in_notification_ = true;  }
   void notify_end()   { in_notification_ = false; }
   bool in_notification_;
   std::vector<AbstractObserver*> observers_;
   friend class ChangeStartNotification;
   void notify_delete();
public:
   void notify_start(const std::vector<ecf::Aspect::Type>& aspects);
   void notify(const std::vector<ecf::Aspect::Type>& aspects);
   void attach(AbstractObserver*);
   void detach(AbstractObserver*);
   bool is_observed(AbstractObserver*) const; // return true if we have this observer in our list
   bool in_notification() const { return in_notification_;}

private:
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/)
   {
      ar & state_change_no_;
      ar & modify_change_no_;
      ar & updateCalendarCount_;
      ar & state_;
      ar & server_;
      ar & suiteVec_;
      ar & flag_;

      // only save the edit history when check pointing.
      if (Archive::is_saving::value) {
         if (save_edit_history_) {
            ar & edit_history_;
            save_edit_history_ = false; // reset
         }
         else {
            std::map<std::string, std::deque<std::string> > empty_edit_history;
            ar & empty_edit_history;
         }
      }
      else {
         ar & edit_history_;
      }

      if (Archive::is_loading::value) {
         size_t vec_size = suiteVec_.size();
         for(size_t i = 0; i < vec_size; i++) {
            suiteVec_[i]->set_defs(this);
         }
      }
   }
};

std::ostream& operator<<(std::ostream& os, const Defs*);
std::ostream& operator<<(std::ostream& os, const Defs&);

// =====================================================================
// This class is used to read the History
class DefsHistoryParser : private boost::noncopyable {
public:
   DefsHistoryParser();

   void parse(const std::string& line);
   const std::vector<std::string>& parsed_messages() const { return parsed_messages_;}

private:

   std::string::size_type find_log(const std::string& line, std::string::size_type pos) const;

private:
   std::vector<std::string> log_types_;
   std::vector<std::string> parsed_messages_;
};



// Start notification. End notification automatically signalled, Even if exception raised.
class ChangeStartNotification : private boost::noncopyable {
public:
   explicit ChangeStartNotification(defs_ptr defs) : defs_ptr_(defs) { defs_ptr_->notify_start();}
   ~ChangeStartNotification() {  defs_ptr_->notify_end();}
private:
   defs_ptr defs_ptr_;
};


#endif /* DEFS_HPP_ */
