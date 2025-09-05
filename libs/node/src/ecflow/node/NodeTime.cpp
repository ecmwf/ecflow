/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/CmdContext.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/SuiteChanged.hpp"

using namespace ecf;
using namespace std;

// #define DEBUG_DAY 1

///////////////////////////////////////////////////////////////////////////////////////////

void Node::do_requeue_time_attrs(bool reset_next_time_slot, bool reset_relative_duration, Requeue_args::Requeue_t rt) {
    // must be done before the re-queue
    if (reset_relative_duration) {
        for (auto& cron : crons_) {
            cron.resetRelativeDuration();
        }
        for (auto& today : todays_) {
            today.resetRelativeDuration();
        }
        for (auto& time : times_) {
            time.resetRelativeDuration();
        }
    }

    /// If a job takes longer than it slots, then that slot is missed, and next slot is used
    /// Note we do *NOT* reset for requeue as we want to advance to the next time slot
    /// *NOTE* Update calendar will *free* time dependencies *even* time series. They rely
    /// on this function to clear the time dependencies so they *HOLD* the task.
    const Calendar& calendar = suite()->calendar();
    for (auto& today : todays_) {
        today.requeue(calendar, reset_next_time_slot);
    }
    for (auto& time : times_) {
        time.requeue(calendar, reset_next_time_slot);
    }
    for (auto& cron : crons_) {
        cron.requeue(calendar, reset_next_time_slot);
    }
    for (auto& aviso : avisos_) {
        aviso.start();
    }

    for (auto& date : dates_) {
        date.requeue();
    } // make sure only run once

    if (!days_.empty()) {
        // The day attribute, is matched with the corresponding *date* under requeue.
        // HOWEVER, when re-queueing due to a *TIME* dependency we MUST keep the current date on the day attribute
        // ADDITIONALLY WE use expired_ to remove days which have *EXPIRED* (i.e. failed for check for re-queue), thus
        // avoiding is_free() OTHERWISE when we have multiple days, even the days which have expired are considered for
        // running the task.
        switch (rt) {
            case Requeue_args::FULL: {
#ifdef DEBUG_DAY
                cout << " Node::do_requeue_time_attrs  Requeue_args::FULL \n";
#endif
                for (auto& day : days_) {
                    day.requeue_manual(calendar);
                }
                break;
            }
            case Requeue_args::REPEAT_INCREMENT: {
#ifdef DEBUG_DAY
                cout << " Node::do_requeue_time_attrs  Requeue_args::REPEAT_INCREMENT \n";
#endif
                for (auto& day : days_) {
                    day.requeue_repeat_increment(calendar);
                }
                break;
            }
            case Requeue_args::TIME: {
#ifdef DEBUG_DAY
                cout << " Node::do_requeue_time_attrs  Requeue_args::TIME \n";
#endif
                for (auto& day : days_) {
                    day.requeue_time();
                }
                break;
            }
        }
    }
}

