/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/simulator/SimulatorVisitor.hpp"

#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"

namespace ecf {

// Please note: we can speed up simulation, by using a calendar increment of 1 hour
// However this will *ONLY* work *IF*
//   o All the time based attributes time,today,cron have no minutes, i.e only i hour resolution
//   o The calendar start time, must also have hour resolution, otherwise time/cron based attributes
//     will be missed. i.e we increment calendar start time , with calendar increment
//     If calendar start time is minutes based, and increment in hour based, time attributes will be missed
//     Note: today is different, to time/cron, as it does not require a exact match

///////////////////////////////////////////////////////////////////////////////
SimulatorVisitor::SimulatorVisitor(const std::string& defs_filename)
    : defs_filename_(defs_filename),
      foundTasks_(false),
      foundCrons_(false),
      foundTime_(false),
      hasTimeDependencies_(false),
      has_end_clock_(false),
      max_sim_duration_(boost::posix_time::hours(24)),
      max_suite_duration_(boost::posix_time::hours(24)),
      ci_(boost::posix_time::hours(1)) {
}

void SimulatorVisitor::visitDefs(Defs* d) {
    for (suite_ptr s : d->suiteVec()) {
        s->acceptVisitTraversor(*this);
    }
}

void SimulatorVisitor::visitSuite(Suite* s) {
    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the ecf files.
    s->begin();

    // Found time dependencies use calendar increment of one minute
    if (s->hasTimeDependencies()) {
        hasTimeDependencies_ = true;
    }

    if (s->clockAttr() && s->clock_end_attr()) {
        max_suite_duration_ = s->clock_end_attr()->ptime() - s->clockAttr()->ptime();
        has_end_clock_      = true; // no need to determine max_length, user specified
    }

    visitNodeContainer(s);

    // IF the suite has no task  (i.e could consist of just limits, set suite to complete
    // Since we rely on it for termination of tests
    // make setting NState::COMPLETE is after begin(), which will set Node into the queued state
    if (!foundTasks_) {
        s->set_state(NState::COMPLETE);
        std::stringstream ss;
        ss << "The defs file " << defs_filename_ << " has a suite '/" << s->suite()->name()
           << "' which has no tasks. Ignoring \n";
        log(Log::WAR, ss.str());
    }

    // If we have cron/time with calendar increment of 1 hour, where calendar start time is in minutes
    // we will miss the time/cron based attributes, hence use 1 minute resolution
    if ((foundCrons_ || foundTime_) && ci_ == boost::posix_time::hours(1)) {
        // simulation has not started so, suiteTime same as start time.
        boost::posix_time::time_duration start_time = s->calendar().suiteTime().time_of_day();
        if (start_time.minutes() != 0) {
            // cout << " start_time " << start_time << "   " << defs_filename_ << "\n";
            log(Log::WAR,
                "Found cron or time based attributes, with 1 hour resolution, however suite calendar start time has "
                "minute resolution, reverting to minute resolution for simulation.");
            log(Log::WAR,
                "To speed up resolution use suite calendar with hour setting only, i.e where minutes is zero");
            ci_ = boost::posix_time::minutes(1);
        }
    }

    // record the max simulation period for THIS suite. i.e different suites can have different simulation period
    suite_duration_vec_.emplace_back(s, max_suite_duration_);

    // make sure total simulation period cover all the suites.
    if (max_suite_duration_ > max_sim_duration_) {
        max_sim_duration_ = max_suite_duration_;
    }
}

boost::posix_time::time_duration SimulatorVisitor::max_simulation_period(Suite* s) const {
    size_t suite_duration_vec_size = suite_duration_vec_.size();
    for (size_t i = 0; i < suite_duration_vec_size; i++) {
        if (suite_duration_vec_[i].first == s)
            return suite_duration_vec_[i].second;
    }
    return max_sim_duration_;
}

void SimulatorVisitor::visitFamily(Family* f) {
    visitNodeContainer(f);
}

void SimulatorVisitor::visitNodeContainer(NodeContainer* nc) {
    if (ci_ == boost::posix_time::hours(1))
        nc->get_time_resolution_for_simulation(ci_);
    if (!has_end_clock_)
        nc->get_max_simulation_duration(max_suite_duration_);

    if (!nc->crons().empty()) {
        foundCrons_ = true;
        std::stringstream ss;
        ss << defs_filename_ << ": Found crons on NodeContainer\n";
        log(Log::MSG, ss.str());
    }

    if (!nc->timeVec().empty()) {
        foundTime_ = true;
    }

    for (node_ptr t : nc->nodeVec()) {
        t->acceptVisitTraversor(*this);
    }
}

void SimulatorVisitor::visitTask(Task* t) {
    if (ci_ == boost::posix_time::hours(1))
        t->get_time_resolution_for_simulation(ci_);
    if (!has_end_clock_)
        t->get_max_simulation_duration(max_suite_duration_);

    foundTasks_ = true;

    if (!t->crons().empty()) {
        foundCrons_ = true;
    }
    if (!t->timeVec().empty()) {
        foundTime_ = true;
    }
}

} // namespace ecf
