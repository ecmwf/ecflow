// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsBenchmark.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace bm = ecf::test::benchmark;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsBenchmarkHelpers)

///
/// @brief Verifies that parameters are positive integers, given in full, or their defaults when unset.
///
BOOST_AUTO_TEST_CASE(test_benchmark_parameters) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_EQUAL(bm::parse_positive("p", "20"), 20);
    for (const std::string invalid : {"", "0", "-1", "+1", " 5", "5 ", "20oops", "1.5", "9999999999"}) {
        BOOST_CHECK_THROW(bm::parse_positive("p", invalid), std::invalid_argument);
    }

    ecf::test::scaffold::WithoutTestEnvironmentVariable unset("ECF_TEST_PERMISSIONS_UNSET");
    BOOST_CHECK_EQUAL(bm::parameter("ECF_TEST_PERMISSIONS_UNSET", 7), 7);
    ecf::test::scaffold::WithTestEnvironmentVariable invalid("ECF_TEST_PERMISSIONS_INVALID", "0");
    BOOST_CHECK_THROW(bm::parameter("ECF_TEST_PERMISSIONS_INVALID", 7), std::invalid_argument);
}

///
/// @brief Verifies the summary of a sample, and that an empty sample is refused.
///
BOOST_AUTO_TEST_CASE(test_benchmark_summaries) {
    ECF_NAME_THIS_TEST();

    const auto odd = bm::summarise({3, 1, 2});
    BOOST_CHECK_EQUAL(odd.median, 2);
    BOOST_CHECK_EQUAL(odd.max, 3);
    const auto even = bm::summarise({4, 1, 3, 2});
    BOOST_CHECK_EQUAL(even.median, 3);
    BOOST_CHECK_EQUAL(even.max, 4);
    BOOST_CHECK_THROW(bm::summarise({}), std::invalid_argument);
}

///
/// @brief Verifies the parsing of the measurements printed by ps, and that malformed output is refused.
///
BOOST_AUTO_TEST_CASE(test_benchmark_process_measurements) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_CLOSE(bm::parse_cpu_time("0:00.00"), 0.0, 1e-9);
    BOOST_CHECK_CLOSE(bm::parse_cpu_time("00:01.50"), 1.5, 1e-9);
    BOOST_CHECK_CLOSE(bm::parse_cpu_time("01:02:03"), 3723.0, 1e-9);
    BOOST_CHECK_CLOSE(bm::parse_cpu_time("1-00:00:01"), 86401.0, 1e-9);
    for (const std::string invalid : {"",
                                      "abc",
                                      "12",
                                      "1:2:3:4",
                                      "1:x",
                                      "-1:00",
                                      "1:-2",
                                      "1:02:",
                                      ":02",
                                      "nan:00",
                                      "inf:00",
                                      "+1:00",
                                      "1.5:00",
                                      "1:60",
                                      "60:00:00.5x",
                                      "1-24:00:00",
                                      "0-24:00:00",
                                      "1-00:00"}) {
        BOOST_CHECK_THROW(bm::parse_cpu_time(invalid), std::invalid_argument);
    }

    BOOST_CHECK_CLOSE(bm::parse_memory_kib("2048"), 2.0, 1e-9);
    BOOST_CHECK_THROW(bm::parse_memory_kib(""), std::invalid_argument);

    BOOST_CHECK_EQUAL(bm::run("echo ' text '"), "text");
    BOOST_CHECK_EQUAL(bm::run("true"), "");
    BOOST_CHECK_THROW(bm::run("false"), std::runtime_error);
}

///
/// @brief Verifies that a report is written in full, and that a failure to write it is reported.
///
BOOST_AUTO_TEST_CASE(test_benchmark_report) {
    ECF_NAME_THIS_TEST();

    auto directory  = ecf::test::scaffold::MakeDirectory{}.create();
    const auto file = directory.path() / "report.json";
    bm::write_report(file, "{}\n");
    std::ifstream in(file);
    BOOST_CHECK_EQUAL(std::string(std::istreambuf_iterator<char>(in), {}), "{}\n");

    BOOST_CHECK_EXCEPTION(
        bm::write_report(directory.path() / "missing" / "report.json", "{}\n"),
        std::runtime_error,
        [](const std::runtime_error& e) { return std::string(e.what()).find("missing") != std::string::npos; });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
