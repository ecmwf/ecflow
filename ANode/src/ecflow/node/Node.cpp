/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cassert>
#include <stdexcept>

#include "ecflow/attribute/AutoArchiveAttr.hpp"
#include "ecflow/attribute/AutoCancelAttr.hpp"
#include "ecflow/attribute/LateAttr.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Extract.hpp"
#include "ecflow/core/Indentor.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/Stl.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/cereal_boost_time.hpp"
#include "ecflow/node/AbstractObserver.hpp"
#include "ecflow/node/AutoRestoreAttr.hpp"
#include "ecflow/node/CmdContext.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/ExprAst.hpp"
#include "ecflow/node/ExprAstVisitor.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/MiscAttrs.hpp"
#include "ecflow/node/NodeStats.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

using namespace ecf;
using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

///////////////////////////////////////////////////////////////////////////////////////////
// #define DEBUG_DEPENDENCIES 1
// #define DEBUG_REQUEUE 1
// #define DEBUG_FIND_REFERENCED_NODE 1

Node::Node(const std::string& n, bool check) : n_(n) {
    if (check) {
        string msg;
        if (!Str::valid_name(n, msg)) {
            throw std::runtime_error("Invalid node name : " + msg);
        }
    }
}

Node::Node() = default;

Node::Node(const Node& rhs)
    : enable_shared_from_this(rhs),
      parent_(nullptr),
      n_(rhs.n_),
      st_(rhs.st_),
      d_st_(rhs.d_st_),
      vars_(rhs.vars_),
      c_expr_((rhs.c_expr_) ? new Expression(*rhs.c_expr_) : nullptr),
      t_expr_((rhs.t_expr_) ? new Expression(*rhs.t_expr_) : nullptr),
      meters_(rhs.meters_),
      events_(rhs.events_),
      labels_(rhs.labels_),
      times_(rhs.times_),
      todays_(rhs.todays_),
      crons_(rhs.crons_),
      dates_(rhs.dates_),
      days_(rhs.days_),
      late_((rhs.late_) ? new ecf::LateAttr(*rhs.late_) : nullptr),
      misc_attrs_((rhs.misc_attrs_) ? new MiscAttrs(*rhs.misc_attrs_) : nullptr),
      repeat_(rhs.repeat_),
      inLimitMgr_(rhs.inLimitMgr_),
      flag_(rhs.flag_),
      auto_cancel_((rhs.auto_cancel_) ? new AutoCancelAttr(*rhs.auto_cancel_) : nullptr),
      auto_archive_((rhs.auto_archive_) ? new AutoArchiveAttr(*rhs.auto_archive_) : nullptr),
      auto_restore_((rhs.auto_restore_) ? new AutoRestoreAttr(*rhs.auto_restore_) : nullptr),
      suspended_(rhs.suspended_) {
    inLimitMgr_.set_node(this);
    if (misc_attrs_)
        misc_attrs_->set_node(this);

    if (auto_restore_)
        auto_restore_->set_node(this);

    for (const auto& limit : rhs.limits_) {
        limit_ptr the_limit = std::make_shared<Limit>(*limit);
        the_limit->set_node(this);
        limits_.push_back(the_limit);
    }
}

bool Node::check_defaults() const {
    if (parent_ != nullptr)
        throw std::runtime_error("Node::check_defaults(): parent_ != nullptr");
    if (graphic_ptr_ != nullptr)
        throw std::runtime_error("Node::check_defaults(): graphic_ptr_  != nullptr");
    if (suspended_ != false)
        throw std::runtime_error("Node::check_defaults(): suspended_ != false");
    if (state_change_no_ != 0)
        throw std::runtime_error("Node::check_defaults(): state_change_no_ != 0");
    if (variable_change_no_ != 0)
        throw std::runtime_error("Node::check_defaults(): variable_change_no_  != 0");
    if (suspended_change_no_ != 0)
        throw std::runtime_error("Node::check_defaults():  suspended_change_no_ != 0");
    if (d_st_.state() != DState::QUEUED)
        throw std::runtime_error("Node::check_defaults(): d_st_.state() != DState::QUEUED");
    return true;
}

void Node::delete_attributes() {
    c_expr_.reset(nullptr);
    t_expr_.reset(nullptr);
    late_.reset(nullptr);
    misc_attrs_.reset(nullptr);
}

node_ptr Node::create(const std::string& node_string) {
    DefsStructureParser parser(node_string);
    std::string errorMsg, warningMsg;
    (void)parser.doParse(errorMsg, warningMsg);
    return parser.the_node_ptr(); // can return NULL
}

node_ptr Node::create(const std::string& node_string, std::string& error_msg) {
    DefsStructureParser parser(node_string);
    std::string warningMsg;
    if (!parser.doParse(error_msg, warningMsg))
        return node_ptr();
    return parser.the_node_ptr();
}

Node& Node::operator=(const Node& rhs) {
    // Note:: Defs assignment operator use copy/swap, hence this assignment note used
    // parent must set parent_
    if (this != &rhs) {
        n_         = rhs.n_;
        suspended_ = rhs.suspended_;
        st_        = rhs.st_;
        d_st_      = rhs.d_st_;

        vars_ = rhs.vars_;

        delete_attributes();

        meters_ = rhs.meters_;
        events_ = rhs.events_;
        labels_ = rhs.labels_;

        times_  = rhs.times_;
        todays_ = rhs.todays_;
        crons_  = rhs.crons_;
        dates_  = rhs.dates_;
        days_   = rhs.days_;

        if (rhs.c_expr_)
            c_expr_ = std::make_unique<Expression>(*rhs.c_expr_);
        if (rhs.t_expr_)
            t_expr_ = std::make_unique<Expression>(*rhs.t_expr_);
        if (rhs.late_)
            late_ = std::make_unique<ecf::LateAttr>(*rhs.late_);
        if (rhs.misc_attrs_)
            misc_attrs_ = std::make_unique<MiscAttrs>(*rhs.misc_attrs_);

        if (rhs.auto_cancel_)
            auto_cancel_ = std::make_unique<AutoCancelAttr>(*rhs.auto_cancel_);
        if (rhs.auto_archive_)
            auto_archive_ = std::make_unique<AutoArchiveAttr>(*rhs.auto_archive_);
        if (rhs.auto_restore_)
            auto_restore_ = std::make_unique<AutoRestoreAttr>(*rhs.auto_restore_);

        repeat_     = rhs.repeat_;
        inLimitMgr_ = rhs.inLimitMgr_;
        inLimitMgr_.set_node(this);
        flag_ = rhs.flag_;

        state_change_no_     = 0;
        variable_change_no_  = 0;
        suspended_change_no_ = 0;
        graphic_ptr_         = nullptr;

        if (misc_attrs_)
            misc_attrs_->set_node(this);

        if (auto_restore_)
            auto_restore_->set_node(this);

        limits_.clear();
        for (const auto& limit : rhs.limits_) {
            limit_ptr the_limit = std::make_shared<Limit>(*limit);
            the_limit->set_node(this);
            limits_.push_back(the_limit);
        }
    }
    return *this;
}

Node::~Node() = default;

bool Node::isParentSuspended() const {
    Node* theParent = parent();
    if (theParent) {
        if (theParent->isSuspended())
            return true;
        return theParent->isParentSuspended();
    }
    return defs()->isSuspended(); // obtained from the server states
}

void Node::resume() {
    /// Guard against unnecessary creation of memento's
    if (suspended_) {
        suspended_           = false;
        suspended_change_no_ = Ecf::incr_state_change_no();
    }
}

void Node::suspend() {
    // Typically called via user action or via defstatus
    suspended_           = true;
    suspended_change_no_ = Ecf::incr_state_change_no();
}

void Node::begin() {
    // record effect of defstatus for node changes, for verify attributes
    if (misc_attrs_)
        misc_attrs_->begin();

    // Set the state without causing any side effects
    initState(0);

    clearTrigger();
    clearComplete();

    flag_.reset();
    repeat_.reset(); // if repeat is empty reset() does nothing

    for (auto& meter : meters_) {
        meter.reset();
    }
    for (auto& event : events_) {
        event.reset();
    }
    for (auto& label : labels_) {
        label.reset();
    }

    if (late_)
        late_->reset();
    for (auto& limit : limits_) {
        limit->reset();
    }

    // Let time base attributes use, relative duration if applicable
    {
        const Calendar& calendar = suite()->calendar();
        for (auto& today : todays_) {
            today.reset(calendar);
        }
        for (auto& time : times_) {
            time.reset(calendar);
        }
        for (auto& cron : crons_) {
            cron.reset(calendar);
        }

        for (auto& day : days_) {
            day.reset(calendar);
        }
        for (auto& date : dates_) {
            date.reset();
        }
        markHybridTimeDependentsAsComplete();
    }

    inLimitMgr_.reset(); // new to 5.0.0 clear inlimit.incremented() flag

    // DO *NOT* call update_generated_variables(). Called on a type specific bases, for begin
    // Typically we need only call update_generated_variables() for a task, at job creation time.
    // so that ECF_OUT, ECF_TRYNO, ECF_JOBOUT, ECF_PASS(paswd_) can be updated.
    // However, the generated variables are used when within job generation and can be referenced by AST
    // Hence to avoid excessive memory consumption, they are created on demand
}

