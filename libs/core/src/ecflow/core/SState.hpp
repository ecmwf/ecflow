/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_SState_HPP
#define ecflow_core_SState_HPP

#include <string>

class SState {
public:
    /// The following table shows the effect of state, on server behaviour:
    ///
    ///           User Request    Task Request   Job Scheduling   Check-pointing
    /// RUNNING      yes               yes              yes            yes
    /// SHUTDOWN     yes               yes              no             yes
    /// HALTED       yes               no               no             no
    enum State { HALTED, SHUTDOWN, RUNNING };

    // Disable default construction
    SState() = delete;
    // Disable copy (and move) semantics
    SState(const SState&)                  = delete;
    const SState& operator=(const SState&) = delete;

    /// Given an integer return the server state as a string.
    /// If status is not 0, 1, or 2, returns "UNKNOWN".
    static std::string to_string(int status);
    static std::string to_string(SState::State);

    static SState::State toState(const std::string&);
    static bool isValid(const std::string&);
};

#endif /* ecflow_core_SState_HPP */
