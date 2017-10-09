//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$ 
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
#include <iostream>
#include <fstream>
#include <stdlib.h>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>  // requires boost date and time lib

#include "Simulator.hpp"
#include "Analyser.hpp"
#include "SimulatorVisitor.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Log.hpp"
#include "JobsParam.hpp"
#include "Jobs.hpp"
#include "CalendarUpdateParams.hpp"
#include "CmdContext.hpp"
#include "Str.hpp"

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

//#define DEBUG_LONG_RUNNING_SUITES 1

namespace ecf {

/// Class that allows multiple log files.
class LogDestroyer {
public:
	LogDestroyer() {}
	~LogDestroyer() { Log::destroy(); }
};

Simulator::Simulator() : level_(0), print_style_(PrintStyle::STATE){}

bool Simulator::run(const std::string& theDefsFile,std::string& errorMsg) const
{
#ifdef DEBUG_LONG_RUNNING_SUITES
   cout << "Simulator::run parsing file " << theDefsFile << endl;
#endif

   Defs theDefs;
   std::string warningMsg;
   if (!theDefs.restore(theDefsFile,errorMsg,warningMsg))  return false;
   //cout << theDefs << "\n";
   return run(theDefs,theDefsFile,errorMsg, false /* don't do check, allready done */);
}

bool Simulator::run(Defs& theDefs, const std::string& defs_filename,  std::string& errorMsg, bool do_checks) const
{
   CmdContext cmd_context;

#ifdef DEBUG_LONG_RUNNING_SUITES
	std::cout << "Simulator::run " << defs_filename << endl;
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
	   if (!theDefs.check(errorMsg,warningMsg)) {
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
 	//cout << "defs_filename " << defs_filename << "\n";
	//cout << "***** after Log::create(logFileName)\n";

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
      errorMsg +=  simiVisitor.errors_found();
      return false;
	}

	// Let visitor determine calendar increment. i.e if no time dependencies we will use 1 hour increment
 	time_duration calendarIncrement =  simiVisitor.calendarIncrement();
 	boost::posix_time::time_duration max_simulation_period = simiVisitor.maxSimulationPeriod();

 	std::stringstream ss;
 	ss << " time dependency(" <<  simiVisitor.hasTimeDependencies()
		<< ") max_simulation_period(" << to_simple_string(max_simulation_period)
		<< ") calendarIncrement(" << to_simple_string(calendarIncrement) << ")";
 	std::string msg = ss.str();
 	log(Log::MSG,msg);
#ifdef DEBUG_LONG_RUNNING_SUITES
 	cout << defs_filename << msg << "\n";
#endif

 	// Do we have autocancel, must be done before.
   int hasAutoCancel = 0;
   BOOST_FOREACH(suite_ptr s, theDefs.suiteVec()) { if (s->hasAutoCancel()) hasAutoCancel++; }

   // ==================================================================================
	// Start simulation ...
 	// Assume: User has taken into account autocancel end time.
   // ==================================================================================
 	CalendarUpdateParams calUpdateParams( calendarIncrement );
	boost::posix_time::time_duration duration(0,0,0,0);
 	while (duration <= max_simulation_period) {

#ifdef DEBUG_LONG_RUNNING_SUITES
      BOOST_FOREACH(suite_ptr ss, theDefs.suiteVec()) {
         cout << "duration: " << to_simple_string(duration) << " " << ss->calendar().toString() << endl;
      }
#endif

 		// Resolve dependencies and submit jobs
 		if (!doJobSubmission(theDefs,errorMsg)) return false;

		// Increment calendar.
		theDefs.updateCalendar( calUpdateParams );
		duration += calendarIncrement;
  	}

 	// ==================================================================================
 	// END of simulation
 	// ==================================================================================
 	if ( !theDefs.checkInvariants(errorMsg)) {
 	   return false;
 	}

   // Cater for suite, with no verify attributes, but which are not complete. testAnalysis.cpp
 	// Ignore suites with crons as they will never complete
 	// Ignore suites with autocancel, as suite may get deleted
 	if (!simiVisitor.foundCrons() && (hasAutoCancel == 0)) {
 	   size_t completeSuiteCnt = 0;
 	   BOOST_FOREACH(suite_ptr s, theDefs.suiteVec()) { if (s->state() == NState::COMPLETE) completeSuiteCnt++; }

 	   if ( (theDefs.suiteVec().size() != completeSuiteCnt)) {
 	      std::stringstream ss; ss << "Defs file " << defs_filename << "\n";
 	      BOOST_FOREACH(suite_ptr s, theDefs.suiteVec()) {
 	         ss << "  suite '/" << s->name() << " has not completed\n";
 	      }
 	      errorMsg += ss.str();

 	      run_analyser(theDefs,errorMsg);
 	      return false;
 	   }
 	}

 	if (theDefs.verification(errorMsg)) {
 	   return true;
 	}

 	run_analyser(theDefs,errorMsg);
 	return false;
}

void Simulator::run_analyser(Defs& theDefs,std::string& errorMsg ) const
{
   Analyser analyser;
   analyser.run(theDefs);
   errorMsg += "Please see files .flat and .depth for analysis\n";

   PrintStyle style(PrintStyle::MIGRATE);
   std::stringstream ss;
   ss << theDefs;
   errorMsg += ss.str();
}

bool Simulator::doJobSubmission(Defs& theDefs, std::string& errorMsg) const
{
	// For the simulation we ensure job submission takes less than 10 seconds
	// Resolve dependencies and submit jobs
	JobsParam jobsParam(10 /*submitJobsInterval */, false /*create jobs*/); // spawn jobs *will* be set to false
	Jobs jobs(&theDefs);
	if (!jobs.generate(jobsParam)) {
		ecf::log(Log::ERR, jobsParam.getErrorMsg());
		assert(false);
		return false;
	}

//#ifdef DEBUG_LONG_RUNNING_SUITES
//	cout << "Simulator::doJobSubmission jobsParam.submitted().size() " << jobsParam.submitted().size() << " level = " << level_ << endl;
//#endif

	level_++;

	// For those jobs that were submitted, Simulate client by going
	// through the task events and meters and updating them and then
	// re-checking for job submission. Finally mark task as complete
	// This is important as there may be other task dependent on this.
	BOOST_FOREACH(Submittable* t, jobsParam.submitted()) {

#ifdef DEBUG_LONG_RUNNING_SUITES
		// If task repeating themselves, determine what is causing this:
		std::map<Submittable*,int>::iterator i = taskIntMap_.find(t);
		if (i == taskIntMap_.end())  taskIntMap_.insert( std::make_pair(t,1));
		else {
			(*i).second++;
			// Find top most node that has a repeat, check if its incrementing:
 			Node*   nodeWithRepeat = NULL;
			Node* theParent = t->parent();
			while (theParent) {
				if (!theParent->repeat().empty())  nodeWithRepeat = theParent;
				theParent = theParent->parent();
			}
			//cout << t->suite()->calendar().toString();
			if ( nodeWithRepeat) {
				cout << " level " << level_ << " submitted task " << t->debugNodePath() << " count = " << (*i).second
				     << " HAS parent Repeating node " << nodeWithRepeat->debugNodePath()
				     << " " << nodeWithRepeat->repeat().dump() << endl;
			}
			else {  // cound be a cron
				cout << " level " << level_ << " submitted task " << t->debugNodePath() << " count = " << (*i).second  << endl;
			}
		}
#endif



#ifdef DEBUG_LONG_RUNNING_SUITES
      cout << t->debugNodePath() << " completes at " << t->suite()->calendar().toString() << " level " << level_ << " parent state:" <<  endl;
#endif

		// If the task has any event used in the trigger expressions, then update event.
      std::string msg;
 		BOOST_FOREACH(Event& event, t->ref_events()) {
 			if (event.usedInTrigger()) { // event used in trigger/complete expression
 				event.set_value(true);

 				msg.clear();
 				msg += Str::CHILD_CMD();
 				msg += "event ";
 				msg += event.name_or_number();
 				msg += " ";
 				msg += t->absNodePath();
 				log(Log::MSG,msg);

  				if (!doJobSubmission(theDefs,errorMsg))  {
  					level_--;
  					return false;
  				}
 			}
 			else {
 				// warn about events not used in any trigger or complete expressions
// 				cout << "submitted task " << t->debugNodePath() << " UN-USED event " << event.toString() << "\n";
 			}
  		}

		// if the task has any meters used in trigger expressions, then increment meters
 		BOOST_FOREACH(Meter& meter, t->ref_meters()) {
 			if (meter.usedInTrigger()) { // meter used in trigger/complete expression
 				while (meter.value() < meter.max()) {
 					meter.set_value(meter.value()+1);

 	            msg.clear();
 	            msg += Str::CHILD_CMD();
 	            msg += "meter ";
 	            msg += meter.name();
 	            msg += " ";
 	            msg += t->absNodePath();
 	            log(Log::MSG,msg);

  					if (!doJobSubmission(theDefs,errorMsg)) {
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

      // any state change should be followed with a job submission
      t->complete();  // mark task as complete
	}

	level_--;
	return true;
}

}
