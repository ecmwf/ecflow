/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_Node_HPP
#define ecflow_node_Node_HPP

///
/// \note The node class does NOT serialise the triggers and complete.
/// These are created on demand in the server. However when the client code
/// loads the definition file, the Defs::check will create AST for trigger and
/// complete expressions, because:
///  1/ the AST are created, so that any parser errors can be _reported_ to the user
///  2/ References in the AST expressions are resolved, and errors flagged.
///
/// This information could have been saved, however was _not_, because:
///  1/ problems on AIX
///  2/ Cut down on IPC load between client/server
///

#include <iosfwd>
#include <limits>

#include "ecflow/attribute/CronAttr.hpp"
#include "ecflow/attribute/DateAttr.hpp"
#include "ecflow/attribute/DayAttr.hpp"
#include "ecflow/attribute/NodeAttr.hpp"
#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/attribute/TimeAttr.hpp"
#include "ecflow/attribute/TodayAttr.hpp"
#include "ecflow/attribute/Variable.hpp"
#include "ecflow/core/Child.hpp"
#include "ecflow/core/DState.hpp"
#include "ecflow/core/NOrder.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Aspect.hpp"
#include "ecflow/node/Attr.hpp"
#include "ecflow/node/Flag.hpp"
#include "ecflow/node/InLimit.hpp"
#include "ecflow/node/InLimitMgr.hpp"
#include "ecflow/node/NodeFwd.hpp"

namespace ecf {
class Simulator;
class SimulatorVisitor;
class DefsAnalyserVisitor;
class FlatAnalyserVisitor;
} // namespace ecf

namespace ecf {
class Calendar;
class NodeTreeVisitor;
class LateAttr;
class AvisoAttr;
class MirrorAttr;
} // namespace ecf

class Node : public std::enable_shared_from_this<Node> {
protected:
    Node(const std::string& name, bool check);
    Node();

public:
    Node& operator=(const Node&);
    bool operator<(const Node& rhs) { return name() < rhs.name(); }

    Node(const Node& rhs);
    virtual ~Node();

    virtual bool check_defaults() const;

    // parse string and create suite || family || task || alias. Can return a NULL node_ptr() for errors
    static node_ptr create(const std::string& node_string);
    static node_ptr create(const std::string& node_string, std::string& error_msg);

    /// The Parent Must set the parent pointer. For a Suite however this will be NULL
    void set_parent(Node* p) { parent_ = p; }

    virtual node_ptr clone() const = 0;

    // Server called functions:
    //   Required when we have time attributes, when time related attribute are free they stay free, until re-queue
    struct Calendar_args
    {
        Calendar_args() = default;

        std::vector<node_ptr> auto_cancelled_nodes_;
        std::vector<node_ptr> auto_archive_nodes_;
    };
    virtual bool calendarChanged(const ecf::Calendar&,
                                 Node::Calendar_args&,
                                 const ecf::LateAttr* inherited_late,
                                 bool holding_parent_day_or_date);

    /// resolving dependencies means we look at day,date,time and triggers and check to
    /// to see if a node is free or still holding. When a node if free of its dependencies and limits
    /// Its state is changed to submitted. When a task is in a the submitted state its
    /// associated ecf file can be submitted.
    /// ************************************************************************************
    /// There is no point in resolving child dependencies if the parent is not FREE
    //  We will return a bool, true means we are free, false for holding
    // *************************************************************************************
    virtual bool resolveDependencies(JobsParam& jobsParam);

    /// When the server is terminated and then restarted, Allow time attributes to catch up.
    /// We need to check all attributes dependent on time.
    virtual bool has_time_based_attributes() const;

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
    virtual void order(Node* /*immediateChild*/, NOrder::Order) {}
    virtual void move_peer(Node* /*src*/, Node* /*dest*/) {}

    /// called when definition is restored from a file/checkpoint
    virtual void handle_migration(const ecf::Calendar&);

    /// reset all. Used after job generation.
    /// Unlike re-queue/begin, will reset time attributes. see ECFLOW-1204
    virtual void reset();

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
    ///
    /// When the user issues force-complete or run interactive commands, we want to miss
    /// the next-time slot. (i.e to avoid running the job again).
    /// This is done by using NO_REQUE_IF_SINGLE_TIME_DEP flag.
    /// The flag remain set until we get to *this* function. We use it to avoid
    /// resetting the time slots, effectively missing the next time slot. we then clear the flag.
    /// However if the JOB *abort* we clear NO_REQUE_IF_SINGLE_TIME_DEP
    /// Otherwise if we run again, we miss additional time slots necessarily
    struct Requeue_args
    {
        enum Requeue_t { REPEAT_INCREMENT = 1, TIME = 2, FULL = 3 };
        Requeue_args() = default;
        explicit Requeue_args(Requeue_t r_t) : requeue_t(r_t) {};
        Requeue_args(Requeue_t r_t,
                     bool resetRepeats,
                     int clear_suspended_in_child_nodes,
                     bool reset_next_time_slot,
                     bool reset_relative_duration,
                     bool log_state_changes = true)
            : requeue_t(r_t),
              clear_suspended_in_child_nodes_(clear_suspended_in_child_nodes),
              resetRepeats_(resetRepeats),
              reset_next_time_slot_(reset_next_time_slot),
              reset_relative_duration_(reset_relative_duration),
              log_state_changes_(log_state_changes) {}
        const Requeue_t requeue_t{FULL};
        int clear_suspended_in_child_nodes_{0};
        const bool resetRepeats_{true};
        const bool reset_next_time_slot_{true};
        const bool reset_relative_duration_{true};
        const bool log_state_changes_{true};
    };
    virtual void requeue(Requeue_args&);

