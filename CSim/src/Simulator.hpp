#ifndef SIMULATOR_HPP_
#define SIMULATOR_HPP_

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

#include <boost/noncopyable.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <string>
#include <map>
class Defs;
class Task;
namespace ecf { class SimulatorVisitor;}

namespace ecf {

/// This class is used to simulate a definition file. This is use full
/// because:
//       a/ Save time over involving server and asking client to play definition file
//       b/ Tells you of any parser errors in the definition file
//       c/ Tells you about any deadlocks, ie if suite does not complete
//       d/ Will simulate for both real and hybrid clocks
//       e/ Simulation will by default run for a year.
class Simulator : private boost::noncopyable {
public:
	// default to run simulation for 1 year, or until suites complete if there are time dependencies 8784 =  366 X 24
	// Otherwise will simulate for 24 hours
 	Simulator(const boost::posix_time::time_duration& period = boost::posix_time::time_duration(8784,0,0,0));

 	/// Some definition file will run forever. **NOT USED ***, kept for reference. Need a better mechanism
  	void truncateLongRepeats(int truncateTo) { truncateLongRepeatsTo_ = truncateTo ;}

 	/// return true if all ok else returns false;
	bool run(Defs&, const std::string& defs_filename, std::string& errorMsg, bool do_checks = true) const;
	bool run(const std::string& theDefsFile, std::string& errorMsg) const;

private:

	bool abortSimulation(const ecf::SimulatorVisitor&, const boost::posix_time::time_duration& duration,std::string& message) const;
	bool doJobSubmission(Defs&, std::string& errorMsg) const;

	mutable boost::posix_time::time_duration max_simulation_period_;
	mutable std::map<Task*,int> taskIntMap_;
	int truncateLongRepeatsTo_;
	mutable int level_;
	mutable bool foundCrons_;
};
}
#endif /* SIMULATOR_HPP_ */
