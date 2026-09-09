/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_simulator_test_TestUtil_HPP
#define ecflow_simulator_test_TestUtil_HPP

#include <string>

/// Returns the location of the defs file, such that it is in the test data area
std::string findTestDataLocation(const std::string& defsFile);

#endif /* ecflow_simulator_test_TestUtil_HPP */
