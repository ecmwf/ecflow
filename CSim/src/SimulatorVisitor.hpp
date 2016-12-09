#ifndef SIMULATORVISITOR_HPP_
#define SIMULATORVISITOR_HPP_

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

#include "NodeTreeVisitor.hpp"
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <iostream>
#include <sstream>
#include <set>
#include <vector>
class Node;

namespace ecf {

class SimulatorVisitor : public NodeTreeVisitor {
public:
	SimulatorVisitor(int truncateRepeats);

	/// If the definition file has suites with no tasks, ie they could have server limits
	/// then for simulation purposes(i.e when all suites complete we terminate simulation)
 	/// mark these as complete. Must be done *AFTER* beginAll() which sets all nodes to queued state
 	bool foundTasks() const { return foundTasks_;}

 	/// Crons run for ever. Detect them so that we can abort early
   bool foundCrons() const { return foundCrons_;}

 	/// returns true if defs has time,date,today, date time based attributes
 	bool hasTimeDependencies() const { return hasTimeDependencies_;}

 	/// Determine the max simulation period in hours. We will default to a years 8784 =  366 X 24
 	/// However by going through and looking at the repeats, we can get a better idea
 	boost::posix_time::time_duration maxSimulationPeriod() const;

  	// default calendar increment is one minute, however if we have no time dependencies,
 	// then simulation can be speeded up, i.e by using hour increment
 	const boost::posix_time::time_duration& calendarIncrement() const { return ci_;}

	virtual bool traverseObjectStructureViaVisitors() const { return true;}
	virtual void visitDefs(Defs*);
	virtual void visitSuite(Suite*);
	virtual void visitFamily(Family*);
	virtual void visitNodeContainer(NodeContainer*);
	virtual void visitTask(Task*);

private:
	/// Commented out since, we need to find a way of truncating lon repeats
	/// without changing repeat structure.
//	void analyse(Node* node);
//	int truncateRepeats_;  // allow for simulation to complete earlier. ***NOT USED, kept for reference *****

	bool foundTasks_;
	bool foundCrons_;
	bool hasTimeDependencies_;
	int  max_length_;
 	boost::posix_time::time_duration ci_;
};

}
#endif