bool Node::calendar_changed_timeattrs(const ecf::Calendar& c, Node::Calendar_args& cal_args) {
#ifdef DEBUG_DAY
    cout << "Node::calendar_changed_timeattrs " << debugNodePath() << " " << c.suite_time_str() << "\n";
#endif

    // For time/today/cron attributes if the time is free, it *remains* free until re-queued
    // However if we have day/date dependencies, that do NOT match, then we should *NOT* free
    // any time/today/cron attributes.
    //
    //   task t
    //     day Monday   # this will guard the time.
    //     time 10:00
    //
    // Hence if we are on Sunday we do *NOT* want to free the time on SUNDAY
    // (Otherwise we will end up running the task at Monday Midnight
    //  and not Monday at 10.00)
    //
    // Likewise:
    //   family f1
    //     day monday     # this will guard the time(this is different to ecflow 4.0) holding_parent_day_or_date_
    //     task t
    //       time 10:00
    //
    //   family f1
    //      time 10:00    # time will be set down at 10.00 am
    //      task t
    //        day monday  # Job will run at Monday morning *AND* at 10:00

    // cout << "Node::calendar_changed_timeattrs " << c.toString() << "\n";
    //   std::vector<node_ptr> all_children1;
    //   allChildren(all_children1);
    //   for(size_t t = 0; t <  all_children1.size(); t++) {
    //       cout << "   " << all_children1[t]->debugNodePath() << " " << NState::toString(all_children1[t]->state()) <<
    //       "\n";
    //   }
    //   AstTop* ast = triggerAst();
    //   if (ast) {
    //      ast->print(cout);
    //      cout << "\n";
    //   }

    if (days_.empty() && dates_.empty()) {

        // No Day or Date, If time matches  calendarChanged(c) will free time dependencies
        for (auto& time : times_) {
            time.calendarChanged(c);
        }
        for (auto& today : todays_) {
            today.calendarChanged(c);
        }
        for (auto& cron : crons_) {
            cron.calendarChanged(c);
        }
    }
    else {

        // If *BEFORE* midnight we have FREE day/date and submitted or active jobs, don't clear the day/dates
        // i.e. take:
        //    family f1
        //       day monday
        //       time 23:00
        //       task t1  # This took longer than 1 hour
        //       task t2  # allow task to continue to the next day
        //         trigger t1 == complete
        // This is only applicable for NodeContainers, for task with day/date always CLEAR at midnight
        // ECFLOW-337 versus ECFLOW-1550

        bool clear_day_at_midnight  = true;
        bool clear_date_at_midnight = true;
        if (c.dayChanged() && isNodeContainer()) {

#ifdef DEBUG_DAY
            cout << "  Node::calendar_changed_timeattrs DAY CHANGED " << debugNodePath() << " " << c.suite_time_str()
                 << "\n";
#endif
            // Check if day/date are free *BEFORE* midnight and *BEFORE* calendarChanged called(since that clears
            // Day::makeFree_) The isFree below relies on Day::makeFree_/Date:makeFree_ not being cleared till after
            // midnight
            bool free_date = false;
            bool free_day  = false;
            for (const auto& date : dates_) {
                if (date.isFree(c)) {
                    free_date = true;
                    break;
                }
            }
            for (const auto& day : days_) {
                if (day.isFree(c)) {
                    free_day = true;
                    break;
                }
            }

#ifdef DEBUG_DAY
            cout << "  Node::calendar_changed_timeattrs free_day " << free_day << " free_date " << free_date << "\n";
#endif
            if (free_date || free_day) {
                // See if we have any complete submitted or active children,
                // if so DON'T clear day/date at midnight. hence day/date will *STAY* *FREE*
                // Allow following tasks to complete
                std::vector<node_ptr> all_children;
                allChildren(all_children);
                int completed = 0;
                int submitted = 0;
                int queued    = 0;
                int active    = 0;
                for (auto& t : all_children) {
                    // cout << all_children[t]->debugNodePath() << " " << NState::toString(all_children[t]->state()) <<
                    // "\n";
                    if (t->isTask()) {
                        if (t->state() == NState::SUBMITTED) {
                            submitted++;
                        }
                        else if (t->state() == NState::ACTIVE) {
                            active++;
                        }
                        else if (t->state() == NState::COMPLETE) {
                            completed++;
                        }
                        else if (t->state() == NState::QUEUED) {
                            queued++;
                        }
                        if (active || submitted) {
                            if (free_date) {
                                clear_date_at_midnight = false;
                            }
                            if (free_day) {
                                clear_day_at_midnight = false;
                            }
#ifdef DEBUG_DAY
                            cout << "  if (active || submitted ) clear_day_date_at_midnight = false\n";
#endif
                            break;
                        }
                        if (completed && (active || submitted || queued)) {
                            if (free_date) {
                                clear_date_at_midnight = false;
                            }
                            if (free_day) {
                                clear_day_at_midnight = false;
                            }
#ifdef DEBUG_DAY
                            cout << "  if (completed && (active || submitted || queued)) clear_day_date_at_midnight = "
                                    "false\n";
#endif
                            break;
                        }
                    }
                }
            }
        }

        bool at_least_one_day_free = false;
        for (auto& day : days_) {
            day.calendarChanged(c, clear_day_at_midnight);
            if (!at_least_one_day_free) {
                at_least_one_day_free = day.isFree(c);
            }
        }

        bool at_least_one_date_free = false;
        for (auto& date : dates_) {
            date.calendarChanged(c, clear_date_at_midnight);
            if (!at_least_one_date_free) {
                at_least_one_date_free = date.isFree(c);
            }
        }

        if (at_least_one_day_free || at_least_one_date_free) {
            for (auto& time : times_) {
                time.calendarChanged(c);
            }
            for (auto& today : todays_) {
                today.calendarChanged(c);
            }
            for (auto& cron : crons_) {
                cron.calendarChanged(c);
            }
        }
        else {
            // Node has *HOLDING* day or date dependency. Avoid free time dependencies of *ANY* child nodes
#ifdef DEBUG_DAY
            cout << "Node::calendar_changed_timeattrs " << debugNodePath()
                 << " SETTING holding_parent_day_or_date_  at " << c.suite_time_str() << "\n";
#endif
            return true;
        }
    }
    return false;
}