    // force queued allows a job to re-run preserving job output.
    // However, other nodes may reference this node's events/meters/late in trigger expression,
    // and reset events, meters and late flag ECFLOW-1617
    virtual void reset_late_event_meters();

    /// Re queue the time based attributes only.
    /// Used as a part of Alter (clock) functionality.
    /// Note: Under the hybrid clock this will not mark node as complete, (if we have day,date,cron attributes)
    /// Since alter clock, should not change node state. This is left for user to re-queue the suite
    virtual void requeue_time_attrs();

    /// Previously < 4.0.6 requeue always reset the labels on requeue.
    /// However ECFLOW-195 suggest some user prefer to see the last label value.
    /// hence we will only reset the labels on the tasks when task is being run.
    void requeue_labels();

    /// This functionality is only required during interactive force or run
    /// Avoid running the task on the same time slot, by missing the next time slot.
    /// Requires we set a flag, to avoid the requeue resetting the time slots
    void miss_next_time_slot();

    /// Recursively run the tasks under this node, ignore suspend,limits,triggers, and time dependencies
    /// if force is set, run even if task is submitted or active. (will create zombies)
    virtual bool run(JobsParam& jobsParam, bool force) = 0;

    /// Recursively determines why the node is not running.
    virtual bool top_down_why(std::vector<std::string>& theReasonWhy, bool html_tags = false) const;
    void bottom_up_why(std::vector<std::string>& theReasonWhy, bool html_tags = false) const;

    void freeTrigger() const;
    void clearTrigger() const;
    void freeComplete() const;
    void clearComplete() const;
    void freeHoldingDateDependencies();
    void freeHoldingTimeDependencies();

    // Used in the force cmd
    bool set_event(const std::string& event_name_or_number);
    bool clear_event(const std::string& event_name_or_number);
    void setRepeatToLastValue();
    virtual void setRepeatToLastValueHierarchically() { setRepeatToLastValue(); }

    /// find all %VAR% and replaces with variable values, returns false on the
    /// first variable that can't be found, cmd will be left half processed.
    /// Will search for ECF_MICRO, if not found assumes % as the micro char
    bool variableSubstitution(std::string& cmd) const;

    bool variable_substitution(std::string& cmd, const NameValueMap& user_edit_variables, char micro = '%') const;

    /// Find all %VAR% and add to the list, there can be more than one. i.e %ECF_FILES% -I %ECF_INCLUDE%"
    bool find_all_used_variables(std::string& cmd, NameValueMap& used_variables, char micro = '%') const;

    /// Find all environment variables, in the input string and substitute.
    /// with correspondingly named variable value.
    /// i.e search for ${ENV} and replace
    bool variable_dollar_substitution(std::string& cmd) const;

    /// Resolve inlimit references to limits, and check trigger and complete expression
    virtual bool check(std::string& errorMsg, std::string& warningMsg) const;

    /// Generated variables. Suites can have thousands of tasks. If the generated variables associated with
    /// tasks are sent to the client, it can amount to a very large network traffic.
    /// To minimise this bandwidth the generated variables for tasks/families are not persisted.
    /// However client can demand create the generated  variable by calling this function.
    virtual void update_generated_variables() const = 0;

    ///  generates job file independent of dependencies
    virtual void check_job_creation(job_creation_ctrl_ptr jobCtrl) = 0;

    node_ptr remove(); // gets the parent then calls removeChild
    virtual node_ptr removeChild(Node* child) = 0;
    virtual bool
    addChild(const node_ptr& child,
             size_t position =
                 std::numeric_limits<std::size_t>::max()) = 0; // return false if child of same name exist, leak!!!
    virtual bool isAddChildOk(Node* child, std::string& errorMsg) const = 0; // return false if child of same name

    virtual void verification(std::string& errorMsg) const;

    /// See defs.hpp
    virtual void generate_scripts(const std::map<std::string, std::string>& override) const = 0;

    // standard functions: ==============================================
    std::string print() const;
    virtual void print(std::string&) const;
    std::string print(PrintStyle::Type_t type) const;
    bool operator==(const Node& rhs) const;
    virtual bool checkInvariants(std::string& errorMsg) const;

    /// Implements the visitor pattern
    virtual void accept(ecf::NodeTreeVisitor&)               = 0;
    virtual void acceptVisitTraversor(ecf::NodeTreeVisitor&) = 0; // Visitor does the traversal

    // state related functions: ========================================

    /// If the task was aborted provide the reason, default returns empty string
    /// Only the task will return the reason for abort.
    virtual const std::string& abortedReason() const;