void Node::requeue(Requeue_args& args) {
#ifdef DEBUG_REQUEUE
    LOG(Log::DBG, "      Node::requeue() " << absNodePath() << " resetRepeats = " << args.resetRepeats_);
#endif
    /// Note: we don't reset verify attributes as they record state stat's

    // Set the state without causing any side effects
    initState(args.clear_suspended_in_child_nodes_, args.log_state_changes_);

    clearTrigger();
    clearComplete();

    if (args.resetRepeats_)
        repeat_.reset(); // if repeat is empty reset() does nothing

    /// If a job takes longer than it slots, then that slot is missed, and next slot is used
    /// Note we do *NOT* reset for requeue as we want to advance the valid time slots.
    /// *NOTE* Update calendar will *free* time dependencies *even* time series. They rely
    /// on this function to clear the time dependencies so they *HOLD* the task.
    if (has_time_dependencies()) {

        /// Requeue has several contexts:
        ///   1/ manual requeue
        ///   2/ automated requeue due to repeats
        ///   3/ automated requeue due to time dependencies
        /// For manual and automated reueue due to repeat's we always clear Flag::NO_REQUE_IF_SINGLE_TIME_DEP
        /// since in those context we do NOT want miss any time slots
        bool reset_next_time_slot = true;
        if (args.reset_next_time_slot_) {
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

        // must be done before the re-queue
        do_requeue_time_attrs(reset_next_time_slot, args.reset_relative_duration_, args.requeue_t);
        markHybridTimeDependentsAsComplete();
    }

    // Should *NOT* clear, archived flag, as this is done via autorestore or --restore
    // reset the flags, however remember if edit were made
    bool edit_history_set = flag().is_set(ecf::Flag::MESSAGE);
    bool archived_set     = flag().is_set(ecf::Flag::ARCHIVED);
    flag_.reset(); // will CLEAR NO_REQUE_IF_SINGLE_TIME_DEP
    if (edit_history_set)
        flag().set(ecf::Flag::MESSAGE);
    if (archived_set)
        flag().set(ecf::Flag::ARCHIVED);

    if (late_)
        late_->reset();

    for (auto& meter : meters_) {
        meter.reset();
    }
    for (auto& event : events_) {
        event.reset();
    }
    // ECFLOW-195, only clear labels, if they are on Suites/Family not tasks(typically only specified on tasks)
    if (isNodeContainer()) {
        for (auto& label : labels_) {
            label.reset();
        }
    }

    if (misc_attrs_)
        misc_attrs_->requeue();

    for (auto& limit : limits_) {
        limit->reset();
    }
    inLimitMgr_.reset(); // new to 5.0.0 clear inlimit.incremented() flag

    // ECFLOW-196, ensure the re-queue release tokens held by Limits higher up the tree.
    // Note: Its safe to call decrementInLimit, even when no limit consumed
    std::set<Limit*> limitSet;  // ensure local limit have preference over parent
    decrementInLimit(limitSet); // will recurse up, expensive but needed
}

void Node::reset_late_event_meters() {
    if (late_)
        late_->reset();
    for (auto& meter : meters_) {
        meter.reset();
    }
    for (auto& event : events_) {
        event.reset();
    }
}

void Node::reset() {
    // Set the state without causing any side effects
    initState(1);

    clearTrigger();
    clearComplete();

    repeat_.reset(); // if repeat is empty reset() does nothing

    for (auto& today : todays_) {
        today.resetRelativeDuration();
        today.reset_only();
    }
    for (auto& time : times_) {
        time.resetRelativeDuration();
        time.reset_only();
    }
    for (auto& cron : crons_) {
        cron.resetRelativeDuration();
        cron.reset_only();
    }
    for (auto& date : dates_) {
        date.reset();
    }
    for (auto& day : days_) {
        day.reset();
    }

    flag_.reset();

    if (late_)
        late_->reset();

    for (auto& meter : meters_) {
        meter.reset();
    }
    for (auto& event : events_) {
        event.reset();
    }
    if (isNodeContainer()) {
        for (auto& label : labels_) {
            label.reset();
        }
    }

    for (auto& limit : limits_) {
        limit->reset();
    }
}

void Node::handle_migration(const ecf::Calendar& c) {
    // called when defs created by reading from a file on disk. i.e. checkpoint
    // handle any migration
    for (auto& day : days_) {
        day.handle_migration(c);
    }
}

void Node::requeue_time_attrs() {
    // Note: we *don't* mark hybrid time dependencies as complete.
    //       i.e. since this is called during alter command, it could be that
    //        the task is in a submitted or active state.
    do_requeue_time_attrs(true /*reset_next_time_slot*/, true /*reset_relative_duration*/, Requeue_args::FULL);
}

void Node::requeue_labels() {
    // ECFLOW-195, clear labels before a task is run.
    for (auto& label : labels_) {
        label.reset();
    }
}

// #define DEBUG_DAYS 1
bool Node::calendarChanged(const ecf::Calendar& c,
                           Node::Calendar_args& cal_args,
                           const ecf::LateAttr*,
                           bool holding_parent_day_or_date) {
    // cout << " Node:calendarChanged: " << debugNodePath() << " holding_parent_day_or_date " <<
    // holding_parent_day_or_date << "\n";

    if (!holding_parent_day_or_date) {
        holding_parent_day_or_date = calendar_changed_timeattrs(c, cal_args);

#ifdef DEBUG_DAYS
        if (holding_parent_day_or_date) {
            cout << " Node::calendarChanged: " << debugNodePath() << " SETTING holding_parent_day_or_date "
                 << holding_parent_day_or_date << " ***********\n";
        }
#endif
    }
#ifdef DEBUG_DAYS
    else {
        cout << " Node::calendarChanged: " << debugNodePath()
             << " IGNORING since holding_parent_day_or_date  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
    }
#endif

    if (checkForAutoCancel(c)) {
        cal_args.auto_cancelled_nodes_.push_back(shared_from_this());
    }

    // Avoid automatically archiving a restored node. Wait till begin/re-queue
    if (!flag().is_set(ecf::Flag::RESTORED) && check_for_auto_archive(c)) {
        cal_args.auto_archive_nodes_.push_back(shared_from_this());
    }
    return holding_parent_day_or_date;
}

void Node::check_for_lateness(const ecf::Calendar& c, const ecf::LateAttr* inherited_late) {
    // Late flag should ONLY be set on Submittable
    if (late_) {
        // Only check for lateness if we are not late.
        if (!late_->isLate()) {
            if (!inherited_late || inherited_late->isNull())
                checkForLateness(c);
            else {
                LateAttr overridden_late = *inherited_late;
                overridden_late.override_with(late_.get());
                if (overridden_late.check_for_lateness(st_, c)) {
                    late_->setLate(true);
                    flag().set(ecf::Flag::LATE);
                }
            }
        }
    }
    else {
        // inherited late, we can only set late flag.
        if (inherited_late && !flag().is_set(ecf::Flag::LATE) && inherited_late->check_for_lateness(st_, c)) {
            flag().set(ecf::Flag::LATE);
        }
    }
}

void Node::checkForLateness(const ecf::Calendar& c) {
    if (late_ && late_->check_for_lateness(st_, c)) {
        late_->setLate(true);
        flag().set(ecf::Flag::LATE);
        // cout << "Node::checkForLateness late flag set on " << absNodePath() << "\n";
    }
}

void Node::initState(int clear_suspended_in_child_nodes, bool log_state_changes) {
    // The state duration is ONLY updated *IF* state has changed.
    // However, on re-queue *ALWAYS* reset state time.
    // Otherwise, we can end up showing time in the future. SEE ECFLOW-1215
    Suite* theSuite = suite();
    if (theSuite) {
        const Calendar& calendar = theSuite->calendar();
        st_.second               = calendar.duration();
    }

    if (d_st_ == DState::SUSPENDED) {
        /// Note: DState::SUSPENDED is not a real state, its really a user interaction
        /// Replace with suspend, and set underlying state as queued
        suspend();
        setStateOnly(NState::QUEUED, false /*force*/, Str::EMPTY() /* additional info to log */, log_state_changes);
    }
    else {

        if (clear_suspended_in_child_nodes > 0) {
            resume();
        }

        // convert DState --> NState.
        // NOTE::  NState does *NOT* have SUSPENDED
        setStateOnly(DState::convert(d_st_.state()),
                     false /*force*/,
                     Str::EMPTY() /* additional info to log */,
                     log_state_changes);
    }
}

void Node::requeueOrSetMostSignificantStateUpNodeTree() {
    // Get the computed state of my immediate children
    // *** A family can be marked as complete, via complete trigger when not all its children
    // *** are complete, hence computedState() *MUST* first check the immediate state  ,
    // *** before considering its immediate children
    NState::State computedStateOfImmediateChildren = computedState(Node::IMMEDIATE_CHILDREN);

#ifdef DEBUG_REQUEUE
    LogToCout toCoutAsWell;
    cout << "\n";
    Indentor indent;
    LOG(Log::DBG,
        "requeueOrSetMostSignificantStateUpNodeTree() " << debugNodePath() << "(" << NState::toString(state())
                                                        << ") computed("
                                                        << NState::toString(computedStateOfImmediateChildren) << ")");
#endif

    if (computedStateOfImmediateChildren == NState::COMPLETE) {

        // set most significant state of my immediate children
        // Record: That Suite/Family completed.
        if (computedStateOfImmediateChildren != state()) {
            setStateOnly(computedStateOfImmediateChildren);
        }

        // For automated re-queue do *not* clear suspended state in *child* nodes
        int clear_suspended_in_child_nodes = -1;

        if (!repeat_.empty()) {

            repeat_.increment();

            // If the repeat is still valid, re-queue the node
#ifdef DEBUG_REQUEUE
            LOG(Log::DBG,
                "requeueOrSetMostSignificantStateUpNodeTree " << debugNodePath() << " for repeat "
                                                              << repeat_.toString());
#endif
            if (repeat_.valid()) {
#ifdef DEBUG_REQUEUE
                LOG(Log::DBG,
                    "requeueOrSetMostSignificantStateUpNodeTree() VALID for requeue "
                        << debugNodePath() << " for repeat at " << repeat_.toString());
#endif

                // Remove effects of RUN and Force complete interactive commands
                // For automated re-queue *DUE* to Repeats, *CLEAR* any user interaction that would miss the next time
                // slots. *Down* the hierarchy This handles the case where a user, has manually intervened (i.e. via run
                // or complete) and we had a time attribute That time attribute will have expired, typically we show
                // next day. In the case where we have a parent repeat we need to clear the flag, otherwise the
                // task/family with time based attribute would wait for next day.
                Node::Requeue_args args(Node::Requeue_args::REPEAT_INCREMENT,
                                        false /* don't reset repeats */,
                                        clear_suspended_in_child_nodes,
                                        true /* reset_next_time_slot */,
                                        true /* reset relative duration */);
                requeue(args);
                set_most_significant_state_up_node_tree();
                return;
            }
        }

        /// If user has *INTERACTIVLY* forced changed in state to complete *OR* run the job.
        /// This would cause Node to miss the next time slot. i.e. expire the time slot
        /// In which case testTimeDependenciesForRequeue should return false for a single time/today dependency
        /// and not requeue the node.
        if (has_time_dependencies() && testTimeDependenciesForRequeue()) {

            // This is the only place we do not explicitly reset_next_time_slot
            bool reset_next_time_slot = false;

            // Remove effects of RUN and Force complete interactive commands, *BUT* only if *not* applied to this cron
            if (!crons().empty()) {
                if (!flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP)) {
                    reset_next_time_slot = true;
                }
            }

            Node::Requeue_args args(Node::Requeue_args::TIME,
                                    false /* don't reset repeats */,
                                    clear_suspended_in_child_nodes,
                                    reset_next_time_slot,
                                    false /*  don't reset relative duration */);
            requeue(args); // time +00:01 00:07 00:03 # here task re-queued many times, relative time must be preserved.
            set_most_significant_state_up_node_tree();
            return;
        }
    }

    // In case compute state is other that COMPLETE, update. i,e for Family/Suite
    if (computedStateOfImmediateChildren != state()) {
        setStateOnly(computedStateOfImmediateChildren);
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

void Node::set_most_significant_state_up_node_tree() {
    if (isTask()) {
        parent()->set_most_significant_state_up_node_tree();
        return;
    }

    // set most significant state  of my immediate children
    NState::State computedStateOfImmediateChildren = computedState(Node::IMMEDIATE_CHILDREN);
    if (computedStateOfImmediateChildren != state()) {
        setStateOnly(computedStateOfImmediateChildren);
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

// Returning false, *STOPS* further traversal *DOWN* the node tree
bool Node::resolveDependencies(JobsParam& jobsParam) {
    // This function is called:
    //    a/ Periodically by the server, i.e. every minute
    //    b/ Asynchronously, after child command, via job submission
#ifdef DEBUG_DEPENDENCIES
    LogToCout toCoutAsWell;
    cout << "\n";
    LOG(Log::DBG,
        "   " << debugNodePath() << "::resolveDependencies " << NState::toString(state()) << " AT "
              << suite()->calendar().toString());
#endif

    // A node that is archived should not allow any change of state.
    if (get_flag().is_set(ecf::Flag::ARCHIVED))
        return false;

    if (isSuspended()) {
#ifdef DEBUG_DEPENDENCIES
        LOG(Log::DBG,
            "   Node::resolveDependencies() " << absNodePath() << " HOLDING as node state " << NState::toString(state())
                                              << " is SUSPENDED ");
#endif
        return false;
    }

    if (state() == NState::COMPLETE) {
#ifdef DEBUG_DEPENDENCIES
        LOG(Log::DBG,
            "   Node::resolveDependencies() " << absNodePath() << " HOLDING as node state " << NState::toString(state())
                                              << " is not valid for job submission");
#endif
        return false;
    }

    if (!timeDependenciesFree()) {
#ifdef DEBUG_DEPENDENCIES
        const Calendar& calendar = suite()->calendar();
        LOG(Log::DBG,
            "   Node::resolveDependencies() " << absNodePath() << " HOLDING due to time dependencies at "
                                              << calendar.toString());
#endif
        return false;
    }

    // Complete *MUST* be evaluated before trigger. As it can affect the other
    // i.e. A state change to COMPLETE, in which case no need to submit tasks
    // However if the complete does *not* evaluate it should *NOT* hold the node.
    if (evaluateComplete()) {
        if (completeAst()) {

            flag().set(ecf::Flag::BYRULE);

            // If we are a parent sets the state first. then set state on all the children
            set_state_hierarchically(NState::COMPLETE,
                                     false); // reset try no and decrement inlimit resources & update repeats

#ifdef DEBUG_DEPENDENCIES
            LOG(Log::DBG,
                "   Node::resolveDependencies() " << absNodePath() << " HOLDING since evaluation of COMPLETE");
#endif
            // We have a complete By returning false here, we stop dependency evaluation for any children
            return false;
        }
    }
    // No complete, or we have a complete  that does not evaluate.(this should not hold the node)

    if (evaluateTrigger()) {
        // WE only get here **IF** :
        // 1/ There is no trigger
        // 2/ WE have a trigger, and it evaluates to true
#ifdef DEBUG_DEPENDENCIES
        LOG(Log::DBG, "   Node::resolveDependencies() " << absNodePath() << " FREE of TRIGGER");
#endif
        return true;
    }

    // We *have* a trigger, and it does not evaluate, hold the node
#ifdef DEBUG_DEPENDENCIES
    LOG(Log::DBG, "   Node::resolveDependencies() " << absNodePath() << " HOLDING due to TRIGGER");
#endif
    return false;
}

bool Node::has_time_based_attributes() const {
    if (has_time_dependencies())
        return true;
    if (auto_cancel_)
        return true;
    if (auto_archive_)
        return true;
    if (late_)
        return true;
    return false;
}

void Node::freeTrigger() const {
    if (t_expr_)
        t_expr_->setFree();
}

void Node::clearTrigger() const {
    if (t_expr_)
        t_expr_->clearFree();
}

void Node::freeComplete() const {
    if (c_expr_)
        c_expr_->setFree();
}

void Node::clearComplete() const {
    if (c_expr_)
        c_expr_->clearFree();
}

bool Node::evaluateComplete() const {
    // Complete *MUST* be evaluated before trigger. As it can affect the other
    AstTop* theCompeteAst = completeAst();
    if (theCompeteAst) {
        // *NOTE* if we have a non-NULL complete ast, we must have complete expression
        // The freed state is stored on the expression ( i.e. not on the ast)
        // ISSUE: Complete expression cannot be by-passed in the GUI
        if (c_expr_->isFree() || theCompeteAst->evaluate()) {

            // Note: if a task has been set complete, the use may decide to place into queued state( via GUI)
            //       In which case, we *want* this complete expression to be re-evaluated.
            //       Hence, the old code below has been commented out.
            // >>old: Set the complete as free, until begin()/requeue, // Only update state change no, if state has
            // changed.
            // >>old:if (!c_expr_->isFree()) freeComplete();

            // ECFLOW-247 Family goes complete despite active child
            //            Typically we have a complete expression on the family i/e f1/t1 == complete
            //            However if we have another child that is active,submitted forcing family to complete will
            //            cause zombies Another child could be a family which could be aborted.
            // if computedState state is:
            //    NState::ABORTED   -> don't complete if any of the children are aborted  -> ECFLOW-247
            //                         This can hide active/submitted nodes, as abort has higher priority, could cause
            //                         zombies
            //    NState::ACTIVE    -> can cause zombies
            //    NState::SUBMITTED -> can cause zombies
            // hence only allow complete is we are in a NState::QUEUED state
            NState::State theComputedState = computedState(Node::HIERARCHICAL);
            if (theComputedState != NState::QUEUED) {
#ifdef DEBUG_DEPENDENCIES
                LOG(Log::DBG,
                    "   Node::evaluateComplete() " << absNodePath() << " AST evaluation succeeded *BUT* " << debugType()
                                                   << " has children in ACTIVE or SUBMITTED States");
#endif
                return false;
            }

#ifdef DEBUG_DEPENDENCIES
            LOG(Log::DBG,
                "   Node::evaluateComplete() " << debugNodePath() << " FREE, COMPLETE AST evaluation succeeded ");
#endif
            return true;
        }
        else {
            /// *IMPORTANT* When a complete does not evaluate, it should *NOT* stop further tree walking
#ifdef DEBUG_DEPENDENCIES
            LOG(Log::DBG,
                "   Node::evaluateComplete() " << debugNodePath() << " HOLDING, COMPLETE AST evaluation failed");
#endif
            return false;
        }
    }
    return true;
}

bool Node::evaluateTrigger() const {
    AstTop* theTriggerAst = triggerAst();
    if (theTriggerAst) {

        // Note 1: A trigger can be freed by the ForceCmd
        // Note 2: if we have a non-NULL trigger ast, we must have trigger expression
        // Note 3: The freed state is stored on the expression ( i.e. *NOT* on the ast (abstract syntax tree) )
        if (t_expr_->isFree() || theTriggerAst->evaluate()) {

            // *ALWAYS* evaluate trigger expression unless user has forcibly removed trigger dependencies
            // ******** This allows force queued functionality, to work as expected, since trigger's will be honoured
            // The old code below has been commented out.
            // >> old: Set the trigger as free, until begin()/requeue.  Only update state change no, when required
            // >> old: if (!t_expr_->isFree()) freeTrigger();

#ifdef DEBUG_DEPENDENCIES
            LOG(Log::DBG,
                "   Node::evaluateTrigger() " << debugNodePath() << " FREE, TRIGGER AST evaluation succeeded");
#endif
            return true;
        }

#ifdef DEBUG_DEPENDENCIES
        LOG(Log::DBG, "   Node::evaluateTrigger() " << debugNodePath() << " HOLDING  TRIGGER AST evaluation failed");
#endif
        return false; // evaluation failed. this Node holds
    }

#ifdef DEBUG_DEPENDENCIES
    LOG(Log::DBG, "   Node::evaluateTrigger() " << debugNodePath() << " FREE NO TRIGGER defined");
#endif
    return true;
}

const std::string& Node::abortedReason() const {
    return Str::EMPTY();
}

void Node::set_state(NState::State newState, bool force, const std::string& additional_info_to_log) {
    if (st_.first.state() != newState) {

        setStateOnly(newState, false, additional_info_to_log);

        // Handle any state change specific functionality. This will update any repeats
        // This is a virtual function, since we want different behaviour during state change
        handleStateChange();
    }
}

void Node::handleStateChange() {
    if (state() == NState::COMPLETE) {
        if (auto_restore_)
            auto_restore_->do_autorestore();

        Node* the_parent = parent();
        while (the_parent) {
            if (the_parent->state() == NState::COMPLETE && the_parent->get_autorestore()) {
                the_parent->get_autorestore()->do_autorestore();
            }
            the_parent = the_parent->parent();
        }
    }
}

void Node::setStateOnly(NState::State newState,
                        bool force,
                        const std::string& additional_info_to_log,
                        bool do_log_state_changes) {
    if (st_.first.state() == newState) {
        return; // if old and new state the same don't do anything
    }

    Suite* theSuite          = suite();
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

        if (newState == NState::SUBMITTED)
            submit_to_complete_duration_ = Calendar::second_clock_time();
        else if (newState == NState::COMPLETE && !submit_to_complete_duration_.is_special()) {

            // Under HYBRID we can go from UNKNOWN->COMPLETE, missing out SUBMITTED
            // hence: submit_to_complete_duration_ is never initialised
            // ie when we have a,date,cron dependency that relies on a day change

            time_duration td = (Calendar::second_clock_time() - submit_to_complete_duration_);
            // cout << debugNodePath() << " submit->active->complete time = " << td.total_seconds()  << " seconds.\n";

            // Avoid this check if we have meters. as we wait a second between each meter update
            if (td.total_seconds() >= jobSubmissionInterval && meters_.empty()) {

                const Variable& do_check = theSuite->findVariable("CHECK_TASK_DURATION_LESS_THAN_SERVER_POLL");
                if (!do_check.empty()) {
                    // 				cout << "Calendar::second_clock_time() = " <<
                    // to_simple_string(Calendar::second_clock_time())
                    // << "\n"; 				cout << "submit_to_complete_duration_ = " <<
                    // to_simple_string(submit_to_complete_duration_) << "\n"; 				cout <<
                    // "(Calendar::second_clock_time() - submit_to_complete_duration_) = " << to_simple_string(td) <<
                    // "\n";
                    cout << "Testing::" << debugNodePath() << " For each job submission interval of "
                         << jobSubmissionInterval << " seconds, the calendar is increment by 60 seconds.\n"
                         << " The job submission interval is too small as it takes " << td.total_seconds()
                         << " seconds for state change of submit->active->complete, for an empty job.\n"
                         << " Please increase the job submission interval to at least " << (td.total_seconds() + 1)
                         << " seconds.\n";
                }
            }
        }
    }
#endif

    // Change format is significant it is used in verification of log files
    // Please change/update LogVerification::extractNodePathAndState() all verification relies on this one function
    //           " " +  submitted(max) + ": " + path(estimate)  + " try-no: " + try_no(estimate)  + " reason: " +
    //           reason(estimate)
    // reserve : 1   +  9              + 2    + 100             + 9           + 3                 + 9           + 12   =
    // 145
    std::string log_state_change;
    if (do_log_state_changes) {
        log_state_change.reserve(145 + additional_info_to_log.size());
        log_state_change += " ";
        log_state_change += NState::toString(newState);
        log_state_change += ": ";
        log_state_change += absNodePath();
        if (!additional_info_to_log.empty()) {
            log_state_change += " ";
            log_state_change += additional_info_to_log;
        }
    }

    if (newState == NState::ABORTED) {
        if (force)
            flag().set(ecf::Flag::FORCE_ABORT);
        Submittable* submittable = isSubmittable();
        if (submittable) {
            flag().set(ecf::Flag::TASK_ABORTED);
            if (do_log_state_changes) {
                log_state_change += " try-no: ";
                log_state_change += submittable->tryNo();
                log_state_change += " reason: ";
                log_state_change += abortedReason();
            }
        }
    }
    else {
        flag().clear(ecf::Flag::TASK_ABORTED);
        flag().clear(ecf::Flag::FORCE_ABORT);
    }

    if (do_log_state_changes) {
        // SUP-408 what does submitted mean in log?
        // We want to minimize the calls to create a new time stamp in the log file.
        // A time stamp is automatically created, whenever a *new* client request is received, & then cached
        // However we can get a change in state, during tree traversal, when a node is free of its dependencies
        // If we were to just log the message it would use the last cached time stamp. Giving misleading info:
        // Since state changes are bubbled up, we need only update the time stamp for tasks, when not in a command
        if (!CmdContext::in_command() && isTask() && Log::instance()) {
            // std::cout << "!!!!! NOT in cmd context updating time stamp before logging\n";
            Log::instance()->cache_time_stamp();
        }
        ecf::log(Log::LOG, log_state_change); // Note: log type, must be same for debug & release for test, i.e. for log
                                              // file verification
    }

    st_.first.setState(newState); // this will update state_change_no
    if (newState == NState::QUEUED)
        sc_rt_ = boost::posix_time::time_duration(0, 0, 0, 0);
    else
        sc_rt_ = calendar.duration() - st_.second;
    st_.second = calendar.duration(); // record state change duration for late, autocancel,autoarchive,etc

    // Record state changes for verification
    if (misc_attrs_) {
        for (auto& verify : misc_attrs_->verifys_) {
            if (verify.state() == newState) {
                verify.incrementActual();
            }
        }
    }
}

boost::posix_time::ptime Node::state_change_time() const {
    const Calendar& calendar                       = suite()->calendar();
    boost::posix_time::ptime the_state_change_time = calendar.begin_time();
    the_state_change_time += st_.second; // st_.second is calendar duration relative to calendar begin_time
    return the_state_change_time;
}

DState::State Node::dstate() const {

    // ECFLOW-139, check for suspended must be done first
    if (isSuspended())
        return DState::SUSPENDED;

    switch (state()) {
        case NState::COMPLETE:
            return DState::COMPLETE;
            break;
        case NState::ABORTED:
            return DState::ABORTED;
            break;
        case NState::ACTIVE:
            return DState::ACTIVE;
            break;
        case NState::SUBMITTED:
            return DState::SUBMITTED;
            break;
        case NState::QUEUED:
            return DState::QUEUED;
            break;
        case NState::UNKNOWN:
            return DState::UNKNOWN;
            break;
    }
    return DState::UNKNOWN;
}

bool Node::set_event(const std::string& event_name_or_number) {
    auto found = ecf::algorithm::find_by(
        events_, [&](const auto& item) { return item.name_or_number() == event_name_or_number; });

    if (found == std::end(events_)) {
        return false;
    }

    found->set_value(true);
    return true;
}

bool Node::clear_event(const std::string& event_name_or_number) {
    auto found = ecf::algorithm::find_by(
        events_, [&](const auto& item) { return item.name_or_number() == event_name_or_number; });

    if (found == std::end(events_)) {
        return false;
    }

    found->set_value(false);
    return true;
}

void Node::setRepeatToLastValue() {
    repeat_.setToLastValue(); // no op for empty repeat
    repeat_.increment();      // make repeat invalid
}

bool Node::check_in_limit_up_node_tree() const {
    if (!inLimitMgr_.inLimit())
        return false;

    Node* theParent = parent();
    while (theParent) {
        if (!theParent->inLimitMgr_.inLimit())
            return false;
        theParent = theParent->parent();
    }
    return true;
}

void Node::incrementInLimit(std::set<Limit*>& limitSet) {
    // cout << "Node::incrementInLimit " << absNodePath() << endl;
    std::string the_abs_node_path = absNodePath();
    inLimitMgr_.incrementInLimit(limitSet, the_abs_node_path);

    Node* theParent = parent();
    while (theParent) {
        theParent->inLimitMgr_.incrementInLimit(limitSet, the_abs_node_path);
        theParent = theParent->parent();
    }
}

void Node::decrementInLimit(std::set<Limit*>& limitSet) {
    // cout << "Node::decrementInLimit " << absNodePath() << endl;
    std::string the_abs_node_path = absNodePath();
    inLimitMgr_.decrementInLimit(limitSet, the_abs_node_path);

    Node* theParent = parent();
    while (theParent) {
        theParent->inLimitMgr_.decrementInLimit(limitSet, the_abs_node_path);
        theParent = theParent->parent();
    }
}

void Node::decrementInLimitForSubmission(std::set<Limit*>& limitSet) {
    // cout << "Node::decrementInLimit " << absNodePath() << endl;
    std::string the_abs_node_path = absNodePath();
    inLimitMgr_.decrementInLimitForSubmission(limitSet, the_abs_node_path);

    Node* theParent = parent();
    while (theParent) {
        theParent->inLimitMgr_.decrementInLimitForSubmission(limitSet, the_abs_node_path);
        theParent = theParent->parent();
    }
}

static bool
search_user_edit_variables(const std::string& name, std::string& value, const NameValueMap& user_edit_variables) {
    auto i = user_edit_variables.find(name);
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

// #define DEBUG_S 1
bool Node::variableSubstitution(std::string& cmd) const {
    char micro = '%';
    std::string micro_char;
    findParentUserVariableValue(Str::ECF_MICRO(), micro_char);
    if (!micro_char.empty() && micro_char.size() == 1) {
        micro = micro_char[0];
    }

    NameValueMap user_edit_variables;
    return variable_substitution(cmd, user_edit_variables, micro);
}

bool Node::variable_substitution(std::string& cmd, const NameValueMap& user_edit_variables, char micro) const {
    // scan the command for variables, and substitute
    // edit cmd "/home/ma/map/sms/smsfectch -F %ECF_FILES% -I %ECF_INCLUDE%"
    // We can also have
    //
    // "%<VAR>:<substitute>% i.e. if VAR exist use it, else use substitute
    //
    // ************************************************************************************************************
    // Special case handling for user variables, and generated variables, which take precedence over node variables
    // ************************************************************************************************************
    //
    // i.e. VAR is defined as BILL
    //  %VAR:fred --f%  will either be "BILL" or if VAR is not defined "fred --f"
    //
    // Infinite recursion. It is possible to end up with infinite recursion:
    //   	edit hello '%hello%'  # string like %hello% will cause infinite recursion
    //   	edit fred '%bill%'
    //	 	edit bill '%fred%'   # should be 10
    // To prevent this we will use a simple count
#ifdef DEBUG_S
    cout << "cmd  = " << cmd << "\n";
#endif
    bool double_micro_found    = false;
    std::string::size_type pos = 0;
    int count                  = 0;
    Alias* is_a_alias          = isAlias();
    while (true) {
        // A while loop here is used to:
        //   a/ Allow for multiple substitution on a single line. i.e. `%ECF_FILES% -I %ECF_INCLUDE%`
        //   b/ Allow for recursive substitution. %fred% -> %bill% -> 10

        size_t firstPercentPos = cmd.find(micro, pos);
        if (firstPercentPos == string::npos)
            break;

        size_t secondPercentPos = cmd.find(micro, firstPercentPos + 1);
        if (secondPercentPos == string::npos)
            break;

        pos = 0;
        if (secondPercentPos - firstPercentPos <= 1) {
            // handle %% with no characters in between, skip over
            // i.e. to handle "printf %%02d %HOUR:00%" --> "printf %02d 00" i.e. if HOUR not defined
            pos                = secondPercentPos + 1;
            double_micro_found = true;
            continue;
        }

        string percentVar(cmd.begin() + firstPercentPos + 1, cmd.begin() + secondPercentPos);
        // cout << percentVar << "\n";
#ifdef DEBUG_S
        cout << "   Found percentVar " << percentVar << "\n";
#endif

        // ****************************************************************************************
        // Look for generated variables that should NOT be overridden first:
        //    Variable like ECF_PASS can be overridden, i.e. with FREE_JOBS_PASSWORD
        //    However for job file generation we should use the generated variables first.
        //    if the user removes ECF_PASS then we are stuck with the wrong value in the script file
        //    FREE_JOBS_PASSWORD is left for the server to deal with
        // Leave ECF_JOB and ECF_JOBOUT out of this list: As user may legitimately override these. ECFLOW-999
        bool generated_variable = false;
        if (percentVar.find("ECF_") == 0) {
            if (percentVar.find(Str::ECF_HOST()) != std::string::npos)
                generated_variable = true;
            else if (percentVar.find(Str::ECF_PORT()) != std::string::npos)
                generated_variable = true;
            else if (percentVar.find(Str::ECF_TRYNO()) != std::string::npos)
                generated_variable = true;
            else if (percentVar.find(Str::ECF_NAME()) != std::string::npos)
                generated_variable = true;
            else if (percentVar.find(Str::ECF_PASS()) != std::string::npos)
                generated_variable = true;
        }

        size_t firstColon = percentVar.find(':');

        // First search user variable (*ONLY* set user edit's the script)
        // Handle case: cmd = "%fred:bill% and where we have user variable "fred:bill"
        // Handle case: cmd = "%fred%      and where we have user variable "fred"
        // If we fail to find the variable we return false.
        // Note: When a variable is found, it can have an empty value  which is still valid
        std::string varValue;
        if (!user_edit_variables.empty() && search_user_edit_variables(percentVar, varValue, user_edit_variables)) {
            cmd.replace(firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue);
        }
        else if (generated_variable && firstColon == string::npos &&
                 find_parent_gen_variable_value(percentVar, varValue)) {
            cmd.replace(firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue);
        }
        else {
            if (firstColon != string::npos) {

                if (is_a_alias && findParentVariableValue(percentVar, varValue)) {
                    // For alias, we could have added variables with %A:0%, %A:1%.
                    // Aliases allow variables with ':' in the name.
                    cmd.replace(firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue);
                }
                else {
                    // ':' is not a valid in variables, hence split, and search, if search fails use replacement
                    string var(percentVar.begin(), percentVar.begin() + firstColon);
#ifdef DEBUG_S
                    cout << "   var " << var << "\n";
#endif
                    if (!user_edit_variables.empty() &&
                        search_user_edit_variables(var, varValue, user_edit_variables)) {
#ifdef DEBUG_S
                        cout << "   user var value = " << varValue << "\n";
#endif
                        cmd.replace(firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue);
                    }
                    else if (generated_variable && find_parent_gen_variable_value(var, varValue)) {
#ifdef DEBUG_S
                        cout << "   generated var value = " << varValue << "\n";
#endif
                        cmd.replace(firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue);
                    }
                    else if (findParentVariableValue(var, varValue)) {
                        // Note: variable can exist, but have an empty value
#ifdef DEBUG_S
                        cout << "   var value = " << varValue << "\n";
#endif
                        // replace the "%VAR:fred --f%" with var
                        cmd.replace(firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue);
                    }
                    else {
                        string substitute(percentVar.begin() + firstColon + 1, percentVar.end());
#ifdef DEBUG_S
                        cout << "  substitute value = " << substitute << "\n";
#endif
                        cmd.replace(firstPercentPos, secondPercentPos - firstPercentPos + 1, substitute);
                    }
#ifdef DEBUG_S
                    cout << "   cmd = " << cmd << "\n";
#endif
                }
            }
            else if (findParentVariableValue(percentVar, varValue)) {
                // No ':' search user variables, repeat, and then generated variables.
                cmd.replace(firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue);
            }
            else {
                // Can't find in user variables, or node variable, hence can't go any further
                return false;
            }
        }

        // Simple Check for infinite recursion
        if (count > 1000)
            return false;
        count++;
    }

    if (double_micro_found) {
        // replace all double micro with a single micro, this must be a single parse
        // date +%%Y%%m%%d" ==> date +%Y%m%d
        // %%%%             ==> %%            // i.e. single parse
        std::string doubleEcfMicro;
        doubleEcfMicro += micro;
        doubleEcfMicro += micro;
        size_t last_pos = 0;
        while (true) {
            string::size_type ecf_double_micro_pos = cmd.find(doubleEcfMicro, last_pos);
            if (ecf_double_micro_pos != std::string::npos) {
                cmd.erase(cmd.begin() + ecf_double_micro_pos);
                last_pos = ecf_double_micro_pos + 1;
            }
            else
                break;
        }
    }

    return true;
}

bool Node::find_all_used_variables(std::string& cmd, NameValueMap& used_variables, char micro) const {
#ifdef DEBUG_S
    cout << "cmd  = " << cmd << "\n";
#endif
    int count = 0;
    while (true) {
        // A while loop here is used to:
        //   a/ Allow for multiple substitution on a single line. i.e. `%ECF_FILES% -I %ECF_INCLUDE%`
        //   b/ Allow for recursive substitution. %fred% -> %bill% -> 10

        size_t firstPercentPos = cmd.find(micro);
        if (firstPercentPos == string::npos)
            break;
        size_t secondPercentPos = cmd.find(micro, firstPercentPos + 1);
        if (secondPercentPos == string::npos)
            break;
        if (secondPercentPos - firstPercentPos <= 1)
            break; // handle %% with no characters in between

        string percentVar(cmd.begin() + firstPercentPos + 1, cmd.begin() + secondPercentPos);
#ifdef DEBUG_S
        cout << "   Found percentVar " << percentVar << "\n";
#endif

        size_t firstColon = percentVar.find(':');
        if (firstColon != string::npos) {

            string var(percentVar.begin(), percentVar.begin() + firstColon);
#ifdef DEBUG_S
            cout << "   var " << var << "\n";
#endif

            std::string varValue;
            if (findParentVariableValue(var, varValue)) {
                // Note: variable can exist, but have an empty value
#ifdef DEBUG_S
                cout << "   var value = " << theFoundVariable.value() << "\n";
#endif
                // %VAR:fred% --->  name("VAR:fred") value(theFoundVariable.value())
                used_variables.insert(std::make_pair(percentVar, varValue));

                // replace the "%VAR:fred --f%" with variable value, so that we don't process it again
                cmd.replace(firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue);
            }
            else {
                string substitute(percentVar.begin() + firstColon + 1, percentVar.end());
#ifdef DEBUG_S
                cout << "  substitute value = " << substitute << "\n";
#endif

                // %VAR:fred% --->  name("VAR:fred") value(fred)
                used_variables.insert(std::make_pair(percentVar, substitute));

                cmd.replace(firstPercentPos, secondPercentPos - firstPercentPos + 1, substitute);
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
            if (!findParentVariableValue(percentVar, varValue))
                return false;

            used_variables.insert(std::make_pair(percentVar, varValue));
            cmd.replace(firstPercentPos, secondPercentPos - firstPercentPos + 1, varValue);
        }

        // Simple Check for infinite recursion
        if (count > 100)
            return false;
        count++;
    }
    return true;
}

bool Node::variable_dollar_substitution(std::string& cmd) const {
    // scan command for environment variables, and substitute
    // edit ECF_INCLUDE $ECF_HOME/include

    while (true) {
        size_t firstPos = cmd.find('$');
        if (firstPos == string::npos)
            break;

        size_t secondPos = cmd.find_first_not_of(Str::ALPHANUMERIC_UNDERSCORE(), firstPos + 1);
        if (secondPos == string::npos)
            secondPos = cmd.size();
        if (secondPos - firstPos <= 1)
            break; // handle $/ with no characters in between

        string env(cmd.begin() + firstPos + 1, cmd.begin() + secondPos);
        // cout << "find env " << env << "\n";

        std::string envValue;
        if (!findParentVariableValue(env, envValue)) {
            // cout << " could not find " << env << "\n";
            return false;
        }

        cmd.replace(firstPos, secondPos - firstPos, envValue);

        if (envValue.find(env) != std::string::npos) {
            // infinite loop
            break;
        }
    }
    return true;
}

std::string Node::completeExpression() const {
    if (c_expr_) {
        string ret = "complete ";
        ret += c_expr_->expression();
        return ret;
    }
    return string();
}

std::string Node::triggerExpression() const {
    if (t_expr_) {
        string ret = "trigger ";
        ret += t_expr_->expression();
        return ret;
    }
    return string();
}

bool Node::check_expressions(Ast* ast, const std::string& expr, bool trigger, std::string& errorMsg) const {
    if (ast) {
        // The expression has been parsed, and we have created the abstract syntax tree.
        // We now try to resolve the path/node references in the expressions.
        // And, also, resolve references to events, meter, repeats and variables.
        AstResolveVisitor astVisitor(this);
        ast->accept(astVisitor);
        if (!astVisitor.errorMsg().empty()) {
            errorMsg += "Error: Expression node tree references failed for '";
            if (trigger)
                errorMsg += "trigger ";
            else
                errorMsg += "complete ";
            errorMsg += expr;
            errorMsg += "' at ";
            errorMsg += debugNodePath();
            errorMsg += "\n ";
            errorMsg += astVisitor.errorMsg();
            return false;
        }

        // check divide by zero and module by zero
        if (!ast->check(errorMsg)) {
            errorMsg += " Error: Expression checking failed for '";
            if (trigger)
                errorMsg += "trigger ";
            else
                errorMsg += "complete ";
            errorMsg += expr;
            errorMsg += "' at ";
            errorMsg += debugNodePath();
            return false;
        }
    }
    return true;
}

std::unique_ptr<AstTop>
Node::parse_and_check_expressions(const std::string& expr, bool trigger, const std::string& context) {
    std::unique_ptr<AstTop> ast = Expression::parse(expr, context); // will throw for errors

    std::string errorMsg;
    if (!check_expressions(ast.get(), expr, trigger, errorMsg)) {
        std::stringstream ss;
        ss << context << " " << errorMsg;
        throw std::runtime_error(ss.str());
    }
    return ast;
}

bool Node::check(std::string& errorMsg, std::string& warningMsg) const {
    // #ifdef DEBUG
    //  	cout << "Node::check " << debugNodePath() << " complete and trigger\n";
    // #endif

    /// ************************************************************************************
    /// *IMPORTANT side effect: *
    /// The simulator relies AstResolveVisitor to set usedInTrigger() for events and meters
    /// *************************************************************************************

    /// Make Sure: To sure capture parser errors:
    /// defs which fail parse errors should not be allowed to be loaded into the server
    /// Even if the code parses, check the expression for divide by zero, for divide and modulo operators
    AstTop* ctop = completeAst(errorMsg);
    if (ctop) {
        // capture node path resolve errors, and expression divide/module by zero
        std::string expr;
        if (c_expr_)
            expr = c_expr_->expression();
        (void)check_expressions(ctop, expr, false, errorMsg);
    }

    AstTop* ttop = triggerAst(errorMsg);
    if (ttop) {
        std::string expr;
        if (t_expr_)
            expr = t_expr_->expression();
        (void)check_expressions(ttop, expr, true, errorMsg);
    }

    // check inLimit references to limits.
    // Client: Unresolved references, which are not in the externs reported as errors/warnings
    // Server: There are no externs, all unresolved references reported as errors
    bool reportErrors   = true;
    bool reportWarnings = true;
    inLimitMgr_.check(errorMsg, warningMsg, reportErrors, reportWarnings);

    /// Check that the references to nodes in autorestore resolve
    if (auto_restore_)
        auto_restore_->check(errorMsg);

    return errorMsg.empty();
}

void Node::add_comment_char(std::string& ret, bool& added_comment_char) const {
    if (!added_comment_char) {
        ret += " #";
        added_comment_char = true;
    }
}

void Node::write_state(std::string& ret, bool& added_comment_char) const {
    // *IMPORTANT* we *CANT* use ';' character, since is used in the parser, when we have
    //             multiple statement on a single line i.e. `task a; task b;`
    // If attribute correspond to the defaults don't write then out
    if (state() != NState::UNKNOWN) {
        add_comment_char(ret, added_comment_char);
        ret += " state:";
        ret += NState::toString(state());
    }
    if (st_.second.total_seconds() != 0) {
        add_comment_char(ret, added_comment_char);
        ret += " dur:";
        ret += to_simple_string(st_.second);
    }
    if (flag_.flag() != 0) {
        add_comment_char(ret, added_comment_char);
        ret += " flag:";
        flag_.write(ret);
    }
    if (suspended_) {
        add_comment_char(ret, added_comment_char);
        ret += " suspended:1";
    }
    if (!sc_rt_.is_special() && (sc_rt_.hours() != 0 || sc_rt_.minutes() != 0 || sc_rt_.seconds() != 0)) {
        add_comment_char(ret, added_comment_char);
        ret += " rt:";
        ret += to_simple_string(sc_rt_);
    }
}

void Node::read_state(const std::string& line, const std::vector<std::string>& lineTokens) {
    //  0    1   2
    // task name #
    std::string token;
    size_t line_tokens_size = lineTokens.size();
    for (size_t i = 3; i < line_tokens_size; i++) {
        token.clear();
        const std::string& line_token_i = lineTokens[i];
        if (line_token_i.find("state:") != std::string::npos) {
            if (!Extract::split_get_second(line_token_i, token))
                throw std::runtime_error("Node::read_state Invalid state specified for node " + name());
            std::pair<NState::State, bool> state_pair = NState::to_state(token);
            if (!state_pair.second)
                throw std::runtime_error("Node::read_state Invalid state specified for node : " + name());
            set_state_only(state_pair.first);
        }
        else if (line_token_i.find("flag:") != std::string::npos) {
            if (!Extract::split_get_second(line_token_i, token))
                throw std::runtime_error("Node::read_state invalid flags for node " + name());
            flag().set_flag(token); // this can throw
        }
        else if (line_token_i.find("dur:") != std::string::npos) {
            if (!Extract::split_get_second(line_token_i, token))
                throw std::runtime_error("Node::read_state invalid duration for node: " + name());
            st_.second = duration_from_string(token);
        }
        else if (line_token_i.find("rt:") != std::string::npos) {
            if (!Extract::split_get_second(line_token_i, token))
                throw std::runtime_error("Node::read_state invalid runtime duration for node: " + name());
            sc_rt_ = duration_from_string(token);
        }
        else if (line_token_i == "suspended:1")
            suspend();
    }
}

std::string Node::print() const {
    std::string s;
    print(s);
    return s;
}

void Node::print(std::string& os) const {
    if (d_st_ != DState::default_state()) {
        Indentor in;
        Indentor::indent(os);
        os += "defstatus ";
        os += DState::toString(d_st_);
        os += "\n";
    }

    if (late_)
        late_->print(os);

    if (c_expr_) {
        c_expr_->print(os, "complete");
        if (PrintStyle::getStyle() == PrintStyle::STATE) {
            Indentor in;
            if (c_expr_->isFree()) {
                Indentor::indent(os);
                os += "# (free)\n";
            }
            if (completeAst()) {
                if (!defs()) {
                    // Full defs is required for extern checking, and finding absolute node paths
                    // Hence print will with no defs can give in-accurate information
                    Indentor in2;
                    Indentor::indent(os);
                    os += "# Warning: Full/correct AST evaluation requires the definition\n";
                }
                std::stringstream ss;
                completeAst()->print(ss);
                os += ss.str();
            }
        }
    }
    if (t_expr_) {
        t_expr_->print(os, "trigger");
        if (PrintStyle::getStyle() == PrintStyle::STATE) {
            Indentor in;
            if (t_expr_->isFree()) {
                Indentor::indent(os);
                os += "# (free)\n";
            }
            if (triggerAst()) {
                if (!defs()) {
                    Indentor in2;
                    Indentor::indent(os);
                    os += "# Warning: Full/correct AST evaluation requires the definition\n";
                }
                std::stringstream ss;
                triggerAst()->print(ss);
                os += ss.str();
            }
        }
    }
    repeat_.print(os); // if repeat is empty print(..) does nothing

    for (const Variable& v : vars_) {
        v.print(os);
    }

    if (PrintStyle::getStyle() == PrintStyle::STATE) {
        // Distinguish normal variable from generated, by adding a #
        // This also allows it to be read in again and compared in the AParser/tests
        std::vector<Variable> gvec;
        gen_variables(gvec);
        for (const Variable& v : gvec) {
            v.print_generated(os);
        }
    }

    for (limit_ptr l : limits_) {
        l->print(os);
    }
    inLimitMgr_.print(os);

    for (const Label& la : labels_) {
        la.print(os);
    }
    for (const Meter& m : meters_) {
        m.print(os);
    }
    for (const Event& e : events_) {
        e.print(os);
    }

    for (const ecf::TimeAttr& t : times_) {
        t.print(os);
    }
    for (const ecf::TodayAttr& t : todays_) {
        t.print(os);
    }
    for (const DateAttr& date : dates_) {
        date.print(os);
    }
    for (const DayAttr& day : days_) {
        day.print(os);
    }
    for (const CronAttr& cron : crons_) {
        cron.print(os);
    }

    if (auto_cancel_)
        auto_cancel_->print(os);
    if (auto_archive_)
        auto_archive_->print(os);
    if (auto_restore_)
        auto_restore_->print(os);

    if (misc_attrs_)
        misc_attrs_->print(os);
}

std::string Node::print(PrintStyle::Type_t p_style) const {
    PrintStyle print_style(p_style);
    return to_string();
}

std::string Node::to_string() const {
    std::string ss;
    print(ss);
    return ss;
}

bool Node::operator==(const Node& rhs) const {
    if (n_ != rhs.n_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==( n_(" << n_ << ") != rhs.n_(" << rhs.n_ << ")) for: " << debugNodePath()
                      << "\n";
        }
#endif
        return false;
    }
    if (state() != rhs.state()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  state(" << NState::toString(state()) << ") != rhs.state("
                      << NState::toString(rhs.state()) << ")) " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    if (d_st_ != rhs.d_st_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  ( d_st_ != rhs.d_st_) " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    if (suspended_ != rhs.suspended_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator== suspended_ != rhs.suspended_ " << debugNodePath() << "\n";
        }
#endif
        return false;
    }

    if (flag_ != rhs.flag_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator== ( flag_ != rhs.flag_) : '" << flag_.to_string() << "' != '"
                      << rhs.flag_.to_string() << "' : " << debugNodePath() << "\n";
        }
#endif
        return false;
    }

    if ((t_expr_ && !rhs.t_expr_) || (!t_expr_ && rhs.t_expr_)) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  (t_expr_ && !rhs.t_expr_) || (!t_expr_&& rhs.t_expr_)  " << debugNodePath()
                      << "\n";
        }
#endif
        return false;
    }
    if (t_expr_ && rhs.t_expr_ && (*t_expr_ != *rhs.t_expr_)) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  t_expr_ && rhs.t_expr_ && (*t_expr_ != *rhs.t_expr_) " << debugNodePath()
                      << "\n";
        }
#endif
        return false;
    }

    if ((c_expr_ && !rhs.c_expr_) || (!c_expr_ && rhs.c_expr_)) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  (c_expr_ && !rhs.c_expr_) || (!c_expr_&& rhs.c_expr_)  " << debugNodePath()
                      << "\n";
        }