void Node::markHybridTimeDependentsAsComplete() {
    // If hybrid clock, then we may have day/date/cron time dependencies
    // which mean that node will be stuck in the QUEUED state, i.e. since the
    // date/day does not change with the hybrid clock.
    // hence Mark these Nodes as complete
    const Calendar& calendar = suite()->calendar();
    if (state() != NState::COMPLETE && calendar.hybrid()) {
        if (!dates_.empty() || !days_.empty() || !crons_.empty()) {

            int noOfTimeDependencies = 0;
            if (!dates_.empty()) {
                noOfTimeDependencies++;
            }
            if (!days_.empty()) {
                noOfTimeDependencies++;
            }
            if (!crons_.empty()) {
                noOfTimeDependencies++;
            }

            bool oneDateIsFree = false;
            bool oneDayIsFree  = false;
            bool oneCronIsFree = false;

            for (auto& date : dates_) {
                if (date.validForHybrid(calendar)) {
                    if (noOfTimeDependencies == 1) {
                        setStateOnly(NState::QUEUED);
                        return;
                    }
                    oneDateIsFree = true;
                    break;
                }
            }
            for (auto& day : days_) {
                if (day.validForHybrid(calendar)) {
                    if (noOfTimeDependencies == 1) {
                        setStateOnly(NState::QUEUED);
                        return;
                    }
                    oneDayIsFree = true;
                    break;
                }
            }
            for (auto& cron : crons_) {
                if (cron.validForHybrid(calendar)) {
                    if (noOfTimeDependencies == 1) {
                        setStateOnly(NState::QUEUED);
                        return;
                    }
                    oneCronIsFree = true;
                    break;
                }
            }

            if (oneDateIsFree || oneDayIsFree || oneCronIsFree) {
                if (noOfTimeDependencies > 1) {
                    // when we have multiple time dependencies their results *MUST* be added for the node to be free.
                    if (!dates_.empty() && !oneDateIsFree) {
                        setStateOnly(NState::COMPLETE);
                        return;
                    }
                    if (!days_.empty() && !oneDayIsFree) {
                        setStateOnly(NState::COMPLETE);
                        return;
                    }
                    if (!crons_.empty() && !oneCronIsFree) {
                        setStateOnly(NState::COMPLETE);
                        return;
                    }

                    // We will only get here, if we have a multiple time dependencies any there is one free in each
                    // category
                    setStateOnly(NState::QUEUED);
                    return;
                }
            }

            setStateOnly(NState::COMPLETE);
        }
    }
}