    /// This state added as an convenience, it includes Suspended attribute returned as enum
    DState::State dstate() const;

    /// This represents the persisted/saved state // First = NState, Second = time_duration
    /// The State represent the life cycle changes of a node.
    NState::State state() const { return st_.first.state(); }

    /// return the state and duration time(relative to when suite was begun) when the state change happened
    std::pair<NState, boost::posix_time::time_duration> get_state() const { return st_; }

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
    void set_state(NState::State s, bool force = false, const std::string& additional_info_to_log = "");
    virtual void set_state_hierarchically(NState::State s, bool force) { set_state(s, force); }

    /// Set state only, has no side effects
    void setStateOnly(NState::State s,
                      bool force                                = false,
                      const std::string& additional_info_to_log = "",
                      bool log_state_changes                    = true);
    virtual void setStateOnlyHierarchically(NState::State s, bool force = false) { setStateOnly(s, force); }

    /// This returns the time of state change: (relative to real time when the suite calendar was begun)
    /// The returned time is *real time/computer UTC time* and *not* suite real time.
    boost::posix_time::ptime state_change_time() const;

    /// Returns the difference between last two state changes, effectively state change runtime.
    /// queued    -> submitted : 1hr 30s,  submitted -> active    : 30s, active    -> complete  : 59m  etc
    const boost::posix_time::time_duration& state_change_runtime() const { return sc_rt_; }

    /// Sets the default status the node should have when the begin/re-queue is called
    /// *Distinguish* between adding a def status and changing it.
    /// Changing via d_st_.setState(s);  should alter state_change_no
    void addDefStatus(DState::State s) { d_st_ = DState(s); }
    DState::State defStatus() const { return d_st_.state(); }

    // Query functions: =========================================================
    /// returns my parent, for suite will return NULL;
    Node* parent() const { return parent_; }
    virtual Suite* suite() const = 0;
    virtual Defs* defs() const   = 0;

    // Performance hack, to avoid casts
    virtual Task* isTask() const { return nullptr; }
    virtual Alias* isAlias() const { return nullptr; }
    virtual Submittable* isSubmittable() const { return nullptr; }
    virtual NodeContainer* isNodeContainer() const { return nullptr; }
    virtual Family* isFamily() const { return nullptr; }
    virtual Suite* isSuite() const { return nullptr; }
    virtual bool has_archive() const { return false; }

    ///  returns the absolute node path
    std::string absNodePath() const;
    virtual const std::string& debugType() const = 0;

    /// returns abs node path preceded by the type of the node
    std::string debugNodePath() const;
    static std::string path_href_attribute(const std::string& path);
    static std::string path_href_attribute(const std::string& path, const std::string& path2);
    std::string path_href() const;

    /// returns true if this node OR any of its children
    /// has cron,time,day,date or today time dependencies
    virtual bool hasTimeDependencies() const { return has_time_dependencies(); }
    bool isTimeFree() const { return (hasTimeDependencies()) ? timeDependenciesFree() : false; }
    bool time_today_cron_is_free() const; /* used by viewer */

    /// If no time dependencies then we have a resolution of 1 hour.
    /// If we have just day/date then we have a resolution of 1 hour
    /// Otherwise if we have time/today/cron with minutes, then resolution is 1 minute
    void get_time_resolution_for_simulation(boost::posix_time::time_duration& resol) const;
    void get_max_simulation_duration(boost::posix_time::time_duration& resol) const;

    /// hierarchical function
    virtual bool hasAutoCancel() const { return (auto_cancel_) ? true : false; }
    virtual void invalidate_trigger_references() const;

    ///
    /// Checks if the current node is synchronised to (i.e. mirror of) a Node in an external ecFlow server.
    ///
    /// \return true, if this node is a mirror; otherwise, false
    ///
    bool isMirror() const { return !mirrors_.empty(); }

    // Access functions: ======================================================
    const std::string& name() const { return n_; }
    const Repeat& repeat() const { return repeat_; } // can be empty()
    const std::vector<Variable>& variables() const { return vars_; }
    const std::vector<limit_ptr>& limits() const { return limits_; }
    const std::vector<InLimit>& inlimits() const { return inLimitMgr_.inlimits(); }

    const std::vector<Meter>& meters() const { return meters_; }
    const std::vector<Event>& events() const { return events_; }
    const std::vector<Label>& labels() const { return labels_; }

    const std::vector<ecf::TimeAttr>& timeVec() const { return times_; }
    const std::vector<ecf::TodayAttr>& todayVec() const { return todays_; }
    const std::vector<DateAttr>& dates() const { return dates_; }
    const std::vector<DayAttr>& days() const { return days_; }
    const std::vector<ecf::CronAttr>& crons() const { return crons_; }

    std::vector<ecf::AvisoAttr>& avisos() { return avisos_; }
    const std::vector<ecf::AvisoAttr>& avisos() const { return avisos_; }

    std::vector<ecf::MirrorAttr>& mirrors() { return mirrors_; }
    const std::vector<ecf::MirrorAttr>& mirrors() const { return mirrors_; }

