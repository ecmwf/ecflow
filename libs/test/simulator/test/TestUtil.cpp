/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TestUtil.hpp"

#include "ecflow/core/File.hpp"

using namespace ecf;

std::string findTestDataLocation(const std::string& defsFile) {
    std::string testData = File::test_data("libs/test/simulator/test/data", "libs/test/simulator");
    testData += "/";
    testData += defsFile;
    return testData;
}