// #define DEBUG_REQUEUE 1
// #include "ecflow/core/Log.hpp"
bool Node::testTimeDependenciesForRequeue() {
    // This function is called as a part of handling state change.
    // We only get here if the Node has *COMPLETED* ( either automatically, or manually i.e. force complete)
    // We are now determining if the node should be re-queued due to time dependency in the *FUTURE*
    const Calendar& calendar = suite()->calendar();

#ifdef DEBUG_REQUEUE
    LogToCout logtocout;
    LOG(Log::DBG, "Node::testTimeDependenciesForRequeue() " << debugNodePath() << " calendar " << calendar.toString());
#endif

    // When we have a mixture of cron *with* other time based attributes
    // The cron *takes* priority.  Crons should always return true, for checkForRequeue
    for (const CronAttr& cron : crons_) {
        if (cron.checkForRequeue(calendar)) { // will always return true
#ifdef DEBUG_REQUEUE
            LOG(Log::DBG,
                "   Node::testTimeDependenciesForRequeue() " << debugNodePath() << " for cron -> allow-requeue");
#endif
            return true;
        }
    }

    // When in a CmdContext. If we are *BEFORE* the scheduled time, then *ALLOW* re-queue
    bool cmd_context = CmdContext::in_command();
    if (!times_.empty()) {
        TimeSlot the_min, the_max; // Needs to handle multiple single slot time attributes
        for (const ecf::TimeAttr& time : times_) {
            time.min_max_time_slots(the_min, the_max);
        }
        for (const ecf::TimeAttr& time : times_) {
            if (time.checkForRequeue(calendar, the_min, the_max, cmd_context)) {
#ifdef DEBUG_REQUEUE
                LOG(Log::DBG,
                    "   Node::testTimeDependenciesForRequeue() " << debugNodePath() << " for " << time.toString()
                                                                 << " -> allow-requeue");
#endif
                return true;
            }
        }
    }

    if (!todays_.empty()) {
        TimeSlot the_min, the_max; // Needs to handle multiple single slot today attributes
        for (const ecf::TodayAttr& today : todays_) {
            today.min_max_time_slots(the_min, the_max);
        }
        for (const ecf::TodayAttr& today : todays_) {
            if (today.checkForRequeue(calendar, the_min, the_max, cmd_context)) {
#ifdef DEBUG_REQUEUE
                LOG(Log::DBG,
                    "   Node::testTimeDependenciesForRequeue() " << debugNodePath() << " for " << today.toString()
                                                                 << " -> allow-requeue");
#endif
                return true;
                ;
            }
        }
    }

    // **********************************************************************
    // If we get here there are **NO** time/today/cron dependencies which are free
    // We now need to determine if this node has a *FUTURE* time dependency which
    // should re-queue this node.
    // *********************************************************************
    for (const DateAttr& date : dates_) {
        if (date.checkForRequeue(calendar)) {
#ifdef DEBUG_REQUEUE
            LOG(Log::DBG,
                "   Node::testTimeDependenciesForRequeue() " << debugNodePath() << " for " << date.toString()
                                                             << " -> allow-requeue");
#endif
            return true;
        }
    }

    for (DayAttr& day : days_) {
        if (cmd_context) {
            // In the command context,i.e. force complete or task runs and completes, then expire the day.
            // *EVEN* if the day is in the future: why?: The user has taken control, *TYPICALLY* the day is under
            // a repeat, by expiring, we allow the repeat to increment
            //       before *only* if the day matched under the command context, we got this behaviour. i.e. with
            //       check_for_expiration
            day.set_expired();
        }
        else {
            // If any day matches calendar day or is in the past, then expire it, so we don't run again on that day.
            // Must be done BEFORE checkForRequeue
            //   task t1
            //      time 09:00    # time attribute get considered before day/date, allowing multiple re-queues on the
            //      same day time 10:00    # day saturday  # After time expiration, day must be expired day sunday
            day.check_for_expiration(calendar);
        }
    }
    for (const DayAttr& day : days_) {
        if (day.checkForRequeue(calendar)) {
#ifdef DEBUG_REQUEUE
            LOG(Log::DBG,
                "   Node::testTimeDependenciesForRequeue() " << debugNodePath() << " for days  -> allow-requeue");
#endif
            return true;
        }
    }

#ifdef DEBUG_REQUEUE
    LOG(Log::DBG, "   Node::testTimeDependenciesForRequeue() " << debugNodePath() << " HOLDING");
#endif
    return false;
}