    const std::vector<VerifyAttr>& verifys() const;
    const std::vector<ZombieAttr>& zombies() const;
    const std::vector<QueueAttr>& queues() const;
    const std::vector<GenericAttr>& generics() const;
    ecf::LateAttr* get_late() const { return late_.get(); }
    ecf::AutoRestoreAttr* get_autorestore() const { return auto_restore_.get(); }
    ecf::AutoCancelAttr* get_autocancel() const { return auto_cancel_.get(); }
    ecf::AutoArchiveAttr* get_autoarchive() const { return auto_archive_.get(); }
    ecf::Flag& flag() { return flag_; }
    const ecf::Flag& get_flag() const { return flag_; }

    [[deprecated]] virtual void gen_variables(std::vector<Variable>&) const;
    std::vector<Variable> gen_variables() const;
    bool getLabelValue(const std::string& name, std::string& value) const;
    bool getLabelNewValue(const std::string& name, std::string& value) const;

    // Use get_trigger()/get_complete() for determining if we have trigger
    // and complete expressions. This is many times faster than calling
    // triggerAst()/completeAst() as this will force a parse and construction
    // of Abstract syntax tree, first time it is called.
    Expression* get_trigger() const { return t_expr_.get(); }
    Expression* get_complete() const { return c_expr_.get(); }
    AstTop* completeAst() const; // Will create AST on demand
    AstTop* triggerAst() const;  // Will create AST on demand
    std::string completeExpression() const;
    std::string triggerExpression() const;

    virtual void get_all_tasks(std::vector<task_ptr>&) const                   = 0;
    virtual void get_all_nodes(std::vector<node_ptr>&) const                   = 0;
    virtual void get_all_aliases(std::vector<alias_ptr>&) const                = 0;
    virtual void getAllTasks(std::vector<Task*>&) const                        = 0;
    virtual void getAllSubmittables(std::vector<Submittable*>&) const          = 0;
    virtual void get_all_active_submittables(std::vector<Submittable*>&) const = 0;
    virtual void getAllNodes(std::vector<Node*>&) const                        = 0;
    virtual void getAllAstNodes(std::set<Node*>&) const;

    /// returns the immediate children
    virtual void immediateChildren(std::vector<node_ptr>&) const {}

    /// retrieve _ALL_ children by hierarchically traversing down the node tree
    virtual void allChildren(std::vector<node_ptr>&) const {}

    void replace_variables(const std::vector<Variable>& vars);
    void replace_labels(const std::vector<Label>& labels);
    void replace_meters(const std::vector<Meter>& meters);
    void replace_events(const std::vector<Event>& events);

    std::vector<Variable> get_all_generated_variables() const;

    // Add functions: ===============================================================
    void addVerify(const VerifyAttr&); // for testing and verification Can throw std::runtime_error
    void addVariable(const Variable&); // will update if duplicate
    void add_variable(const std::string& name, const std::string& value); // will write to std:out if duplicates
    void add_variable_bypass_name_check(const std::string& name,
                                        const std::string& value); // will write to std:out if duplicates
    void add_variable_int(const std::string& name, int);           // will throw std::runtime_error if duplicate

    void add_trigger(const std::string&);          // use for short complete expressions,Can throw std::runtime_error
    void add_complete(const std::string&);         // use for short complete expressions,Can throw std::runtime_error
    void add_trigger_expr(const Expression&);      // Can throw std::runtime_error
    void add_complete_expr(const Expression&);     // Can throw std::runtime_error
    void add_part_trigger(const PartExpression&);  // for adding multiple and/or expression,Can throw std::runtime_error
    void add_part_complete(const PartExpression&); // for adding multiple and/or expression,Can throw std::runtime_error
    void py_add_trigger_expr(const std::vector<PartExpression>&);  // used by python api to add expression cumulative
    void py_add_complete_expr(const std::vector<PartExpression>&); // used by python api to add expression cumulative

    void addTime(const ecf::TimeAttr&);
    void addToday(const ecf::TodayAttr&);
    void addDate(const DateAttr&);
    void addDay(const DayAttr&);
    void addCron(const ecf::CronAttr&);
    void addAviso(const ecf::AvisoAttr&);
    void addMirror(const ecf::MirrorAttr&);

    void addLimit(const Limit&, bool check = true);       // will throw std::runtime_error if duplicate
    void addInLimit(const InLimit& l, bool check = true); // will throw std::runtime_error if duplicate
    void auto_add_inlimit_externs(Defs* defs) { inLimitMgr_.auto_add_inlimit_externs(defs); }
    void addEvent(const Event&, bool check = true); // will throw std::runtime_error if duplicate
    void addMeter(const Meter&, bool check = true); // will throw std::runtime_error if duplicate
    void add_meter(const std::string& name,
                   int min,
                   int max,
                   int color_change,
                   int value,
                   bool check = true); // will throw std::runtime_error if duplicate
    void addLabel(const Label&);       // will throw std::runtime_error if duplicate
    void add_label(const std::string& name, const std::string& value, const std::string& new_value, bool check = true);
    void addAutoCancel(const ecf::AutoCancelAttr&);
    void add_autoarchive(const ecf::AutoArchiveAttr&);
    void add_autorestore(const ecf::AutoRestoreAttr&);
    void addLate(const ecf::LateAttr&);
    void addRepeat(Repeat&&);             // will throw std::runtime_error if duplicate
    void addRepeat(const Repeat&);        // will throw std::runtime_error if duplicate
    void addZombie(const ZombieAttr&);    // will throw std::runtime_error if duplicate
    void add_queue(const QueueAttr&);     // will throw std::runtime_error if duplicate
    void add_generic(const GenericAttr&); // will throw std::runtime_error if duplicate

