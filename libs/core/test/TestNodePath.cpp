/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iterator> // std::ostream_iterator
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/NodePath.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Timer.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_NodePath)

BOOST_AUTO_TEST_CASE(test_unix_path_extractor) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string path;
        std::vector<std::string> expected;
    };

    std::vector<tc> test_cases = {{"suite", {"suite"}},
                                  {"/suite", {"suite"}},
                                  {"/suite/family/task", {"suite", "family", "task"}},
                                  {"/suite/family/task/", {"suite", "family", "task"}},
                                  {"/suite///family////task", {"suite", "family", "task"}},
                                  {"/suite///family////task//", {"suite", "family", "task"}},
                                  {"//suite///family////task//", {"suite", "family", "task"}},
                                  {"///suite///family////task//", {"suite", "family", "task"}},
                                  {"///suite///family////task///", {"suite", "family", "task"}}};

    for (const auto& tc : test_cases) {

        std::vector<std::string> actual;
        ecf::node::split_path(tc.path, actual);
        BOOST_CHECK_MESSAGE(actual == tc.expected,
                            "Splitting " << tc.path << ", expected " << ecf::algorithm::as_string(tc.expected)
                                         << ", found " << ecf::algorithm::as_string(actual));
    }
}

BOOST_AUTO_TEST_CASE(test_extractHostPort) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string path;
        bool expected;
        std::string expected_host;
        std::string expected_port;
    };

    std::vector<tc> test_cases = {{"", false, "", ""},
                                  {"Apath", false, "", ""},
                                  {" : ", false, "", ""},
                                  {"host:", false, "host", ""},
                                  {":port", false, "", "port"},
                                  {"host:port", true, "host", "port"},
                                  {"//host:port", true, "host", "port"},
                                  {"//host:port/", true, "host", "port"},
                                  {"//host:port/suite", true, "host", "port"},
                                  {"//host:port/suite/family/task", true, "host", "port"}};

    for (const auto& tc : test_cases) {
        std::string actual_host;
        std::string actual_port;
        auto actual = ecf::node::extract_host_and_port_from_path(tc.path, actual_host, actual_port);

        BOOST_REQUIRE_MESSAGE(actual == tc.expected,
                              "For path " << tc.path << ", expected " << tc.expected << " found " << actual);

        BOOST_REQUIRE_MESSAGE(actual_host == tc.expected_host,
                              "For path '" << tc.path << "', expected host '" << tc.expected_host << "' found '"
                                           << actual_host << "'");
        BOOST_REQUIRE_MESSAGE(actual_port == tc.expected_port,
                              "For path '" << tc.path << "', expected port '" << tc.expected_port << "' found '"
                                           << actual_port << "'");
    }
}

BOOST_AUTO_TEST_CASE(test_NodePath_perf, *boost::unit_test::disabled()) {
    ECF_NAME_THIS_TEST();

    // Timing using:
    //    StringSplitter : 6.35
    //    Str::split     : 8.64
    // See: Str::split -> define USE_STRINGSPLITTER

    // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
    ecf::ScopedPerformanceTimer timer;
    int n = 10000000;
    std::vector<std::string> thePath;
    thePath.reserve(20);
    for (int i = 0; i < n; i++) {
        thePath.clear();
        ecf::node::split_path("/this/is/a/test/string/that/will/be/used/to/check/perf/of/node/path/extraction",
                              thePath);
    }
    ECF_TEST_DBG(<< "Timing for " << n << " NodePath is  " << timer);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