void Node::miss_next_time_slot() {
    // Why do we need to set NO_REQUE_IF_SINGLE_TIME_DEP flag ?
    // This is required when we have time based attributes, which we want to miss.
    //    time 10:00
    //    time 12:00
    // Essentially this avoids an automated job run, *IF* the job was run manually for a given time slot.
    // If we call this function before 10:00, we want to miss the next time slot (i.e. 10:00)
    // and want to *requeue*, for 12:00 time slot. However, at re-queue, we need to ensure
    // we do *not* reset the 10:00 time slot. hence by setting NO_REQUE_IF_SINGLE_TIME_DEP
    // we allow requeue to query this flag, and hence avoid resetting the time based attribute
    // Note: requeue will *always* clear NO_REQUE_IF_SINGLE_TIME_DEP afterward.
    //
    // In the case above when we reach the last time slot, there is *NO* automatic requeue, and
    // hence, *no* clearing of NO_REQUE_IF_SINGLE_TIME_DEP flag.
    // This will then be up to any top level parent that has a Repeat/cron to force a requeue
    // when all the children are complete. *or* user does a manual re-queue
    //
    // Additionally if the job *aborts*, we clear NO_REQUE_IF_SINGLE_TIME_DEP if it was set.
    // Otherwise, if manually run again, we will miss further time slots.
    if (has_time_dependencies()) {

        /// Handle abort
        /// The flag: NO_REQUE_IF_SINGLE_TIME_DEP is *only* set when doing an interactive force complete or run command.
        /// What happens if the job aborts during the run command ?
        ///     time 10:00
        ///     time 11:00
        /// If at 9.00am we used the run command, we want to miss the 10:00 time slot.
        /// However if the run at 9.00 fails, and we run again, we also miss 11:00 time slot.
        /// During the run the flag is still set.
        /// Hence *ONLY* miss the next time slot *IF* Flag::NO_REQUE_IF_SINGLE_TIME_DEP is NOT set
        if (!get_flag().is_set(Flag::NO_REQUE_IF_SINGLE_TIME_DEP)) {

            SuiteChanged0 changed(shared_from_this());
            get_flag().set(Flag::NO_REQUE_IF_SINGLE_TIME_DEP);

            // Note: when we have multiple time dependencies.
            // We need find valid next time dependency:
            //   time 10:00
            //   time 11:00
            //   time 12:00 14:00 00:30
            // Also we could have a mix:
            //   time  10:00
            //   today 10:30
            //   time 11:00
            //   time 12:00 14:00 00:30

            // for the moment assume, they have been added sequentially,
            // hence only first non expired time is updated to miss next time slot
            for (auto& time : times_) {
                if (time.time_series().is_valid()) {
                    time.miss_next_time_slot();
                    break;
                }
            }
            for (auto& today : todays_) {
                if (today.time_series().is_valid()) {
                    today.miss_next_time_slot();
                    break;
                }
            }
            for (auto& cron : crons_) {
                if (cron.time_series().is_valid()) {
                    cron.miss_next_time_slot();
                    break;
                }
            }
        }
    }
}

void Node::freeHoldingDateDependencies() {
    // Multiple time dependencies of the same type are *ORed*
    // Multiple time dependencies of different types are *ANDed*
    //
    // Hence since we have multiple time dependencies of the same
    // type here, we need free only one of them
    const Calendar& calendar = suite()->calendar();
    for (auto& date : dates_) {
        if (!date.isFree(calendar)) {
            date.setFree();
            break;
        }
    }
}

void Node::freeHoldingTimeDependencies() {
    // Multiple time dependencies of the same type are *ORed*
    // Multiple time dependencies of different types are *ANDed*
    //
    // If we have multiple time dependencies of different types
    // we need only free one in each category
    const Calendar& calendar = suite()->calendar();
    for (auto& time : times_) {
        if (!time.isFree(calendar)) {
            time.setFree();
            time.miss_next_time_slot();
            break;
        }
    }
    for (auto& today : todays_) {
        if (!today.isFree(calendar)) {
            today.setFree();
            today.miss_next_time_slot();
            break;
        }
    }
    for (auto& day : days_) {
        if (!day.isFree(calendar)) {
            day.setFree();
            break;
        }
    }
    for (auto& cron : crons_) {
        if (!cron.isFree(calendar)) {
            cron.setFree();
            cron.miss_next_time_slot();
            break;
        }
    }
}

bool Node::has_time_dependencies() const {
    if (!times_.empty()) {
        return true;
    }
    if (!todays_.empty()) {
        return true;
    }
    if (!crons_.empty()) {
        return true;
    }
    if (!dates_.empty()) {
        return true;
    }
    if (!days_.empty()) {
        return true;
    }
    if (!avisos_.empty()) {
        return true;
    }
    return false;
}

bool Node::holding_day_or_date(const ecf::Calendar& c) const {
    if (days_.empty() && dates_.empty()) {
        return false;
    }

    bool at_least_one_day_free = false;
    for (auto& day : days_) {
        if (!at_least_one_day_free) {
            at_least_one_day_free = day.isFree(c);
        }
    }

    bool at_least_one_date_free = false;
    for (auto& date : dates_) {
        if (!at_least_one_date_free) {
            at_least_one_date_free = date.isFree(c);
        }
    }

    if (at_least_one_day_free || at_least_one_date_free) {
        return false;
    }
    return true;
}