    // sort
    // expect one attr to be [ event | meter | label | limits | variable ]
    virtual void sort_attributes(ecf::Attr::Type at,
                                 bool recursive                          = true,
                                 const std::vector<std::string>& no_sort = std::vector<std::string>());

    // Delete functions: can throw std::runtime_error ===================================
    // if name argument is empty, delete all attributes of that type
    // Can throw std::runtime_error of the attribute cannot be found
    void deleteTime(const std::string& name);
    void delete_time(const ecf::TimeAttr&);
    void deleteToday(const std::string& name);
    void delete_today(const ecf::TodayAttr&);
    void deleteDate(const std::string& name);
    void delete_date(const DateAttr&);
    void deleteDay(const std::string& name);
    void delete_day(const DayAttr&);
    void deleteCron(const std::string& name);
    void delete_cron(const ecf::CronAttr&);

    void delete_zombie(const ecf::Child::ZombieType);
    void deleteVariable(const std::string& name); // if name not found throw, if name empty delete all variables
    void
    delete_variable_no_error(const std::string& name); // if variable not found don't error, if name empty do nothing
    void deleteEvent(const std::string& name);
    void deleteMeter(const std::string& name);
    void deleteLabel(const std::string& name);
    void deleteAviso(const std::string& name);
    void deleteMirror(const std::string& name);
    void delete_queue(const std::string& name);
    void delete_generic(const std::string& name);
    void deleteTrigger();
    void deleteComplete();
    void deleteRepeat();
    void deleteLimit(const std::string& name);
    void delete_limit_path(const std::string& limit_name, const std::string& limit_path);
    void deleteInlimit(const std::string& name);
    void deleteZombie(const std::string& type); // string must be one of [ user | ecf | path ]
    void deleteLate();
    void deleteAutoCancel();
    void deleteAutoRestore();
    void deleteAutoArchive();

    // Change functions: ================================================================
    /// returns true the change was made else false, Can throw std::runtime_error for parse errors
    void changeVariable(const std::string& name, const std::string& value);
    void changeEvent(const std::string& name, const std::string& setOrClear = "");
    void changeEvent(const std::string& name, bool value);
    void changeMeter(const std::string& name, const std::string& value);
    void changeMeter(const std::string& name, int value);
    void changeLabel(const std::string& name, const std::string& value);
    void changeAviso(const std::string& name, const std::string& value);
    void changeAviso(const std::string& name, const std::string& value, uint64_t revision);
    void changeMirror(const std::string& name, const std::string& value);
    void changeTrigger(const std::string& expression);
    void changeComplete(const std::string& expression);
    void changeRepeat(const std::string& value);
    void changeLimitMax(const std::string& name, const std::string& maxValue);
    void changeLimitMax(const std::string& name, int maxValue);
    void changeLimitValue(const std::string& name, const std::string& value);
    void changeLimitValue(const std::string& name, int value);
    void changeDefstatus(const std::string& state);
    void changeLate(const ecf::LateAttr&);
    void change_time(const std::string&, const std::string&);
    void change_today(const std::string&, const std::string&);

    bool set_meter(const std::string& name, int value);  // does not throw if meter not found
    bool set_event(const std::string& name, bool value); // does not throw if event not found

    void increment_repeat(); // used in test only

    // mementos functions:
    /// Collect all the state changes, so that only small subset is returned to client
    virtual void collateChanges(DefsDelta&) const = 0;
    void incremental_changes(DefsDelta&, compound_memento_ptr& comp) const;

    void set_memento(const NodeStateMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeDefStatusDeltaMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const SuspendedMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeEventMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeMeterMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeLabelMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeAvisoMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeMirrorMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeQueueMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeGenericMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeQueueIndexMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeTriggerMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeCompleteMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeRepeatMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeRepeatIndexMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeLimitMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeInLimitMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeVariableMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeLateMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeTodayMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeTimeMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeDayMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeCronMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeDateMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeZombieMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const NodeVerifyMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);
    void set_memento(const FlagMemento*, std::vector<ecf::Aspect::Type>& aspects, bool f);

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

    /// Search up the hierarchy, simply checks for existence independent of variable value
    bool user_variable_exists(const std::string& name) const;

    virtual node_ptr findImmediateChild(const std::string& /*name*/, size_t& /*child_pos*/) const { return node_ptr(); }
    virtual node_ptr find_immediate_child(const std::string_view&) const { return node_ptr(); }
    virtual std::string find_node_path(const std::string& /*type*/, const std::string& /*name*/) const {
        return std::string();
    }
    const Variable& findVariable(const std::string& name) const;
    std::string find_parent_variable_sub_value(const std::string& name) const;
    const Variable& find_parent_variable(const std::string& name) const;
    virtual const Variable& findGenVariable(const std::string& name) const;
    bool findVariableValue(const std::string& name, std::string& returnedValue) const;
    bool findGenVariableValue(const std::string& name, std::string& returnedValue) const;

