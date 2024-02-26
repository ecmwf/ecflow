/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_simulator_test_TestUtil_HPP
#define ecflow_simulator_test_TestUtil_HPP

#include <string>

/// Returns the location of the defs file, such thats it in the test data area
std::string findTestDataLocation(const std::string& defsFile);

#endif /* ecflow_simulator_test_TestUtil_HPP */
