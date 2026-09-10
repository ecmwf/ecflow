/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/core/Pid.hpp"

#include <stdexcept>
#include <unistd.h> // for getpid

#include "ecflow/core/Converter.hpp"

std::string Pid::getpid() {
    std::string pid;
    try {
        pid = ecf::convert_to<std::string>(::getpid());
    }
    catch (const ecf::bad_conversion&) {
        throw std::runtime_error("Pid::getpid(): Could not convert PID to a string\n");
    }
    return pid;
}

std::string Pid::unique_name(const std::string& prefix) {
    std::string ret = prefix;
    ret += "_";
    ret += Pid::getpid();
    return ret;
}