    bool findVerify(const VerifyAttr&) const;
    bool findLimit(const Limit&) const;
    bool findLabel(const std::string& name) const;
    const Label& find_label(const std::string& name) const;
    bool findAviso(const std::string& name) const;
    bool findMirror(const std::string& name) const;
    const QueueAttr& find_queue(const std::string& name) const;
    QueueAttr& findQueue(const std::string& name);
    const GenericAttr& find_generic(const std::string& name) const;

    limit_ptr find_limit(const std::string& name) const;
    limit_ptr findLimitUpNodeTree(const std::string& name) const;
    Limit* findLimitViaInLimit(const InLimit& l) const {
        return inLimitMgr_.findLimitViaInLimit(l);
    } // used in *test* only
    bool findInLimitByNameAndPath(const InLimit& l) const {
        return inLimitMgr_.findInLimitByNameAndPath(l);
    } // use name,path,token,
    const Repeat& findRepeat(const std::string& name) const;
    const Meter& findMeter(const std::string& name) const;
    Meter& find_meter(const std::string& name);
    const Event& findEvent(const Event&) const;
    const Event& findEventByNameOrNumber(const std::string& name) const;

    const ZombieAttr& findZombie(ecf::Child::ZombieType) const;
    bool findParentZombie(ecf::Child::ZombieType, ZombieAttr&) const;

    /// Finds the referenced node. The node path can be relative or absolute or a extern path
    /// however if its an extern path, and corresponding suite is loaded, but we still
    /// can't find the path, then an error is returned
    node_ptr findReferencedNode(const std::string& nodePath, std::string& errorMsg) const;
    node_ptr findReferencedNode(const std::string& nodePath, const std::string& externObj, std::string& errorMsg) const;

    /// return true if we can find a event, meter, user, repeat or generated variable with the given name
    bool findExprVariable(const std::string& name); // update event & meter as used in trigger for simulator

    /// The status of family/suite is the inherited most significant status of all its children
    enum TraverseType { IMMEDIATE_CHILDREN, HIERARCHICAL };
    virtual NState::State computedState(Node::TraverseType) const = 0;

    /// Sets the most significant state up the node tree. Ignores tasks
    void set_most_significant_state_up_node_tree();

    /// For use with GUI only
    void set_graphic_ptr(void* p) { graphic_ptr_ = p; }
    void* graphic_ptr() const { return graphic_ptr_; }

    /// returns the position of this node relative to its peers
    /// If not attached to parent returns std::numeric_limits<std::size_t>::max();
    size_t position() const;

    void stats(NodeStats&);

    /// called at the end of state change function
    /// ** Every time we set the state on a nodecontainer, we call handleStateChange
    /// ** This by default works out the most significant state of the children
    /// ** ie. the computed state. Hence setting the state on Suite/Family is really
    /// ** meaningless, since it will always be the computed state.
    /// ** For Aliases we only update the limits, and do not bubble up state changes
    virtual void handleStateChange() = 0; // can end up changing state

    /// update change numbers to force sync
    virtual void force_sync() {};

    /// check trigger expression have nodes and events,meter,repeat that resolve
    bool check_expressions(Ast*, const std::string& expr, bool trigger, std::string& errorMsg) const;

    /// check trigger expression have nodes and events,meter,repeat that resolve, will throw for error
    std::unique_ptr<AstTop>
    parse_and_check_expressions(const std::string& expr, bool trigger, const std::string& context);

    virtual boost::posix_time::time_duration sum_runtime() { return sc_rt_; }

    void set_state_change_no(unsigned int x) { state_change_no_ = x; }
    unsigned int state_change_no() const { return state_change_no_; }

protected:
    void set_runtime(const boost::posix_time::time_duration& rt) { sc_rt_ = rt; }

    /// Used in conjunction with Node::position()
    /// returns std::numeric_limits<std::size_t>::max() if child not found
    virtual size_t child_position(const Node*) const = 0;

    /// The set_state_only() requires a correctly formed tree, ie since it needs suite()/calendar
    /// to initialise the duration. We need a way set the state directly. For initialization
    void set_state_only(NState::State s) { st_.first.setState(s); }

    /// based on the *current* state increment or decrements the limits
    /// Should *only* be called within a task
    virtual void update_limits() = 0;

    /// After job submission we need to increment the in limit, to indicate that a
    /// resource is consumed. The set ensure we only update once during a traversal
    void incrementInLimit(std::set<Limit*>& limitSet);

    /// After job aborts or completes we need to decrement the in limit, to indicate that
    /// additional resource is available. The set ensure we only update once during a traversal
    void decrementInLimit(std::set<Limit*>& limitSet);
    void decrementInLimitForSubmission(std::set<Limit*>& limitSet);

    friend class InLimitMgr;
    bool check_in_limit() const { return inLimitMgr_.inLimit(); }
    bool check_in_limit_up_node_tree() const;

