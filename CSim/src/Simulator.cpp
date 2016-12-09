//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$ 
//
// Copyright 2009-2016 ECMWF. 
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
#include "DefsStructureParser.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Log.hpp"
#include "JobsParam.hpp"
#include "Jobs.hpp"
#include "CalendarUpdateParams.hpp"
#include "CmdContext.hpp"

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

Simulator::Simulator(const boost::posix_time::time_duration& period)
: max_simulation_period_(period),
  truncateLongRepeatsTo_(0),
  level_(0),
  foundCrons_(false)
{
#ifdef DEBUG_LONG_RUNNING_SUITES
	std::cout << "Simulator::Simulator max_simulation_period_ " << to_simple_string(max_simulation_period_) << endl;
#endif
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
 	// **** Need a better mechanism of handling long repeats, the old way of changing repeat
 	// **** attributes is not acceptable.(i.e user could save in python after simulation
 	// **** and there defs would be corrupted
	SimulatorVisitor simiVisitor(truncateLongRepeatsTo_ /* NOT USED */);
	theDefs.acceptVisitTraversor(simiVisitor);
	foundCrons_ = simiVisitor.foundCrons();

	if (!simiVisitor.foundTasks()) {
		errorMsg += "The defs file ";
		errorMsg += defs_filename;
		errorMsg +=  " has no tasks, can not simulate\n";
 		return false;
	}

	// Let visitor determine calendar increment. i.e if no time dependencies we will use 1 hour increment
	// Default max_simulation_period_ is 1 year, however some operation suites run for many years
	// Analyse the repeats to determine max simulation period
 	time_duration calendarIncrement =  simiVisitor.calendarIncrement();
 	boost::posix_time::time_duration max_simulation_period = simiVisitor.maxSimulationPeriod();
 	if ( max_simulation_period > max_simulation_period_) max_simulation_period_ = max_simulation_period;

#ifdef DEBUG_LONG_RUNNING_SUITES
 	cout << defs_filename << " time dependency = " <<  simiVisitor.hasTimeDependencies()
		<< " max_simulation_period_=" << to_simple_string(max_simulation_period_)
		<< " calendarIncrement=" << to_simple_string(calendarIncrement) << endl;
#endif


 	CalendarUpdateParams calUpdateParams( calendarIncrement );

	// Start simulation ...
	boost::posix_time::time_duration duration(0,0,0,0);
 	while (1) {
#ifdef DEBUG_LONG_RUNNING_SUITES
      BOOST_FOREACH(suite_ptr ss, theDefs.suiteVec()) {
         cout << "duration: " << to_simple_string(duration) << " " << ss->calendar().toString() << endl;
      }
#endif

 		// Resolve dependencies and submit jobs
 		if (!doJobSubmission(theDefs,errorMsg)) return false;

 		// Determine termination criteria. If all suite complete, exit,
 		// Let simulation termination take autocancel into account. i.e we extend simulation even though
 		// suite may have completed, to allow autocancel to take effect.
 		// Hence if a suite has complete, but has autocancel we continue simulation
 		// Note: should also handle case of autocancel which remove all suites
		size_t completeSuiteCnt = 0;
		int hasAutoCancel = 0;
		BOOST_FOREACH(suite_ptr s, theDefs.suiteVec()) {
			if (s->state() == NState::COMPLETE) completeSuiteCnt++;
			if (s->hasAutoCancel()) hasAutoCancel++;
		}
		if ( (theDefs.suiteVec().size() == completeSuiteCnt) && (hasAutoCancel == 0)) {
 			LOG(Log::MSG, "Simulation complete in " << to_simple_string(duration) );

 			if ( !theDefs.checkInvariants(errorMsg) )  break;

			// Run verification on the completed suite
  			return theDefs.verification(errorMsg);
		}

		// crons run for ever. To terminate, we rely on test to have Verify attributes
		// The verify attributes should *only* be on the task. (i.e task auto-reques
		// hance parent is never complete, and hence no point in having verify attributes
		if (foundCrons_) {
		   std::string msg;
		   if (theDefs.verification(msg)) {
		      return true;
		   }
		}

		// if simulation runs to long bomb out.,  then Analyse the defs,
		// to determine why the simulation would not complete
		if (abortSimulation(simiVisitor, duration, errorMsg) ) {

 			if ( !theDefs.checkInvariants(errorMsg) )  break;

 			if ( (theDefs.suiteVec().size() == completeSuiteCnt) && (hasAutoCancel != 0)) {
 				errorMsg += "All suites have completed, but autocancel has not taken effect?\n";
 			}

 			if (foundCrons_) {
 			   std::string msg;
 			   if (!theDefs.verification(msg)) {
 			      errorMsg += msg;
 			      errorMsg += "\n";
 			   }
 			}

  			Analyser analyser;
  			analyser.run(theDefs);
  			errorMsg += "Please see files .flat and .depth for analysis\n";

  			PrintStyle::setStyle(PrintStyle::MIGRATE);
  			std::stringstream ss;
  			ss << theDefs;
  			errorMsg += ss.str();
			return false;
		}

		// Increment calendar.
		theDefs.updateCalendar( calUpdateParams );
		duration += calendarIncrement;
  	}

 	return false;
}


bool Simulator::run(const std::string& theDefsFile,std::string& errorMsg) const
{
#ifdef DEBUG_LONG_RUNNING_SUITES
 	cout << "Simulator::run parsing file " << theDefsFile << endl;
#endif

 	Defs theDefs;
	DefsStructureParser checkPtParser( &theDefs , theDefsFile );
	std::string warningMsg;
 	if (!checkPtParser.doParse(errorMsg,warningMsg))  return false;

 	return run(theDefs,theDefsFile,errorMsg, false /* don't do check, allready done */);
}

//---------------------------------------------------------------------------------------

bool Simulator::abortSimulation( const SimulatorVisitor& simiVisitor,
                                 const boost::posix_time::time_duration& duration,
                                 std::string& errorMsg) const
{
	if (duration > max_simulation_period_) {

		errorMsg = "\nTimed out after ";
		errorMsg += to_simple_string(max_simulation_period_);
		errorMsg += " hours of simulation.\n";

		if (!simiVisitor.hasTimeDependencies())  errorMsg += "The definition has no time dependencies.\n";
		return true;
	}

	return false;
}

bool Simulator::doJobSubmission(Defs& theDefs, std::string& errorMsg) const
{
	// For the simulation we ensure job submission takes less than 2 seconds
	int submitJobsInterval = 10;

	// Resolve dependencies and submit jobs
	JobsParam jobsParam(submitJobsInterval, false /*create jobs*/); // spawn jobs *will* be set to false
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

		// If the task has any event used in the trigger expressions, then update event.
 		BOOST_FOREACH(Event& event, t->ref_events()) {

 			if (event.usedInTrigger()) { // event used in triger/complete expression
 				event.set_value(true);
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
 		t->complete();  // Finally mark task as complete

 		// crons run for ever. To terminate, we rely on test to have Verify attributes
 		if (foundCrons_) {
 		   std::string msg;
 		   t->verification(msg);
 		   if (msg.empty()) {
 		      return true;
 		   }
 		}

#ifdef DEBUG_LONG_RUNNING_SUITES
		cout << t->debugNodePath() << " completes at " << t->suite()->calendar().toString() << " level " << level_ << endl;
#endif
	   // for autocancel
		if (!doJobSubmission(theDefs,errorMsg)) {
			level_--;
			return false;
		}
	}

	level_--;
	return true;
}

}