bool Node::timeDependenciesFree() const {
    int noOfTimeDependencies = 0;
    if (!times_.empty()) {
        noOfTimeDependencies++;
    }
    if (!todays_.empty()) {
        noOfTimeDependencies++;
    }
    if (!dates_.empty()) {
        noOfTimeDependencies++;
    }
    if (!days_.empty()) {
        noOfTimeDependencies++;
    }
    if (!crons_.empty()) {
        noOfTimeDependencies++;
    }
    if (!avisos_.empty()) { // TODO: Not really time related! Should be moved to another member function
        noOfTimeDependencies++;
    }

    // if no time dependencies we are free
    if (noOfTimeDependencies == 0) {
        return true;
    }

    // if we have a holding day/date don't consider other time attributes
    const Calendar& calendar = suite()->calendar();
    if (holding_day_or_date(calendar)) {
        return false;
    }

    bool oneDateIsFree  = false;
    bool oneDayIsFree   = false;
    bool oneTodayIsFree = false;
    bool oneTimeIsFree  = false;
    bool oneCronIsFree  = false;
    bool oneAvisoIsFree = false;

    for (const auto& time : times_) {
        if (time.isFree(calendar)) {
            if (noOfTimeDependencies == 1) {
                return true;
            }
            oneTimeIsFree = true;
            break;
        }
    }
    for (const auto& cron : crons_) {
        if (cron.isFree(calendar)) {
            if (noOfTimeDependencies == 1) {
                return true;
            }
            oneCronIsFree = true;
            break;
        }
    }
    for (const auto& date : dates_) {
        if (date.isFree(calendar)) {
            if (noOfTimeDependencies == 1) {
                return true;
            }
            oneDateIsFree = true;
            break;
        }
    }
    for (const auto& day : days_) {
        if (day.isFree(calendar)) {
            if (noOfTimeDependencies == 1) {
                return true;
            }
            oneDayIsFree = true;
            break;
        }
    }
    for (const auto& aviso : avisos_) {
        if (aviso.isFree()) {
            LOG(Log::DBG, "NodeTime: checking Aviso isFree: true, for " << aviso.path() << ":" << aviso.name());
            if (noOfTimeDependencies == 1) {
                return true;
            }
            oneAvisoIsFree = true;
            break;
        }
        else {
            LOG(Log::DBG, "NodeTime: checking Aviso isFree: false");
        }
    }

    if (!todays_.empty()) {
        // : single Today: (single-time)    is free, if calendar time >= today_time
        // : single Today: (range)          is free, if calendar time == (one of the time ranges)
        // : multi Today : (single | range) is free, if calendar time == (one of the time ranges | today_time)
        // : multi Today : (single | range) is free, all are free
        if (todays_.size() == 1) {
            // Single Today Attribute: could be single slot or range
            if (todays_[0].isFree(calendar)) {
                if (noOfTimeDependencies == 1) {
                    return true;
                }
                oneTodayIsFree = true;
            }
        }
        else {
            // Multiple Today Attributes, each could single, or range
            size_t free_count = 0;
            for (const auto& today : todays_) {
                if (today.isFreeMultipleContext(calendar)) {
                    if (noOfTimeDependencies == 1) {
                        return true;
                    }
                    oneTodayIsFree = true;
                    break;
                }
                if (today.isFree(calendar)) {
                    free_count++;
                }
            }
            if (free_count == todays_.size()) {
                if (noOfTimeDependencies == 1) {
                    return true;
                }
                oneTodayIsFree = true;
            }
        }
    }

    if (oneDateIsFree || oneDayIsFree || oneTodayIsFree || oneTimeIsFree || oneCronIsFree || oneAvisoIsFree) {
        if (noOfTimeDependencies > 1) {
            // *When* we have multiple time dependencies of *different types* then the results
            // *MUST* be added for the node to be free.
            if (!dates_.empty() && !oneDateIsFree) {
                return false;
            }
            if (!days_.empty() && !oneDayIsFree) {
                return false;
            }
            if (!todays_.empty() && !oneTodayIsFree) {
                return false;
            }
            if (!times_.empty() && !oneTimeIsFree) {
                return false;
            }
            if (!crons_.empty() && !oneCronIsFree) {
                return false;
            }
            if (!avisos_.empty() && !oneAvisoIsFree) {
                return false;
            }

            // We will only get here, if we have a multiple time dependencies and they are free
            return true;
        }
    }

    return false;
}