#endif
        return false;
    }
    if (c_expr_ && rhs.c_expr_ && (*c_expr_ != *rhs.c_expr_)) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  c_expr_ && rhs.c_expr_ && (*c_expr_ != *rhs.c_expr_) " << debugNodePath()
                      << "\n";
        }
#endif
        return false;
    }

    if (vars_.size() != rhs.vars_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  (vars_.size() != rhs.vars_.size()) " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    for (unsigned i = 0; i < vars_.size(); ++i) {
        if (!(vars_[i] == rhs.vars_[i])) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "Node::operator==  (!(vars_[i] == rhs.vars_[i] )) " << debugNodePath() << "\n";
                std::cout << "     vars_[i] name = '" << vars_[i].name() << "' value = '" << vars_[i].theValue()
                          << "'\n";
                std::cout << " rhs.vars_[i] name = '" << rhs.vars_[i].name() << "' value = '" << rhs.vars_[i].theValue()
                          << "'\n";
            }
#endif
            return false;
        }
    }
    // We don't compare `genvar` as this is only used in server environment

    if (!(inLimitMgr_ == rhs.inLimitMgr_)) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  (!(inLimitMgr == rhs.inLimitMgr)) " << debugNodePath() << "\n";
        }
#endif
        return false;
    }

    if (limits_.size() != rhs.limits_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  (limits_.size(" << limits_.size() << ") != rhs.limits_.size("
                      << rhs.limits_.size() << ")) " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    for (unsigned i = 0; i < limits_.size(); ++i) {
        if (!(*limits_[i] == *rhs.limits_[i])) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "Node::operator==  (!(*limits_[i] == *rhs.limits_[i] )) " << debugNodePath() << "\n";
            }
#endif
            return false;
        }
    }

    if (times_.size() != rhs.times_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  (times_.size() != rhs.times_.size()) " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    for (unsigned i = 0; i < times_.size(); ++i) {
        if (!(times_[i] == rhs.times_[i])) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "Node::operator==  (!(times_[i] == rhs.times_[i] ))  " << debugNodePath() << "\n";
            }
#endif
            return false;
        }
    }

    if (todays_.size() != rhs.todays_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  (todays_.size() != rhs.todays_.size()) " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    for (unsigned i = 0; i < todays_.size(); ++i) {
        if (!(todays_[i] == rhs.todays_[i])) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "Node::operator==  (!(todays_[i] == rhs.todays_[i] ))  " << debugNodePath() << "\n";
            }
