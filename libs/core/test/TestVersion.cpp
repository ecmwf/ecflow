// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <regex>
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
    return ((value.find(std::forward<T>(pattern)) != std::string::npos) && ...);
}

std::string find_cmake_version(const std::vector<std::string>& cmake_content) {
    // Assume a line like the following:
    //   project( ecflow LANGUAGES CXX VERSION X.Y.Z )
    //
    // Find token `VERSION` and return the next token

    for (auto& line : cmake_content) {
        if (contains(line, "project", "ecflow", "LANGUAGES", "CXX", "VERSION")) {
            std::vector<std::string> tokens;
            ecf::algorithm::split_at(tokens, line);

            auto version_arg = std::find(tokens.begin(), tokens.end(), "VERSION");
            auto version_val = version_arg + 1;
            if (version_val != tokens.end()) {
                return *version_val;
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

BOOST_AUTO_TEST_CASE(test_version_description_names_serialisation) {
    ECF_NAME_THIS_TEST();

    auto desc = Version::description();
    BOOST_CHECK_MESSAGE(contains(desc, "serialisation(JSON cereal " + Version::cereal() + ")"),
                        "Expected serialisation(JSON cereal " << Version::cereal() << ") in: " << desc);
    BOOST_CHECK_MESSAGE(!contains(desc, "protocol("), "Unexpected protocol(...) in: " << desc);
}

BOOST_AUTO_TEST_CASE(test_version_git_is_a_short_commit_or_none) {
    ECF_NAME_THIS_TEST();

    auto git = Version::git();
    BOOST_CHECK_MESSAGE(std::regex_match(git, std::regex("none|[0-9a-f]{12}(-dirty)?")),
                        "Expected 'none' or a 12-character commit, optionally with -dirty, but found: " << git);
}

BOOST_AUTO_TEST_CASE(test_version_description_names_git_commit_after_version) {
    ECF_NAME_THIS_TEST();

    auto desc = Version::description();
    BOOST_CHECK_MESSAGE(contains(desc, "version(" + Version::full() + ") git(" + Version::git() + ")"),
                        "Expected git(" << Version::git() << ") after version(...) in: " << desc);
}

BOOST_AUTO_TEST_CASE(test_version_aviso_is_the_version_built_against) {
    ECF_NAME_THIS_TEST();

    // ECF_TEST_EXPECTED_AVISO is set by CMake: the Aviso client library version, or 'disabled'
    BOOST_CHECK_EQUAL(Version::aviso(), std::string(ECF_TEST_EXPECTED_AVISO));
}

BOOST_AUTO_TEST_CASE(test_version_description_names_aviso) {
    ECF_NAME_THIS_TEST();

    auto desc = Version::description();
    BOOST_CHECK_MESSAGE(contains(desc, "aviso(" + Version::aviso() + ")"),
                        "Expected aviso(" << Version::aviso() << ") in: " << desc);
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
