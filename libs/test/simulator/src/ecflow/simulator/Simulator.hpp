/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_simulator_Simulator_HPP
#define ecflow_simulator_Simulator_HPP

#include <map>
#include <string>
#include <vector>

#include "ecflow/core/PrintStyle.hpp"

class QueueAttr;
class Defs;
class Submittable;

namespace ecf {

// This class is used to simulate a definition file. This is useful because:
//   a/ Saves time over involving server and asking client to play definition file
//   b/ Informs about any parser errors in the definition file
//   c/ Informs about any deadlocks, i.e. if a suite does not complete
//   d/ Simulates both real and hybrid clocks
//   e/ Simulation will by default run for a year. Should really use start/end clock for accurate simulations
class Simulator {
public:
    // For deterministic results simulate using clock(start) and endclock(finish)
    // Otherwise default to run simulation for:
    // No time dependencies: simulate for 24 hours
    //    time || today // 24 hours
    //    day           // 1 week
    //    date          // 1 month
    //    cron          // 1 year
    //    repeat        // 1 year
    Simulator() = default;

    Simulator(const Simulator&)            = delete;
    Simulator& operator=(const Simulator&) = delete;
    Simulator(Simulator&&)                 = delete;
    Simulator& operator=(Simulator&&)      = delete;

    /// return true if all ok else returns false;
    bool run(Defs&, const std::string& defs_filename, std::string& errorMsg, bool do_checks = true) const;
    bool run(const std::string& theDefsFile, std::string& errorMsg) const;

private:
    bool doJobSubmission(Defs&, std::string& errorMsg) const;
    void run_analyser(Defs& theDefs, std::string& errorMsg) const;

    bool update_for_queues(Submittable* t,
                           std::string& msg,
                           std::vector<QueueAttr>& queues,
                           Defs& theDefs,
                           std::string& errormsg) const;

    mutable std::map<Submittable*, int> taskIntMap_;
    mutable int level_{0};
};

} // namespace ecf

#endif /* ecflow_simulator_Simulator_HPP */