#endif
            return false;
        }
    }

    if (dates_.size() != rhs.dates_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==   (dates_.size() != rhs.dates_.size()) " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    for (unsigned i = 0; i < dates_.size(); ++i) {
        if (!(dates_[i] == rhs.dates_[i])) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "Node::operator==   (!(dates_[i] == rhs.dates_[i]) " << debugNodePath() << "\n";
            }
#endif
            return false;
        }
    }

    if (days_.size() != rhs.days_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==   (days_.size() != rhs.days_.size()) " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    for (unsigned i = 0; i < days_.size(); ++i) {
        if (!(days_[i] == rhs.days_[i])) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "Node::operator==   (!(days_[i] == rhs.days_[i]) " << debugNodePath() << "\n";
            }
#endif
            return false;
        }
    }

    if (crons_.size() != rhs.crons_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==   (crons_.size() != rhs.crons_.size()) " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    for (unsigned i = 0; i < crons_.size(); ++i) {
        if (!(crons_[i] == rhs.crons_[i])) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "Node::operator==   (!(crons_[i] == rhs.crons_[i]) " << debugNodePath() << "\n";
            }
#endif
            return false;
        }
    }

    if (labels_.size() != rhs.labels_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  (labels_.size() != rhs.labels_.size()) " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    for (unsigned i = 0; i < labels_.size(); ++i) {
        if (labels_[i] != rhs.labels_[i]) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "Node::operator==  (labels_[i] != rhs.labels_[i]) " << debugNodePath() << "\n";
                std::cout << "   lhs = " << labels_[i].dump() << "\n";
                std::cout << "   rhs = " << rhs.labels_[i].dump() << "\n";
            }
