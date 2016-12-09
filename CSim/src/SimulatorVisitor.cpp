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
#include "SimulatorVisitor.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Indentor.hpp"

using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

//#define DEBUG_VISITOR 1

namespace ecf {

///////////////////////////////////////////////////////////////////////////////
SimulatorVisitor::SimulatorVisitor(const std::string& defs_filename,int /* truncateRepeats */)
: /* truncateRepeats_(truncateRepeats), */
  defs_filename_(defs_filename),
  foundTasks_(false),
  foundCrons_(false),
  hasTimeDependencies_(false),
  max_length_(0),
  ci_(hours(1))
  {}

void SimulatorVisitor::visitDefs( Defs* d) {
	BOOST_FOREACH(suite_ptr s, d->suiteVec()) { s->acceptVisitTraversor(*this); }
}

void SimulatorVisitor::visitSuite( Suite* s)   {

#ifdef DEBUG_VISITOR
	cout << "SimulatorVisitor::visitSuite " << s->debugNodePath() << "\n";
#endif

	/// begin , will cause creation of generated variables. The generated variables
	/// are use in client scripts and used to locate the ecf files.
	s->begin();

	// Found time dependencies use calendar increment of one minute
	if (s->hasTimeDependencies()) {
		hasTimeDependencies_ = true;
	}

 	visitNodeContainer(s);

   // IF the suite has no task  (i.e could consist of just limits, set suite to complete
   // Since we rely on it for termination of tests
   // make setting NState::COMPLETE is after begin(), which will set Node into the queued state
 	if (!foundTasks_) {
      s->set_state(NState::COMPLETE);
 	}
}

void SimulatorVisitor::visitFamily( Family* f) { visitNodeContainer(f);}

void SimulatorVisitor::visitNodeContainer(NodeContainer* nc)
{
   if (ci_ == hours(1)) nc->get_time_resolution_for_simulation(ci_);

   if (!nc->crons().empty()) {
      foundCrons_ = true;
      cout << defs_filename_ << ": Found crons on NodeContainer\n";
   }

   // If suite has repeat day attribute( a infinite repeat), it will run forever, hence disable this for simulation purposes
   /// reset will clear the invalid flag., when doing a real job submission.
   /// *** this must be placed after begin() since begin() will reset all attributes() *****
   if (nc->ref_repeat().makeInfiniteInValid()) {
      cout << defs_filename_ << ": Disabling '" << nc->repeat().dump()
           << "' attribute of " << nc->debugNodePath()
           << ". This will allow simulation to complete earlier.\n";
   }

	BOOST_FOREACH(node_ptr t, nc->nodeVec()) { t->acceptVisitTraversor(*this);}
}

void SimulatorVisitor::visitTask( Task* t )
{
   if (ci_ == hours(1)) t->get_time_resolution_for_simulation(ci_);

   foundTasks_ = true;

   if (!t->crons().empty()) {
      foundCrons_ = true;
      // cout << defs_filename_ << ": Found crons on task\n";
   }
}

boost::posix_time::time_duration SimulatorVisitor::maxSimulationPeriod() const
{
	if ( hasTimeDependencies_)  return hours(max_length_);
	return  hours(24);
}

/// Commented out, since we need to find a better mechanism of truncating long repeats
/// without change repeat structure/attributes. (i.e need a simulation mode, with a max length
/// that is ignored in the server.
//void SimulatorVisitor::analyse(Node* node)
//{
//	if (!node->repeat().empty()) {
//		int lengthInDays = node->repeat().length();
//
//#ifdef DEBUG_VISITOR
//		cout << "SimulatorVisitor::analyse " << node->debugNodePath() << " " << node->repeat().dump() << " length = " << lengthInDays << "\n";
//#endif
//
//		// **************************************************************************
//		// ****** CAUTION: Truncate make a change to defs structure. Use with care
//		// **************************************************************************
//		if (truncateRepeats_ != 0  && lengthInDays > truncateRepeats_) {
//			node->repeat_.truncate(truncateRepeats_);
//			lengthInDays = node->repeat().length();
//		}
//
//		lengthInDays *= 24; // convert to hours Day of month value
//
//		if ( lengthInDays > max_length_) {
//			max_length_ = lengthInDays;
//
//#ifdef DEBUG_VISITOR
//			cout << "max_length_ = " << max_length_ << " hours \n";
//#endif
//		}
//	}
//}

}