bool Node::time_today_cron_is_free() const {
    if (!times_.empty() || !todays_.empty() || !crons_.empty()) {

        int noOfTimeDependencies = 0;
        if (!times_.empty()) {
            noOfTimeDependencies++;
        }
        if (!todays_.empty()) {
            noOfTimeDependencies++;
        }
        if (!crons_.empty()) {
            noOfTimeDependencies++;
        }

        bool oneTodayIsFree = false;
        bool oneTimeIsFree  = false;
        bool oneCronIsFree  = false;

        const Calendar& calendar = suite()->calendar();
        for (const auto& time : times_) {
            if (time.isFree(calendar)) {
                if (noOfTimeDependencies == 1) {
                    return true;
                }
                oneTimeIsFree = true;
                break;
            }
        }
        for (const auto& cron : crons_) {
            if (cron.isFree(calendar)) {
                if (noOfTimeDependencies == 1) {
                    return true;
                }
                oneCronIsFree = true;
                break;
            }
        }

        if (!todays_.empty()) {
            // : single Today: (single-time)   is free, if calendar time >= today_time
            // : single Today: (range)         is free, if calendar time == (one of the time ranges)
            // : multi Today : (single | range)is free, if calendar time == (one of the time ranges | today_time)
            if (todays_.size() == 1) {
                // Single Today Attribute: could be single slot or range
                if (todays_[0].isFree(calendar)) {
                    if (noOfTimeDependencies == 1) {
                        return true;
                    }
                    oneTodayIsFree = true;
                }
            }
            else {
                // Multiple Today Attributes, each could single, or range
                for (const auto& today : todays_) {
                    if (today.isFreeMultipleContext(calendar)) {
                        if (noOfTimeDependencies == 1) {
                            return true;
                        }
                        oneTodayIsFree = true;
                        break;
                    }
                }
            }
        }

        if (oneTodayIsFree || oneTimeIsFree || oneCronIsFree) {
            if (noOfTimeDependencies > 1) {
                // *When* we have multiple time dependencies of *different types* then the results
                // *MUST* be added for the node to be free.
                if (!todays_.empty() && !oneTodayIsFree) {
                    return false;
                }
                if (!times_.empty() && !oneTimeIsFree) {
                    return false;
                }
                if (!crons_.empty() && !oneCronIsFree) {
                    return false;
                }

                // We will only get here, if we have a multiple time dependencies and they are free
                return true;
            }
        }
    }

    return false;
}

void Node::get_time_resolution_for_simulation(boost::posix_time::time_duration& resol) const {
    for (const auto& time : times_) {
        const TimeSeries& time_series = time.time_series();
        if (time_series.start().minute() != 0) {
            resol = boost::posix_time::minutes(1);
            return;
        }
        if (time_series.hasIncrement()) {
            if (time_series.finish().minute() != 0) {
                resol = boost::posix_time::minutes(1);
                return;
            }
            if (time_series.incr().minute() != 0) {
                resol = boost::posix_time::minutes(1);
                return;
            }
        }
    }

    for (const auto& today : todays_) {
        const TimeSeries& time_series = today.time_series();
        if (time_series.start().minute() != 0) {
            resol = boost::posix_time::minutes(1);
            return;
        }
        if (time_series.hasIncrement()) {
            if (time_series.finish().minute() != 0) {
                resol = boost::posix_time::minutes(1);
                return;
            }
            if (time_series.incr().minute() != 0) {
                resol = boost::posix_time::minutes(1);
                return;
            }
        }
    }

    for (const auto& cron : crons_) {
        const TimeSeries& time_series = cron.time_series();
        if (time_series.start().minute() != 0) {
            resol = boost::posix_time::minutes(1);
            return;
        }
        if (time_series.hasIncrement()) {
            if (time_series.finish().minute() != 0) {
                resol = boost::posix_time::minutes(1);
                return;
            }
            if (time_series.incr().minute() != 0) {
                resol = boost::posix_time::minutes(1);
                return;
            }
        }
    }
}

void Node::get_max_simulation_duration(boost::posix_time::time_duration& duration) const {
    // don't override a higher value of duration
    if ((!times_.empty() || !todays_.empty()) && duration < boost::posix_time::hours(24)) {
        duration = boost::posix_time::hours(24); // day
    }
    if (!days_.empty() && duration < boost::posix_time::hours(168)) {
        duration = boost::posix_time::hours(168); // week
    }
    if (!dates_.empty() && duration < boost::posix_time::hours(24 * 7 * 31)) {
        duration = boost::posix_time::hours(24 * 7 * 31); // month
    }
    if (!crons_.empty()) {
        duration = boost::posix_time::hours(8760); // year
    }
    if (!repeat_.empty()) {
        duration = boost::posix_time::hours(8760); // year
    }
}