#endif
            return false;
        }
    }

    if (meters_.size() != rhs.meters_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  (meters_.size() != rhs.meters_.size()) " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    for (size_t i = 0; i < meters_.size(); ++i) {
        if (!(meters_[i] == rhs.meters_[i])) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "Node::operator==   (!(meters_[i] == rhs.meters_[i] )) " << debugNodePath() << "\n";
                std::cout << "   " << meters_[i].dump() << "\n";
                std::cout << "   " << rhs.meters_[i].dump() << "\n";
            }
#endif
            return false;
        }
    }
    if (events_.size() != rhs.events_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==   (events_.size() != rhs.events_.size()) " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    for (size_t i = 0; i < events_.size(); ++i) {
        if (!(events_[i] == rhs.events_[i])) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "Node::operator==   (!(events_[i] == rhs.events_[i] )) " << debugNodePath() << "\n";
            }
#endif
            return false;
        }
    }

    if ((misc_attrs_ && !rhs.misc_attrs_) || (!misc_attrs_ && rhs.misc_attrs_)) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator== (( misc_attrs_ && !rhs.misc_attrs_) || ( !misc_attrs_ && rhs.misc_attrs_)) "
                      << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    if (misc_attrs_ && rhs.misc_attrs_ && !(*misc_attrs_ == *rhs.misc_attrs_)) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator== ( misc_attrs_ && rhs.misc_attrs_ && !(*misc_attrs_ == *(rhs.misc_attrs_))) "
                      << debugNodePath() << "\n";
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

    if ((late_ && !rhs.late_) || (!late_ && rhs.late_)) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator== (( late_ && !rhs.late_) || ( !late_ && rhs.late_)) " << debugNodePath()
                      << "\n";
        }
