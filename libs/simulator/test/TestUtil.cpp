/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TestUtil.hpp"

#include "ecflow/core/File.hpp"

using namespace std;
using namespace ecf;

std::string findTestDataLocation(const std::string& defsFile) {
    std::string testData = File::test_data("libs/simulator/test/data", "libs/simulator");
    testData += "/";
    testData += defsFile;
    return testData;
}
