#ifndef SIMULATORVISITOR_HPP_
#define SIMULATORVISITOR_HPP_

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
#include <sstream>
#include <set>
#include <vector>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "NodeTreeVisitor.hpp"
#include "NodeFwd.hpp"

namespace ecf {

class SimulatorVisitor : public NodeTreeVisitor {
public:
   explicit SimulatorVisitor(const std::string& defs_filename);

	/// If we have crons and no endclock then show error message
	const std::string& errors_found() const { return error_msg_;}

 	/// Crons run for ever. Detect them so that we can abort early
   bool foundCrons() const { return foundCrons_;}

 	/// returns true if defs has time,date,today, date time based attributes
 	bool hasTimeDependencies() const { return hasTimeDependencies_;}

 	/// Determine the max simulation period in hours over all the suites.
 	/// We will default to a year( 8784 =  366 X 24)
 	/// However by going through and looking at the repeats, we can get a better idea
 	boost::posix_time::time_duration maxSimulationPeriod() const { return max_sim_duration_;}

 	/// return the max simulation period for the given suite.
 	/// Allow definition with multiple suite with different simulation periods
   boost::posix_time::time_duration max_simulation_period(Suite* s) const;

  	// default calendar increment is one minute, however if we have no time dependencies,
 	// then simulation can be speeded up, i.e by using hour increment
 	const boost::posix_time::time_duration& calendarIncrement() const { return ci_;}

	bool traverseObjectStructureViaVisitors() const override { return true;}
	void visitDefs(Defs*) override;
	void visitSuite(Suite*) override;
	void visitFamily(Family*) override;
	void visitNodeContainer(NodeContainer*) override;
	void visitTask(Task*) override;

private:
	/// Commented out since, we need to find a way of truncating lon repeats
	/// without changing repeat structure.
//	void analyse(Node* node);
//	int truncateRepeats_;  // allow for simulation to complete earlier. ***NOT USED, kept for reference *****

	std::string defs_filename_;
	std::string error_msg_;
	bool foundTasks_;
	bool foundCrons_;
	bool foundTime_;
	bool hasTimeDependencies_;
	bool has_end_clock_;
	boost::posix_time::time_duration max_sim_duration_;
	boost::posix_time::time_duration max_suite_duration_;
	boost::posix_time::time_duration ci_;
	std::vector< std::pair<Suite*,boost::posix_time::time_duration> > suite_duration_vec_;
};

}
#endif