#endif
        return false;
    }
    if (late_ && rhs.late_ && !(*late_ == *rhs.late_)) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator== ( late_ &&   rhs.late_ && !(*late_ == *(rhs.late_))) " << debugNodePath()
                      << "\n";
        }
#endif
        return false;
    }

    if (auto_restore_ && rhs.auto_restore_) {
        if (*auto_restore_ == *rhs.auto_restore_) {
            return true;
        }
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator== auto_restore_ && rhs.auto_restore_   " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    else if (!auto_restore_ && rhs.auto_restore_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  !auto_restore_ && rhs.auto_restore_ " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    else if (auto_restore_ && !rhs.auto_restore_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  auto_restore_ && !rhs.auto_restore_  " << debugNodePath() << "\n";
        }
#endif
        return false;
    }

    if (auto_cancel_ && !rhs.auto_cancel_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  if (auto_cancel_ && !rhs.auto_cancel_)  " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    if (!auto_cancel_ && rhs.auto_cancel_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  if (!auto_cancel_ && rhs.auto_cancel_)  " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    if (auto_cancel_ && rhs.auto_cancel_ && !(*auto_cancel_ == *rhs.auto_cancel_)) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout
                << "Node::operator==  (auto_cancel_ && rhs.auto_cancel_ && !(*auto_cancel_ == *rhs.auto_cancel_)) "
                << debugNodePath() << "\n";
        }
