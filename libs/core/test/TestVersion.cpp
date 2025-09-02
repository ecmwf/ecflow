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

template <typename... T>
bool contains(std::string_view value, T&&... pattern) {
    return ((value.find(pattern) != std::string::npos) && ...);
}

std::string find_cmake_version(const std::vector<std::string>& cmake_content) {
    // Assume a lines like the following:
    //   project( ecflow LANGUAGES CXX VERSION X.Y.Z )
    //
    // Find token `VERSION` and return the next token

    for (auto& line : cmake_content) {
        if (contains(line, "project", "ecflow", "LANGUAGES", "CXX", "VERSION")) {
            std::vector<std::string> tokens;
            Str::split(line, tokens);

            auto version_arg = std::find(tokens.begin(), tokens.end(), "VERSION");
            auto version_val = version_arg + 1;
            if (version_val != tokens.end()) {
                return *version_val;
                break;
            }
        }
    }

    return "";
}

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Version)

BOOST_AUTO_TEST_CASE(test_version) {
    std::string desc = Version::description();
    BOOST_CHECK_MESSAGE(!desc.empty(), "Expected version");
    ECF_NAME_THIS_TEST(<< ", found version: " << desc);
}

BOOST_AUTO_TEST_CASE(test_version_raw_components) {
    ECF_NAME_THIS_TEST();

    auto actual   = Version::base();
    auto expected = Version::major() + "." + Version::minor() + "." + Version::patch();
    BOOST_CHECK_EQUAL(actual, expected);
}

BOOST_AUTO_TEST_CASE(test_version_full_components) {
    ECF_NAME_THIS_TEST();

    auto actual   = Version::full();
    auto expected = Version::major() + "." + Version::minor() + "." + Version::patch() + Version::suffix();
    BOOST_CHECK_EQUAL(actual, expected);
}

BOOST_AUTO_TEST_CASE(test_version_against_cmake) {
    ECF_NAME_THIS_TEST();

    // Load CMakeList.txt
    std::string version_cmake_file = File::root_source_dir() + "/CMakeLists.txt";
    std::vector<std::string> lines;
    BOOST_REQUIRE_MESSAGE(File::splitFileIntoLines(version_cmake_file, lines, true /* ignore empty lines */),
                          "Failed to open file " << version_cmake_file << " (" << strerror(errno) << ")");
    BOOST_REQUIRE_MESSAGE(!lines.empty(), "File " << version_cmake_file << " does not contain version info ??");

    // Find CMake project version
    auto actual_version = find_cmake_version(lines);
    BOOST_CHECK_MESSAGE(!actual_version.empty(), "Unable to find CMake version in file " << version_cmake_file);

    auto expected_version = Version::base();
    BOOST_CHECK_EQUAL(actual_version, expected_version);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
