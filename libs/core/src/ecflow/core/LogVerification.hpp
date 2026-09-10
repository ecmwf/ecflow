/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_core_LogVerification_HPP
#define ecflow_core_LogVerification_HPP

#include <string>
#include <vector>

namespace ecf {

class LogVerification {
public:
    // Disable default construction
    LogVerification() = delete;

    /// Given a log file, extract in order. The node_path and the state
    static bool extractNodePathAndState(const std::string& logfile,
                                        std::vector<std::pair<std::string, std::string>>& pathStateVec,
                                        std::string& errorMsg);

    /// Will compare the input log file, with gold reference.
    /// Will compare the node state changes only
    /// Compensate for states that are scheduler dependent
    static bool
    compareNodeStates(const std::string& logfile, const std::string& goldenRefLogFile, std::string& errorMsg);
};

} // namespace ecf

#endif /* ecflow_core_LogVerification_HPP */
