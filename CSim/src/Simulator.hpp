#ifndef SIMULATOR_HPP_
#define SIMULATOR_HPP_

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

#include <boost/noncopyable.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <string>
#include <map>

#include "PrintStyle.hpp"

class Defs;
class Task;
class Submittable;
namespace ecf { class SimulatorVisitor;}

namespace ecf {

/// This class is used to simulate a definition file. This is use full
/// because:
//       a/ Save time over involving server and asking client to play definition file
//       b/ Tells you about any parser errors in the definition file
//       c/ Tells you about any deadlocks, ie if suite does not complete
//       d/ Will simulate for both real and hybrid clocks
//       e/ Simulation will by default run for a year. Should really use start/end clock for accurate simulations
class Simulator : private boost::noncopyable {
public:
   // For deterministic results simulate using clock(start) and endclock(finish)
   // Otherwise default to run simulation for:
   // No time dependencies: simulate for 24 hours
   //    time || today // 24 hours
   //    day           // 1 week
   //    date          // 1 month
   //    cron          // 1 year
   //    repeat        // 1 year
 	Simulator();

 	/// return true if all ok else returns false;
	bool run(Defs&, const std::string& defs_filename, std::string& errorMsg, bool do_checks = true) const;
	bool run(const std::string& theDefsFile, std::string& errorMsg) const;

private:

	bool doJobSubmission(Defs&, std::string& errorMsg) const;
	void run_analyser(Defs& theDefs,std::string& errorMsg ) const;

	mutable std::map<Submittable*,int> taskIntMap_;
	mutable int level_;
	PrintStyle print_style_;   // by default show state when writing defs to standard out. RAII
};
}
#endif /* SIMULATOR_HPP_ */