    friend class Defs;
    friend class Family;
    friend class NodeContainer;
    friend class HoldingDayOrDate;
    virtual bool doDeleteChild(Node*) { return false; }

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

    node_ptr non_const_this() const;

    void update_repeat_genvar() const;

    // returns node state without trailing new lines
    virtual void write_state(std::string& ret, bool& added_comment_char) const;
    void add_comment_char(std::string& ret, bool& added_comment_char) const;
    virtual void read_state(const std::string& line, const std::vector<std::string>& lineTokens);

    // Observer notifications
protected:
    std::vector<AbstractObserver*> observers_;
    void notify_delete();
    void checkForLateness(const ecf::Calendar&);
    void check_for_lateness(const ecf::Calendar& c, const ecf::LateAttr*);

public:
    void notify_start(const std::vector<ecf::Aspect::Type>& aspects);
    void notify(const std::vector<ecf::Aspect::Type>& aspects);
    void attach(AbstractObserver*);
    void detach(AbstractObserver*);
    bool is_observed(AbstractObserver*) const; // return true if we have this observer in our list

private:
    bool why(std::vector<std::string>& theReasonWhy, bool html_tags = false) const;
    /// Function used as a part of trigger and complete expressions.
    /// The search pattern is event,meter,user-variable,repeat, generated-variable
    int findExprVariableValue(const std::string& name) const;
    int findExprVariableValueAndPlus(const std::string& name, int val) const;
    int findExprVariableValueAndMinus(const std::string& name, int val) const;
    int findExprVariableValueAndType(const std::string& name, std::string& varType) const;
    void findExprVariableAndPrint(const std::string& name, std::ostream& os) const;
    friend class VariableHelper;
    friend class AstParentVariable;
    bool update_variable(const std::string& name, const std::string& value);

private:
    void add_trigger_expression(const Expression&);  // Can throw std::runtime_error
    void add_complete_expression(const Expression&); // Can throw std::runtime_error
    const Event& findEventByNumber(int number) const;
    const Event& findEventByName(const std::string& name) const;
    bool set_meter_used_in_trigger(const std::string& name);
    bool set_event_used_in_trigger(const std::string& name);

    /// When the begin/re-queue is called this function will initialise the state
    /// on the node. If node has a default state this is applied to the node, and
    /// hierarchically to all the children
    /// Can also clear suspended see re-queue()
    void initState(int clear_suspended_in_child_nodes, bool log_state_changes = true);

    void delete_misc_attrs_if_empty();

    /// Under the hybrid calendar some time dependent attributes may not be applicable
    /// i.e if day,date,cron attributes does correspond to 24 hours of today, then we
    /// need make them as complete.
    void markHybridTimeDependentsAsComplete();
    bool testTimeDependenciesForRequeue();
    bool calendar_changed_timeattrs(const ecf::Calendar& c, Node::Calendar_args&);
    bool holding_day_or_date(const ecf::Calendar& c) const;
    void do_requeue_time_attrs(bool reset_next_time_slot, bool reset_relative_duration, Requeue_args::Requeue_t);
    bool has_time_dependencies() const;

private: // allow simulator access
    friend class ecf::DefsAnalyserVisitor;
    friend class ecf::FlatAnalyserVisitor;
    friend class ecf::SimulatorVisitor;
    friend class ecf::Simulator;
    std::vector<Meter>& ref_meters() { return meters_; } // allow simulator set meter value
    std::vector<Event>& ref_events() { return events_; } // allow simulator set event value
    std::vector<QueueAttr>& ref_queues();                // allow simulator set event value

    /// Note: If the complete expression evaluation fails. we should continue resolving dependencies
    ///       If the complete expression evaluation evaluates, then we set node to complete
    bool evaluateComplete() const;
    bool evaluateTrigger() const;
    bool timeDependenciesFree() const;

