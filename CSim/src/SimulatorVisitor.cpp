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
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "SimulatorVisitor.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Indentor.hpp"
#include "Log.hpp"

using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

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
  max_length_(hours(24)),
  ci_(hours(1))
  {}

void SimulatorVisitor::visitDefs( Defs* d) {
	BOOST_FOREACH(suite_ptr s, d->suiteVec()) { s->acceptVisitTraversor(*this); }
}

void SimulatorVisitor::visitSuite( Suite* s)
{
	/// begin , will cause creation of generated variables. The generated variables
	/// are use in client scripts and used to locate the ecf files.
	s->begin();

	// Found time dependencies use calendar increment of one minute
	if (s->hasTimeDependencies()) {
		hasTimeDependencies_ = true;
	}

	if ( s->clockAttr() && s->clock_end_attr()) {
	   max_length_ = s->clock_end_attr()->ptime() - s->clockAttr()->ptime();
	   has_end_clock_ = true; // no need to determine max_length, user specified
	}

 	visitNodeContainer(s);

   // IF the suite has no task  (i.e could consist of just limits, set suite to complete
   // Since we rely on it for termination of tests
   // make setting NState::COMPLETE is after begin(), which will set Node into the queued state
 	if (!foundTasks_) {
      s->set_state(NState::COMPLETE);
      std::stringstream ss; ss <<  "The defs file " <<  defs_filename_ << " has a suite '/" << s->suite()->name() << "' which has no tasks. Ignoring \n";
 	   log(Log::WAR,ss.str());
 	}

 	// If we have cron/time with calendar increment of 1 hour, where calendar start time is in minutes
 	// we will miss the time/cron based attributes, hence use 1 minute resolution
 	if ((foundCrons_ || foundTime_) && ci_ == hours(1)) {
 	   boost::posix_time::time_duration start_time = s->calendar().begin_time().time_of_day();
 	   if (start_time.minutes() != 0) {
 	      log(Log::WAR,"Found cron or time based attributes, with 1 hour resolution, however suite calendar start time has minute resolution, reverting to minute resolution for simulation.");
 	      log(Log::WAR,"To speed up resolution use suite calendar with hour setting only, i.e minutes is zero");
 	      ci_ = minutes(1);
 	   }
 	}
}

void SimulatorVisitor::visitFamily( Family* f) { visitNodeContainer(f);}

void SimulatorVisitor::visitNodeContainer(NodeContainer* nc)
{
   if (ci_ == hours(1)) nc->get_time_resolution_for_simulation(ci_);
   if (!has_end_clock_) nc->get_max_simulation_duration(max_length_);

   if (!nc->crons().empty()) {
      foundCrons_ = true;
      std::stringstream ss; ss  << defs_filename_ << ": Found crons on NodeContainer\n";
      log(Log::MSG,ss.str());
   }

   if (!nc->timeVec().empty()) {
      foundTime_= true;
   }

	BOOST_FOREACH(node_ptr t, nc->nodeVec()) { t->acceptVisitTraversor(*this);}
}

void SimulatorVisitor::visitTask( Task* t )
{
   if (ci_ == hours(1))  t->get_time_resolution_for_simulation(ci_);
   if (!has_end_clock_)  t->get_max_simulation_duration(max_length_);

   foundTasks_ = true;

   if (!t->crons().empty()) {
      foundCrons_ = true;
   }
   if (!t->timeVec().empty()) {
      foundTime_= true;
   }
}

}