#endif
        return false;
    }

    if (auto_archive_ && !rhs.auto_archive_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  if (auto_archive_ && !rhs.auto_archive_)  " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    if (!auto_archive_ && rhs.auto_archive_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "Node::operator==  if (!auto_archive_ && rhs.auto_archive_)  " << debugNodePath() << "\n";
        }
#endif
        return false;
    }
    if (auto_archive_ && rhs.auto_archive_ && !(*auto_archive_ == *rhs.auto_archive_)) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout
                << "Node::operator==  (auto_archive_ && rhs.auto_archive_ && !(*auto_archive_ == *rhs.auto_archive_)) "
                << debugNodePath() << "\n";
        }
#endif
        return false;
    }

    return true;
}

// #define DEBUG_WHY 1

bool Node::top_down_why(std::vector<std::string>& theReasonWhy, bool html_tags) const {
#ifdef DEBUG_WHY
    cout << "Node::top_down_why\n";
#endif
    return why(theReasonWhy, html_tags);
}

void Node::bottom_up_why(std::vector<std::string>& theReasonWhy, bool html_tags) const {
#ifdef DEBUG_WHY
    cout << "Node::bottom_up_why\n";
#endif

    defs()->why(theReasonWhy, html_tags);

    std::vector<Node*> vec;
    vec.push_back(const_cast<Node*>(this));
    Node* theParent = parent();
    while (theParent) {
        vec.push_back(theParent);
        theParent = theParent->parent();
    }
    auto r_end = vec.rend();
    for (auto r = vec.rbegin(); r != r_end; ++r) {
        (void)(*r)->why(theReasonWhy, html_tags);
    }
}

bool Node::why(std::vector<std::string>& vec, bool html) const {
#ifdef DEBUG_WHY
    std::cout << "Node::why " << debugNodePath() << " (" << NState::toString(state()) << ") top_down(" << top_down
              << ") html(" << html << ")\n";
#endif
    bool why_found = false;
    if (isSuspended()) {
        std::string theReasonWhy;
        if (html) {
            theReasonWhy = path_href();
            theReasonWhy += " is ";
            theReasonWhy += DState::to_html(DState::SUSPENDED);
        }
        else {
            theReasonWhy = debugNodePath();
            theReasonWhy += " is suspended";
        }
        vec.push_back(theReasonWhy);
        why_found = true; // return true if why found
    }
    else if (state() != NState::QUEUED && state() != NState::ABORTED) {
        std::stringstream ss;
        if (html)
            ss << path_href() << " (" << NState::to_html(state()) << ") is not queued or aborted";
        else
            ss << debugNodePath() << " (" << NState::toString(state()) << ") is not queued or aborted";
        vec.push_back(ss.str());

        // When task is active/submitted no point, going any further.
        // However, for FAMILY/SUITE we still need to proceed
        if (isTask())
            return why_found;
        why_found = true; // return true if why found
    }

    // Check limits using in limit manager
    if (inLimitMgr_.why(vec, html))
        why_found = true; // return true if why found

    // Prefix <node-type> <path> <state>
    std::string prefix = debugType();
    prefix += " ";
    if (html)
        prefix += path_href_attribute(absNodePath());
    else
        prefix += absNodePath();
    prefix += "(";
    if (html)
        prefix += NState::to_html(state());
    else
        prefix += NState::toString(state());
    prefix += ") ";

    {
        // postfix  = <attr-type dependent> <next run time > < optional current state>
        std::string postFix;
        const Calendar& c = suite()->calendar();
        for (auto day : days_) {
            postFix.clear();
            if (day.why(c, postFix)) {
                vec.push_back(prefix + postFix);
                why_found = true;
            }
        }
        for (const auto& date : dates_) {
            postFix.clear();
            if (date.why(c, postFix)) {
                vec.push_back(prefix + postFix);
                why_found = true;
            }
        }
        for (const auto& today : todays_) {
            postFix.clear();
            if (today.why(c, days_, dates_, postFix)) {
                vec.push_back(prefix + postFix);
                why_found = true;
            }
        }
        for (const auto& time : times_) {
            postFix.clear();
            if (time.why(c, days_, dates_, postFix)) {
                vec.push_back(prefix + postFix);
                why_found = true;
            }
        }
        for (const auto& cron : crons_) {
            postFix.clear();
            if (cron.why(c, postFix)) {
                vec.push_back(prefix + postFix);
                why_found = true;
            }
        }
    }

    // **************************************************************************************
    // If we have a complete expression that does not evaluate then it should *NOT* hold the node.
    // The complete expression is used to set node to complete, when it evaluates and hence
    // should not prevent further tree walking. evaluate each leaf branch
    // **************************************************************************************

    // Only report on trigger expression, if:
    // 1/ No complete expression
    // 2/ Have complete expression, but it does not evaluate
    bool report_on_trigger_expression = true;
    AstTop* theCompleteAst            = completeAst();
    if (theCompleteAst) {
        if (c_expr_->isFree() || theCompleteAst->evaluate()) {
            report_on_trigger_expression = false; // complete is free, hence we do not look at trigger
        }
    }

    AstTop* theTriggerAst = triggerAst();
    if (report_on_trigger_expression && theTriggerAst) {
        // Note 1: A trigger can be freed by the ForceCmd
        // Note 2: if we have a non-NULL trigger ast, we must have trigger expression
        // Note 3: The freed state is stored on the expression ( i.e. *NOT* on the ast (abstract syntax tree) )
        if (!t_expr_->isFree()) {
#ifdef DEBUG_WHY
            std::cout << "   Node::why " << debugNodePath() << " checking trigger dependencies\n";
#endif
            std::string postFix;
            if (theTriggerAst->why(postFix, html)) {
                vec.push_back(prefix + postFix);
                why_found = true; // return true if why found
            }
        }
    }

#ifdef DEBUG_WHY
    std::cout << "   Node::why " << debugNodePath() << " why found(" << why_found << ")\n";
#endif
    return why_found; // no why found
}