    AstTop* completeAst(std::string& errorMsg) const; // Will create AST on demand
    AstTop* triggerAst(std::string& errorMsg) const;  // Will create AST on demand

private:
    bool check_for_auto_archive(const ecf::Calendar& calendar) const;
    bool checkForAutoCancel(const ecf::Calendar& calendar) const;

private: // All mementos access
    friend class CompoundMemento;
    void clear(); /// Clear *ALL* internal attributes
    void delete_attributes();

private: /// For use by python interface,
    friend void export_Node();
    friend void export_Task();
    friend void export_SuiteAndFamily();
    std::vector<Meter>::const_iterator meter_begin() const { return meters_.begin(); }
    std::vector<Meter>::const_iterator meter_end() const { return meters_.end(); }
    std::vector<Event>::const_iterator event_begin() const { return events_.begin(); }
    std::vector<Event>::const_iterator event_end() const { return events_.end(); }
    std::vector<Label>::const_iterator label_begin() const { return labels_.begin(); }
    std::vector<Label>::const_iterator label_end() const { return labels_.end(); }
    std::vector<ecf::AvisoAttr>::const_iterator aviso_begin() const { return avisos_.begin(); }
    std::vector<ecf::AvisoAttr>::const_iterator aviso_end() const { return avisos_.end(); }
    std::vector<ecf::MirrorAttr>::const_iterator mirror_begin() const { return mirrors_.begin(); }
    std::vector<ecf::MirrorAttr>::const_iterator mirror_end() const { return mirrors_.end(); }
    std::vector<ecf::TimeAttr>::const_iterator time_begin() const { return times_.begin(); }
    std::vector<ecf::TimeAttr>::const_iterator time_end() const { return times_.end(); }
    std::vector<ecf::TodayAttr>::const_iterator today_begin() const { return todays_.begin(); }
    std::vector<ecf::TodayAttr>::const_iterator today_end() const { return todays_.end(); }
    std::vector<DateAttr>::const_iterator date_begin() const { return dates_.begin(); }
    std::vector<DateAttr>::const_iterator date_end() const { return dates_.end(); }
    std::vector<DayAttr>::const_iterator day_begin() const { return days_.begin(); }
    std::vector<DayAttr>::const_iterator day_end() const { return days_.end(); }
    std::vector<ecf::CronAttr>::const_iterator cron_begin() const { return crons_.begin(); }
    std::vector<ecf::CronAttr>::const_iterator cron_end() const { return crons_.end(); }
    std::vector<ZombieAttr>::const_iterator zombie_begin() const;
    std::vector<ZombieAttr>::const_iterator zombie_end() const;
    std::vector<VerifyAttr>::const_iterator verify_begin() const;
    std::vector<VerifyAttr>::const_iterator verify_end() const;
    std::vector<QueueAttr>::const_iterator queue_begin() const;
    std::vector<QueueAttr>::const_iterator queue_end() const;
    std::vector<GenericAttr>::const_iterator generic_begin() const;
    std::vector<GenericAttr>::const_iterator generic_end() const;
    std::vector<Variable>::const_iterator variable_begin() const { return vars_.begin(); }
    std::vector<Variable>::const_iterator variable_end() const { return vars_.end(); }
    std::vector<limit_ptr>::const_iterator limit_begin() const { return limits_.begin(); }
    std::vector<limit_ptr>::const_iterator limit_end() const { return limits_.end(); }
    std::vector<InLimit>::const_iterator inlimit_begin() const { return inLimitMgr_.inlimit_begin(); }
    std::vector<InLimit>::const_iterator inlimit_end() const { return inLimitMgr_.inlimit_end(); }
    std::string to_string() const; // For python interface

private:
    Node* parent_{nullptr}; // *NOT* persisted must be set by the parent class
    std::string n_;
    boost::posix_time::time_duration
        sc_rt_; // state change runtime, Used to order peers, no persistence in cereal only defs.
    std::pair<NState, boost::posix_time::time_duration> st_{
        NState(),
        boost::posix_time::time_duration(0, 0, 0, 0)}; // state and duration since suite start when state changed
    DState d_st_;                                      // default value is QUEUED

    std::vector<Variable> vars_;
    mutable std::unique_ptr<Expression> c_expr_; // can only have one complete expression
    mutable std::unique_ptr<Expression> t_expr_; // can only have one trigger expression

    std::vector<Meter> meters_;
    std::vector<Event> events_;
    std::vector<Label> labels_;
    std::vector<ecf::AvisoAttr> avisos_;
    std::vector<ecf::MirrorAttr> mirrors_;

    std::vector<ecf::TimeAttr> times_;
    std::vector<ecf::TodayAttr> todays_;
    std::vector<ecf::CronAttr> crons_;
    std::vector<DateAttr> dates_;
    std::vector<DayAttr> days_;

    std::unique_ptr<ecf::LateAttr> late_;   // Can only have one late attribute per node
    std::unique_ptr<MiscAttrs> misc_attrs_; // VerifyAttr(used for statistics and test verification) & Zombies
    Repeat repeat_;                         // each node can only have one repeat. By value, since has pimpl

    std::vector<limit_ptr> limits_; // Ptrs since many in-limits can point to a single limit
    InLimitMgr inLimitMgr_{this};   // manages the inlimit

    ecf::Flag flag_;

    std::unique_ptr<ecf::AutoCancelAttr> auto_cancel_;   // Can only have 1 auto cancel per node
    std::unique_ptr<ecf::AutoArchiveAttr> auto_archive_; // Can only have 1 auto archive per node
    std::unique_ptr<ecf::AutoRestoreAttr> auto_restore_; // Can only have 1 autorestore per node
    void* graphic_ptr_{nullptr};                         // for use with the gui only

    unsigned int state_change_no_{
        0}; // *not* persisted, only used on server side,Used to indicate addition or deletion of attribute
    unsigned int variable_change_no_{0};  // *not* persisted, placed here rather than Variable, to save memory
    unsigned int suspended_change_no_{0}; // *not* persisted,

#ifdef DEBUG_JOB_SUBMISSION_INTERVAL
    boost::posix_time::ptime submit_to_complete_duration_; // *not* persisted
#endif

    bool suspended_{false};

    friend class MiscAttrs;

private:
    // conditionally save to cut down on client/server bandwidth.
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

#endif /* ecflow_node_Node_HPP */
