/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/File.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;
using namespace boost;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Version)

BOOST_AUTO_TEST_CASE(test_version) {
    std::string desc = Version::description();
    BOOST_CHECK_MESSAGE(!desc.empty(), "Expected version");
    ECF_NAME_THIS_TEST(<< ", found version: " << desc);
}

BOOST_AUTO_TEST_CASE(test_version_against_cmake) {
    ECF_NAME_THIS_TEST();

    // Open the file CMakeList.txt
    std::string version_cmake_file = File::root_source_dir() + "/CMakeLists.txt";
    std::vector<std::string> lines;
    BOOST_REQUIRE_MESSAGE(File::splitFileIntoLines(version_cmake_file, lines, true /* impore empty lines */),
                          "Failed to open file " << version_cmake_file << " (" << strerror(errno) << ")");
    BOOST_REQUIRE_MESSAGE(!lines.empty(), "File " << version_cmake_file << " does not contain version info ??");

    // Expecting lines like:
    //   project( ecflow LANGUAGES CXX VERSION 5.3.1 )
    // Compare against VERSION
    std::string cmake_version;
    for (auto& line : lines) {
        std::vector<std::string> tokens;
        Str::split(line, tokens);

        if (line.find("project") != std::string::npos && line.find("ecflow") != std::string::npos &&
            line.find("LANGUAGES") != std::string::npos && line.find("CXX") != std::string::npos &&
            line.find("VERSION") != std::string::npos) {
            for (size_t i = 0; i < tokens.size(); i++) {
                if (tokens[i] == "VERSION") {
                    if (i + 1 < tokens.size()) {
                        cmake_version = tokens[i + 1];
                        break;
                    }
                }
            }
        }
    }

    // The if they don't match, we have failed to regenrate and check in ecflow_version.h
    BOOST_REQUIRE_MESSAGE(!cmake_version.empty(),
                          "Expected to find 'project( ecflow LANGUAGES CXX VERSION N.N.N )' in file "
                              << version_cmake_file);
    BOOST_REQUIRE_MESSAGE(
        Version::raw() == cmake_version,
        "\n  Expected "
            << cmake_version << " but found " << Version::raw()
            << ", Please regenerate file $WK/libs/core/src/ecflow_version.h by calling 'sh -x $WK/cmake.sh'");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
