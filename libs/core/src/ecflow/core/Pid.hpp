/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_core_Pid_HPP
#define ecflow_core_Pid_HPP

#include <string>

class Pid {
public:
    // Disable default construction
    Pid() = delete;

    /// Returns the current Process ID (as a string); otherwise, throws exception(std::runtime_error)
    static std::string getpid();

    /// Returns a unique name, based on Process ID, composed of prefix + '_' + getpid();
    /// otherwise, throws exception(std::runtime_error)
    static std::string unique_name(const std::string& prefix);
};

#endif /* ecflow_core_Pid_HPP */
