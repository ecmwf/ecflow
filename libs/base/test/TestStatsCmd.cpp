/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "MyDefsFixture.hpp"
#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
///   Tests for `Stats` command
///    - check that the 'number of suits' is correctly reported
///    - check that the 'requests per second' is correctly reported
///

namespace {

std::string extract_report_value(const std::string& report, const std::string& label) {
    boost::char_separator<char> sep("\n"); // splitting report by lines
    using tokenizer_t = boost::tokenizer<boost::char_separator<char>>;
    tokenizer_t tokens(report, sep);
    for (const auto& token : tokens) {
        if (token.find(label) != std::string::npos) {
            // Important!
            //   By convention, the labels in report by `Stats` have width 35
            //   If this convention changes, the test will fail
            return token.substr(35);
        }
    }
    throw std::runtime_error("Unable to find requested label");
}

int extract_number_of_suites(const std::string& report) {
    auto entry = extract_report_value(report, "Number of Suites");
    return ecf::convert_to<int>(entry);
}

std::string extract_request_per_second(const std::string& report) {
    auto entry = extract_report_value(report, "Request/s per 1,5,15,30,60 min");
    return entry;
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_StatsCmd)

BOOST_AUTO_TEST_CASE(test_stats_cmd__reports_number_of_suites) {

    ecf::TestLog test_log("test_stats_cmd__reports_number_of_suites.log");

    Defs defs;
    // Considering 3 suites, to be reported by `Stats`
    defs.addSuite(Suite::create("some_suite"));
    defs.addSuite(Suite::create("another_suite"));
    defs.addSuite(Suite::create("yet_another_suite"));

    MockServer server(&defs);

    // Execute `Stats` command
    {
        auto command = std::make_shared<CtsCmd>(CtsCmd::STATS);

        ClientToServerRequest request;
        request.set_cmd(command);

        try {
            auto reply = request.handleRequest(&server);
            if (reply) {
                BOOST_REQUIRE(reply->ok());

                auto no_of_suites = extract_number_of_suites(reply->get_string());
                BOOST_REQUIRE(no_of_suites == 3);
            }
        }
        catch (std::exception& e) {
            BOOST_CHECK_MESSAGE(false, "Unexpected exception : " << e.what() << " : " << request);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_stats_cmd__reports_requests_per_second) {

    ecf::TestLog test_log("test_stats_cmd__reports_requests_per_second.log");

    Defs defs;

    MockServer server(&defs);

    // Simulate some fictitious server load to report statistics
    int polling = 60; // every 60 seconds
    int samples = 60; // to cover 1 hour 'uptime'
    for (int i = 0; i < samples; ++i) {
        int sample = 30 + (samples - i - 1);
        // Note: the request counter sample varies to allow checking that the
        // measurements are processed in the correct order
        server.stats().request_count_ = sample;
        server.stats().update_stats(polling);
    }

    // Execute `Stats` command
    {
        auto command = std::make_shared<CtsCmd>(CtsCmd::STATS);

        ClientToServerRequest request;
        request.set_cmd(command);

        try {
            auto reply = request.handleRequest(&server);
            if (reply) {
                BOOST_REQUIRE(reply->ok());

                auto requests_per_second = extract_request_per_second(reply->get_string());
                BOOST_REQUIRE(requests_per_second == "0.50 0.53 0.62 0.74 0.99");
            }
        }
        catch (std::exception& e) {
            BOOST_CHECK_MESSAGE(false, "Unexpected exception : " << e.what() << " : " << request);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
