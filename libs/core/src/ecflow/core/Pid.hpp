/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Pid_HPP
#define ecflow_core_Pid_HPP

#include <string>

class Pid {
public:
    // Disable default construction
    Pid() = delete;
    // Disable copy (and move) semantics
    Pid(const Pid&)                  = delete;
    const Pid& operator=(const Pid&) = delete;

    /// Returns the current Process ID (as a string); otherwise, throws exception(std::runtime_error)
    static std::string getpid();

    /// Returns a unique name, based on Process ID, composed of prefix + '_' + getpid();
    /// otherwise, throws exception(std::runtime_error)
    static std::string unique_name(const std::string& prefix);
};

#endif /* ecflow_core_Pid_HPP */