bool Node::checkInvariants(std::string& errorMsg) const {
    for (const ecf::TimeAttr& t : times_) {
        if (!t.checkInvariants(errorMsg))
            return false;
    }
    for (const ecf::TodayAttr& t : todays_) {
        if (!t.checkInvariants(errorMsg))
            return false;
    }
    for (const CronAttr& cron : crons_) {
        if (!cron.checkInvariants(errorMsg))
            return false;
    }

    if (misc_attrs_) {
        if (!misc_attrs_->checkInvariants(errorMsg)) {
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

std::string Node::absNodePath() const {
    std::vector<std::string> vec;
    vec.reserve(Str::reserve_16());
    vec.push_back(name());
    Node* theParent = parent();
    while (theParent) {
        vec.push_back(theParent->name());
        theParent = theParent->parent();
    }
    std::string ret;
    ret.reserve(Str::reserve_64());
    auto r_end = vec.rend();
    for (auto r = vec.rbegin(); r != r_end; ++r) {
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

std::string Node::debugNodePath() const {
    std::string ret = debugType();
    ret += Str::COLON();
    ret += absNodePath();
    return ret;
}

std::string Node::path_href_attribute(const std::string& path) {
    std::string ret = "<a href=\"";
    ret += path;
    ret += "\">";
    ret += path;
    ret += "</a>";
    return ret;
}

std::string Node::path_href_attribute(const std::string& path, const std::string& path2) {
    std::string ret = "<a href=\"";
    ret += path;
    ret += "\">";
    ret += path2;
    ret += "</a>";
    return ret;
}

std::string Node::path_href() const {
    std::string ret = debugType();
    ret += " ";
    ret += path_href_attribute(absNodePath());
    return ret;
}

void Node::verification(std::string& errorMsg) const {
    if (misc_attrs_)
        misc_attrs_->verification(errorMsg);
}

void Node::getAllAstNodes(std::set<Node*>& theSet) const {
    if (completeAst()) {
        AstCollateNodesVisitor astVisitor(theSet);
        completeAst()->accept(astVisitor);
    }
    if (triggerAst()) {
        AstCollateNodesVisitor astVisitor(theSet);
        triggerAst()->accept(astVisitor);
    }
}

AstTop* Node::completeAst() const {
    if (c_expr_) {
        std::string ignoredErrorMsg;
        return completeAst(ignoredErrorMsg);
    }
    return nullptr;
}

AstTop* Node::triggerAst() const {
    if (t_expr_) {
        std::string ignoredErrorMsg;
        return triggerAst(ignoredErrorMsg);
    }
    return nullptr;
}

AstTop* Node::completeAst(std::string& errorMsg) const {
    if (c_expr_) {
        if (c_expr_->get_ast() == nullptr) {

            c_expr_->createAST(const_cast<Node*>(this), "complete", errorMsg);
#ifdef DEBUG
            if (errorMsg.empty())
                LOG_ASSERT(c_expr_->get_ast(), "");
#endif
        }
        return c_expr_->get_ast();
    }
    return nullptr;
}

AstTop* Node::triggerAst(std::string& errorMsg) const {
    if (t_expr_) {
        if (t_expr_->get_ast() == nullptr) {

            t_expr_->createAST(const_cast<Node*>(this), "trigger", errorMsg);
#ifdef DEBUG
            if (errorMsg.empty())
                LOG_ASSERT(t_expr_->get_ast(), "");
#endif
        }
        return t_expr_->get_ast();
    }
    return nullptr;
}

void Node::invalidate_trigger_references() const {
    if (t_expr_) {
        if (t_expr_->get_ast()) {
            t_expr_->get_ast()->invalidate_trigger_references();
        }
    }
    if (c_expr_) {
        if (c_expr_->get_ast()) {
            c_expr_->get_ast()->invalidate_trigger_references();
        }
    }
}

node_ptr Node::remove() {
    SuiteChanged0 changed(shared_from_this());

    Node* theParent = parent();
    if (theParent)
        return theParent->removeChild(this);
    return defs()->removeChild(this);
}

bool Node::getLabelValue(const std::string& labelName, std::string& value) const {
    size_t theSize = labels_.size();
    for (size_t i = 0; i < theSize; i++) {
        if (labels_[i].name() == labelName) {
            if (!(labels_[i].new_value().empty()))
                value = labels_[i].new_value();
            else
                value = labels_[i].value();
            return true;
        }
    }
    return false;
}

bool Node::getLabelNewValue(const std::string& labelName, std::string& value) const {
    size_t theSize = labels_.size();
    for (size_t i = 0; i < theSize; i++) {
        if (labels_[i].name() == labelName) {
            value = labels_[i].new_value();
            return true;
        }
    }
    return false;
}

size_t Node::position() const {
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

void Node::gen_variables(std::vector<Variable>& vec) const {
    repeat_.gen_variables(vec); // if repeat_ is empty vec is unchanged
}

const Variable& Node::findGenVariable(const std::string& name) const {
    return repeat_.find_gen_variable(name); // if repeat_ is empty find returns empty variable by ref
}

void Node::update_repeat_genvar() const {
    repeat_.update_repeat_genvar(); // if repeat_ is empty update_repeat_genvar() does nothing
}

void Node::notify_delete() {
    // make a copy, to avoid iterating over observer list that is being changed
    std::vector<AbstractObserver*> copy_of_observers = observers_;
    for (auto& copy_of_observer : copy_of_observers) {
        copy_of_observer->update_delete(this);
    }

    /// Check to make sure that the Observer called detach
    /// We cannot call detach ourselves, since the the client needs to
    /// call detach in the case where the graphical tree is destroyed by user
    /// In this case the Subject/Node is being deleted.
    assert(observers_.empty());

#ifdef DEBUG_NODE
    if (!observers_.empty()) {
        /// Its not safe to call debugNodePath()/absNodePath() since that will traverse the parent
        /// This may not be safe during a delete.
        std::cout << "notify_delete : Node is not observed : " << name() << "\n";
    }
#endif
}

void Node::notify_start(const std::vector<ecf::Aspect::Type>& aspects) {
    size_t observers_size = observers_.size();
    for (size_t i = 0; i < observers_size; i++) {
        observers_[i]->update_start(this, aspects);
    }
}

void Node::notify(const std::vector<ecf::Aspect::Type>& aspects) {
    size_t observers_size = observers_.size();
    for (size_t i = 0; i < observers_size; i++) {
        observers_[i]->update(this, aspects);
    }
}

void Node::attach(AbstractObserver* obs) {
    observers_.push_back(obs);
}

void Node::detach(AbstractObserver* obs) {
    for (size_t i = 0; i < observers_.size(); i++) {
        if (observers_[i] == obs) {
            observers_.erase(observers_.begin() + i);
            return;
        }
    }
}

bool Node::is_observed(AbstractObserver* obs) const {
    for (auto observer : observers_) {
        if (observer == obs) {
            return true;
        }
    }
    return false;
}

void Node::sort_attributes(ecf::Attr::Type attr, bool recursive, const std::vector<std::string>& no_sort) {
    // cout << "Node::sort_attributes " << absNodePath() << "  " << attr << " recursive " << recursive << "
    // no_sort.size() " << no_sort.size() << "\n";

    if (recursive && !no_sort.empty()) {
        std::string path = absNodePath();
        for (const auto& p : no_sort) {
            // cout << " no_sort: " << p << "\n";
            if (p == path) {
                return;
            }
        }
    }
    auto caseInsen = [](const auto& a, const auto& b) { return Str::caseInsLess(a.name(), b.name()); };

    state_change_no_ = Ecf::incr_state_change_no();
    switch (attr) {
        case Attr::EVENT:
            sort(events_.begin(), events_.end(), [](const Event& a, const Event& b) {
                return Str::caseInsLess(a.name_or_number(), b.name_or_number());
            });
            break;
        case Attr::METER:
            sort(meters_.begin(), meters_.end(), caseInsen);
            break;
        case Attr::LABEL:
            sort(labels_.begin(), labels_.end(), caseInsen);
            break;
        case Attr::LIMIT:
            sort(limits_.begin(), limits_.end(), [](const limit_ptr& a, const limit_ptr& b) {
                return Str::caseInsLess(a->name(), b->name());
            });
            break;
        case Attr::VARIABLE:
            sort(vars_.begin(), vars_.end(), caseInsen);
            break;
        case Attr::ALL:
            sort(vars_.begin(), vars_.end(), caseInsen);
            sort(events_.begin(), events_.end(), [](const Event& a, const Event& b) {
                return Str::caseInsLess(a.name_or_number(), b.name_or_number());
            });
            sort(meters_.begin(), meters_.end(), caseInsen);
            sort(labels_.begin(), labels_.end(), caseInsen);
            sort(limits_.begin(), limits_.end(), [](const limit_ptr& a, const limit_ptr& b) {
                return Str::caseInsLess(a->name(), b->name());
            });
            break;
        case Attr::UNKNOWN:
            break;
        default:
            break;
    }
}

bool Node::checkForAutoCancel(const ecf::Calendar& calendar) const {
    if (auto_cancel_ && state() == NState::COMPLETE) {
        if (auto_cancel_->isFree(calendar, get_state().second)) {

            /// *Only* delete this node if we don't create zombies
            /// anywhere for our children
            vector<Task*> taskVec;
            getAllTasks(taskVec);
            for (Task* t : taskVec) {
                if (t->state() == NState::ACTIVE || t->state() == NState::SUBMITTED) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

bool Node::check_for_auto_archive(const ecf::Calendar& calendar) const {
    if (auto_archive_ && !isSuspended() && !isParentSuspended()) {
        if (auto_archive_->isFree(calendar, get_state())) {

            /// *Only* archive this node if we don't create zombies anywhere for our children
            /// The most significant state could be ABORTED in family, but we could still have active tasks.
            vector<Task*> taskVec;
            getAllTasks(taskVec);
            for (Task* t : taskVec) {
                if (t->state() == NState::ACTIVE || t->state() == NState::SUBMITTED) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

void Node::stats(NodeStats& stats) {
    stats.vars_ += vars_.size();
    if (c_expr_)
        stats.c_trigger_++;
    if (t_expr_)
        stats.trigger_++;
    stats.meters_ += meters_.size();
    stats.events_ += events_.size();
    stats.labels_ += labels_.size();
    stats.times_ += times_.size();
    stats.todays_ += todays_.size();
    stats.crons_ += crons_.size();
    stats.dates_ += dates_.size();
    stats.days_ += days_.size();

    if (late_)
        stats.late_++;
    if (misc_attrs_) {
        stats.verifys_ += misc_attrs_->verifys().size();
        stats.zombies_ += misc_attrs_->zombies().size();
        stats.queues_ += misc_attrs_->queues().size();
        stats.generics_ += misc_attrs_->generics().size();
    }
    if (!repeat_.empty())
        stats.repeats_++;

    stats.limits_ += limits_.size();
    stats.inlimits_ += inLimitMgr_.inlimits().size();
    if (auto_cancel_)
        stats.auto_cancel_++;
    if (auto_archive_)
        stats.auto_archive_++;
    if (auto_restore_)
        stats.auto_restore_++;
}

static std::vector<VerifyAttr> verifys_;
static std::vector<ZombieAttr> zombies_;
static std::vector<QueueAttr> queues_;
static std::vector<GenericAttr> generics_;
const std::vector<VerifyAttr>& Node::verifys() const {
    if (misc_attrs_)
        return misc_attrs_->verifys();
    return verifys_;
}
const std::vector<ZombieAttr>& Node::zombies() const {
    if (misc_attrs_)
        return misc_attrs_->zombies();
    return zombies_;
}
const std::vector<QueueAttr>& Node::queues() const {
    if (misc_attrs_)
        return misc_attrs_->queues();
    return queues_;
}
std::vector<QueueAttr>& Node::ref_queues() {
    if (misc_attrs_)
        return misc_attrs_->ref_queues();
    return queues_;
}
const std::vector<GenericAttr>& Node::generics() const {
    if (misc_attrs_)
        return misc_attrs_->generics();
    return generics_;
}

std::vector<ZombieAttr>::const_iterator Node::zombie_begin() const {
    if (misc_attrs_)
        return misc_attrs_->zombie_begin();
    return zombies_.begin();
}
std::vector<ZombieAttr>::const_iterator Node::zombie_end() const {
    if (misc_attrs_)
        return misc_attrs_->zombie_end();
    return zombies_.end();
}
std::vector<VerifyAttr>::const_iterator Node::verify_begin() const {
    if (misc_attrs_)
        return misc_attrs_->verify_begin();
    return verifys_.begin();
}
std::vector<VerifyAttr>::const_iterator Node::verify_end() const {
    if (misc_attrs_)
        return misc_attrs_->verify_end();
    return verifys_.end();
}
std::vector<QueueAttr>::const_iterator Node::queue_begin() const {
    if (misc_attrs_)
        return misc_attrs_->queue_begin();
    return queues_.begin();
}
std::vector<QueueAttr>::const_iterator Node::queue_end() const {
    if (misc_attrs_)
        return misc_attrs_->queue_end();
    return queues_.end();
}
std::vector<GenericAttr>::const_iterator Node::generic_begin() const {
    if (misc_attrs_)
        return misc_attrs_->generic_begin();
    return generics_.begin();
}
std::vector<GenericAttr>::const_iterator Node::generic_end() const {
    if (misc_attrs_)
        return misc_attrs_->generic_end();
    return generics_.end();
}

template <class Archive>
void Node::serialize(Archive& ar, std::uint32_t const version) {
    ar(CEREAL_NVP(n_));

    CEREAL_OPTIONAL_NVP(ar, st_, [this]() { return st_.first != NState::default_state(); }); // conditionally save
    CEREAL_OPTIONAL_NVP(ar, suspended_, [this]() { return suspended_; });
    CEREAL_OPTIONAL_NVP(ar, d_st_, [this]() { return d_st_.state() != DState::default_state(); }); // conditionally save

    CEREAL_OPTIONAL_NVP(ar, vars_, [this]() { return !vars_.empty(); });  // conditionally save
    CEREAL_OPTIONAL_NVP(ar, c_expr_, [this]() { return c_expr_.get(); }); // conditionally save
    CEREAL_OPTIONAL_NVP(ar, t_expr_, [this]() { return t_expr_.get(); }); // conditionally save

    CEREAL_OPTIONAL_NVP(ar, meters_, [this]() { return !meters_.empty(); }); // conditionally save
    CEREAL_OPTIONAL_NVP(ar, events_, [this]() { return !events_.empty(); }); // conditionally save
    CEREAL_OPTIONAL_NVP(ar, labels_, [this]() { return !labels_.empty(); }); // conditionally save

    CEREAL_OPTIONAL_NVP(ar, times_, [this]() { return !times_.empty(); });   // conditionally save
    CEREAL_OPTIONAL_NVP(ar, todays_, [this]() { return !todays_.empty(); }); // conditionally save
    CEREAL_OPTIONAL_NVP(ar, crons_, [this]() { return !crons_.empty(); });   // conditionally save
    CEREAL_OPTIONAL_NVP(ar, dates_, [this]() { return !dates_.empty(); });   // conditionally save
    CEREAL_OPTIONAL_NVP(ar, days_, [this]() { return !days_.empty(); });     // conditionally save

    CEREAL_OPTIONAL_NVP(ar, late_, [this]() { return late_.get(); });             // conditionally save
    CEREAL_OPTIONAL_NVP(ar, misc_attrs_, [this]() { return misc_attrs_.get(); }); // conditionally save

    CEREAL_OPTIONAL_NVP(ar, repeat_, [this]() { return !repeat_.empty(); });                    // conditionally save
    CEREAL_OPTIONAL_NVP(ar, limits_, [this]() { return !limits_.empty(); });                    // conditionally save
    CEREAL_OPTIONAL_NVP(ar, inLimitMgr_, [this]() { return !inLimitMgr_.inlimits().empty(); }); // conditionally save
    CEREAL_OPTIONAL_NVP(ar, flag_, [this]() { return flag_.flag() != 0; });                     // conditionally save

    CEREAL_OPTIONAL_NVP(ar, auto_cancel_, [this]() { return auto_cancel_.get(); });   // conditionally save
    CEREAL_OPTIONAL_NVP(ar, auto_archive_, [this]() { return auto_archive_.get(); }); // conditionally save
    CEREAL_OPTIONAL_NVP(ar, auto_restore_, [this]() { return auto_restore_.get(); }); // conditionally save

    if (Archive::is_loading::value) {
        if (auto_restore_)
            auto_restore_->set_node(this);
        if (misc_attrs_)
            misc_attrs_->set_node(this);
        for (auto& limit : limits_)
            limit->set_node(this);
    }
}
CEREAL_TEMPLATE_SPECIALIZE_V(Node);
