/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/simulator/Simulator.hpp"

#include <boost/date_time/posix_time/time_formatters.hpp> // requires boost date and time lib

#include "ecflow/attribute/QueueAttr.hpp"
#include "ecflow/core/CalendarUpdateParams.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Jobs.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/Submittable.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/simulator/Analyser.hpp"
#include "ecflow/simulator/SimulatorVisitor.hpp"

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace std;
using namespace ecf;

// #define DEBUG_LONG_RUNNING_SUITES 1

namespace ecf {

/// Class that allows multiple log files.
class LogDestroyer {
public:
    LogDestroyer() = default;
    ~LogDestroyer() { Log::destroy(); }
};

Simulator::Simulator() {
}

bool Simulator::run(const std::string& theDefsFile, std::string& errorMsg) const {
#ifdef DEBUG_LONG_RUNNING_SUITES
    cout << "Simulator::run parsing file " << theDefsFile << endl;
#endif

    Defs theDefs;
    std::string warningMsg;
    if (!theDefs.restore(theDefsFile, errorMsg, warningMsg))
        return false;
    // cout << theDefs << "\n";
    return run(theDefs, theDefsFile, errorMsg, false /* don't do check, allready done */);
}

bool Simulator::run(Defs& theDefs, const std::string& defs_filename, std::string& errorMsg, bool do_checks) const {
#ifdef DEBUG_LONG_RUNNING_SUITES
    std::cout << "Simulator::run " << defs_filename << " no of suites " << theDefs.suiteVec().size() << endl;
#endif
    // ****:NOTE:******
    // ** This simulator relies on the defs checking to set event and meter usedInTrigger()
    // ** to speed up simulator. However this is done in AstResolveVisitor.
    // ** Which is called during checking:
    // ** By default checking in done when reading a defs file from disk:
    // ** However many test create Defs on the fly. These may not do_checks.
    // ** Hence we do it here, since it is simulator specific.

    //   Please note that when we use a clock attribute, for a specific date
    //   then when we are using repeat, they can cause the suite to re-queue
    //   and RESET the suite time, back to clock attribute time

    if (do_checks) {
        std::string warningMsg;
        if (!theDefs.check(errorMsg, warningMsg)) {
            return false;
        }
    }

    // Allow new log to be created each time, by destroying the old log.
    LogDestroyer destroyLog;

    // Initialise the Log file, and destroy when done. This
    // allows new log file to be created named after the definition file
    std::string logFileName = defs_filename + ".log";
    fs::remove(logFileName);
    Log::create(logFileName);
    // cout << "defs_filename " << defs_filename << "\n";
    // cout << "***** after Log::create(logFileName)\n";

    // Do the following:
    // o call begin() for each suite
    // o determine calendar increment. If no time dependencies use an hour increment
    // o determine max simulation period. ie looks at size of repeats
    // o If multiple suites and some suites have no tasks, mark them as complete, these may have server limits
    // o If no tasks at all, no point in simulating
    // o ********************************************************************************
    //   Please note that when we use a clock attribute, for a specific date
    //   then when we are using repeat, they can cause the suite to re-queue
    //   and RESET the suite time, back to clock attribute time
    //   *********************************************************************************
    SimulatorVisitor simiVisitor(defs_filename);
    theDefs.acceptVisitTraversor(simiVisitor);
    if (!simiVisitor.errors_found().empty()) {
        errorMsg += simiVisitor.errors_found();
        return false;
    }

    // Let visitor determine calendar increment. i.e if no time dependencies we will use 1 hour increment
    time_duration calendarIncrement                        = simiVisitor.calendarIncrement();
    boost::posix_time::time_duration max_simulation_period = simiVisitor.maxSimulationPeriod();

    std::stringstream ss;
    ss << " time dependency(" << simiVisitor.hasTimeDependencies() << ") max_simulation_period("
       << to_simple_string(max_simulation_period) << ") calendarIncrement(" << to_simple_string(calendarIncrement)
       << ")";
    std::string msg = ss.str();
    log(Log::MSG, msg);
#ifdef DEBUG_LONG_RUNNING_SUITES
    cout << defs_filename << msg
         << "   ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
#endif

    // Do we have autocancel, must be done before.
    int hasAutoCancel = 0;
    for (suite_ptr s : theDefs.suiteVec()) {
        if (s->hasAutoCancel())
            hasAutoCancel++;
    }

    // ==================================================================================
    // Start simulation ...
    // Assume: User has taken into account autocancel end time.
    // ==================================================================================
    CalendarUpdateParams calUpdateParams(calendarIncrement);
    boost::posix_time::time_duration duration(0, 0, 0, 0);
    while (duration <= max_simulation_period) {

#ifdef DEBUG_LONG_RUNNING_SUITES
        for (suite_ptr my_suite : theDefs.suiteVec()) {
            cout << "duration: " << to_simple_string(duration) << " " << my_suite->calendar().toString()
                 << " +++++++++++++++++++++++++++++++++++ " << endl;

            // use following to snapshot definition state at a given point in time
            //         {
            //        	 boost::gregorian::date debug_date(2019,8,5);
            //        	 ptime debug_time(debug_date, hours(10) );
            //        	 if (my_suite->calendar().suiteTime() == debug_time) {
            //        		 cout << theDefs.print(PrintStyle::MIGRATE) << endl;
            //        	 }
            //         }
        }
#endif

        // Resolve dependencies and submit jobs
        if (!doJobSubmission(theDefs, errorMsg))
            return false;

        // Increment calendar per suite. *MUST* use *COPY* as update_calendar() can remove suites (auto-cancel)
        std::vector<suite_ptr> suiteVec = theDefs.suiteVec();
        for (suite_ptr suite : suiteVec) {
            boost::posix_time::time_duration max_duration_for_suite = simiVisitor.max_simulation_period(suite.get());
            if (duration < max_duration_for_suite) {
                theDefs.update_calendar(suite.get(), calUpdateParams);
            }
        }
        duration += calendarIncrement;
    }

    // ==================================================================================
    // END of simulation
    // ==================================================================================
    if (!theDefs.checkInvariants(errorMsg)) {
        return false;
    }

    // Cater for suite, with no verify attributes, but which are not complete. testAnalysis.cpp
    // Ignore suites with crons as they will never complete
    // Ignore suites with autocancel, as suite may get deleted
    if (!simiVisitor.foundCrons() && (hasAutoCancel == 0)) {
        size_t completeSuiteCnt = 0;
        for (suite_ptr s : theDefs.suiteVec()) {
            if (s->state() == NState::COMPLETE)
                completeSuiteCnt++;
        }

        if ((theDefs.suiteVec().size() != completeSuiteCnt)) {
            std::stringstream mss;
            mss << "\nDefs file " << defs_filename << "\n";
            for (suite_ptr s : theDefs.suiteVec()) {
                if (s->state() != NState::COMPLETE)
                    mss << "  suite '/" << s->name() << "' has not completed\n";
            }
            errorMsg += mss.str();

            run_analyser(theDefs, errorMsg);
            return false;
        }
    }

    if (theDefs.verification(errorMsg)) {
        return true;
    }

    run_analyser(theDefs, errorMsg);
    return false;
}

void Simulator::run_analyser(Defs& theDefs, std::string& errorMsg) const {
    Analyser analyser;
    analyser.run(theDefs);
    errorMsg += "Please see files .flat and .depth for analysis\n";
    errorMsg += ecf::as_string(theDefs, PrintStyle::MIGRATE);
}

bool Simulator::doJobSubmission(Defs& theDefs, std::string& errorMsg) const {
    //   const std::vector<suite_ptr>& suiteVec = theDefs.suiteVec();
    //   for(suite_ptr s: suiteVec) cout << s->debugNodePath() << " " << s->calendar().toString() <<  endl;

    // For the simulation we ensure job submission takes less than 10 seconds
    // Resolve dependencies and submit jobs
    JobsParam jobsParam(10 /*submitJobsInterval */, false /*create jobs*/); // spawn jobs *will* be set to false
    Jobs jobs(&theDefs);
    if (!jobs.generate(jobsParam)) {
        ecf::log(Log::ERR, jobsParam.getErrorMsg());
        assert(false);
        return false;
    }

    // #ifdef DEBUG_LONG_RUNNING_SUITES
    //	cout << "Simulator::doJobSubmission jobsParam.submitted().size() " << jobsParam.submitted().size() << " level =
    //" << level_ << endl; #endif

    level_++;

    // For those jobs that were submitted, Simulate client by going
    // through the task events and meters and updating them and then
    // re-checking for job submission. Finally mark task as complete
    // This is important as there may be other task dependent on this.
    for (Submittable* t : jobsParam.submitted()) {

#ifdef DEBUG_LONG_RUNNING_SUITES
        // If task repeating themselves, determine what is causing this:
        std::map<Submittable*, int>::iterator i = taskIntMap_.find(t);
        if (i == taskIntMap_.end())
            taskIntMap_.insert(std::make_pair(t, 1));
        else {
            (*i).second++;
            // Find top most node that has a repeat, check if its incrementing:
            Node* nodeWithRepeat = NULL;
            Node* theParent      = t->parent();
            while (theParent) {
                if (!theParent->repeat().empty())
                    nodeWithRepeat = theParent;
                theParent = theParent->parent();
            }
            // cout << t->suite()->calendar().toString();
            if (nodeWithRepeat) {
                cout << " level " << level_ << " submitted task " << t->debugNodePath() << " count = " << (*i).second
                     << " HAS parent Repeating node " << nodeWithRepeat->debugNodePath() << " "
                     << nodeWithRepeat->repeat().dump() << endl;
            }
            else { // cound be a cron
                cout << " level " << level_ << " submitted task " << t->debugNodePath() << " count = " << (*i).second
                     << endl;
            }
        }
#endif

#ifdef DEBUG_LONG_RUNNING_SUITES
        cout << t->debugNodePath() << " completes at " << t->suite()->calendar().toString() << " level " << level_
             << " parent state:" << endl;
#endif

        // If the task has any event used in the trigger expressions, then update event.
        std::string msg;
        for (Event& event : t->ref_events()) {
            if (event.usedInTrigger()) { // event used in trigger/complete expression
                // initial value, if the value taken by the event on begin/re-queue. Child command is expected to invert
                // the event
                if (event.initial_value())
                    event.set_value(false); // initial value is set,   hence clear event
                else
                    event.set_value(true); // initial value is clear, hence set the event , (default)

                msg.clear();
                msg += Str::CHILD_CMD();
                msg += "event ";
                msg += event.name_or_number();
                msg += " ";
                msg += t->absNodePath();
                log(Log::MSG, msg);

                if (!doJobSubmission(theDefs, errorMsg)) {
                    level_--;
                    return false;
                }
            }
            else {
                // warn about events not used in any trigger or complete expressions
                // 				cout << "submitted task " << t->debugNodePath() << " UN-USED event " <<
                // event.toString() << "\n";
            }
        }

        // if the task has any meters used in trigger expressions, then increment meters
        for (Meter& meter : t->ref_meters()) {
            if (meter.usedInTrigger()) { // meter used in trigger/complete expression
                while (meter.value() < meter.max()) {
                    meter.set_value(meter.value() + 1);

                    msg.clear();
                    msg += Str::CHILD_CMD();
                    msg += "meter ";
                    msg += meter.name();
                    msg += " ";
                    msg += t->absNodePath();
                    log(Log::MSG, msg);

                    if (!doJobSubmission(theDefs, errorMsg)) {
                        level_--;
                        return false;
                    }
                }
            }
            else {
                // Meters that are not used in trigger are usually used to indicate progress.
                meter.set_value(meter.max());
            }
        }

        if (!update_for_queues(t, msg, t->ref_queues(), theDefs, errorMsg))
            return false;
        Node* parent = t->parent();
        while (parent) {
            if (!update_for_queues(t, msg, parent->ref_queues(), theDefs, errorMsg))
                return false;
            parent = parent->parent();
        }

        // any state change should be followed with a job submission
        t->complete(); // mark task as complete
    }

    level_--;
    return true;
}

bool Simulator::update_for_queues(Submittable* t,
                                  std::string& msg,
                                  std::vector<QueueAttr>& queues,
                                  Defs& theDefs,
                                  std::string& errorMsg) const {
    for (QueueAttr& queue : queues) {
        const std::vector<std::string>& queue_list = queue.list();
        for (size_t i = 0; i < queue_list.size(); i++) {
            std::string step = queue.active();
            if (step != "<NULL>")
                queue.complete(step);
            if (queue.used_in_trigger()) {
                msg.clear();
                msg += Str::CHILD_CMD();
                msg += "queue ";
                msg += queue.name();
                msg += " complete";
                msg += " ";
                msg += step;
                msg += t->absNodePath();
                log(Log::MSG, msg);

                if (!doJobSubmission(theDefs, errorMsg)) {
                    level_--;
                    return false;
                }
            }
        }
    }
    return true;
}

} // namespace ecf
