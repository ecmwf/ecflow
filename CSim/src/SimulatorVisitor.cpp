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

///////////////////////////////////////////////////////////////////////////////
SimulatorVisitor::SimulatorVisitor(const std::string& defs_filename)
: defs_filename_(defs_filename),
  foundTasks_(false),
  foundCrons_(false),
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
	   has_end_clock_ = true; // no need to determine max_length, user specfied
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
}

}
