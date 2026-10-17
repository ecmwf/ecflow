// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "MyDefsFixture.hpp"
#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/ServerProtocol.hpp"
#include "ecflow/base/Stats.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using ecf::test::scaffold::TestLog;

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
            //   By convention, the labels in report by `Stats` have a specific width
            //   If this convention changes, the test will fail
            return token.substr(Stats::width + 2 /* amount of spaces before label */);
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

std::string extract_protocol(const std::string& report) {
    return extract_report_value(report, "Protocol");
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_StatsCmd)

BOOST_AUTO_TEST_CASE(is_able_to_report_number_of_suites) {

    TestLog test_log("test_stats_cmd__reports_number_of_suites.log");

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

BOOST_AUTO_TEST_CASE(is_able_to_report_requests_per_second) {

    TestLog test_log("test_stats_cmd__reports_requests_per_second.log");

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

BOOST_AUTO_TEST_CASE(is_able_to_report_protocol) {

    TestLog test_log("test_stats_cmd__reports_protocol.log");

    Defs defs;

    MockServer server(&defs);
    // SSL is chosen because its encoding (SSL) and its designation (TCP/IP with SSL) differ, so the
    // assertion below distinguishes the stored form from the displayed one.
    server.stats().protocol_ = ecf::Enumerate<ecf::Protocol>::to_string(ecf::Protocol::Ssl).value();

    // Execute `Stats` command
    {
        auto command = std::make_shared<CtsCmd>(CtsCmd::STATS);

        ClientToServerRequest request;
        request.set_cmd(command);

        try {
            auto reply = request.handleRequest(&server);
            if (reply) {
                BOOST_REQUIRE(reply->ok());

                auto protocol = extract_protocol(reply->get_string());
                BOOST_REQUIRE_MESSAGE(protocol == ecf::to_ui_designation(ecf::Protocol::Ssl),
                                      "expected the report to show the designation, but found " << protocol);
            }
        }
        catch (std::exception& e) {
            BOOST_CHECK_MESSAGE(false, "Unexpected exception : " << e.what() << " : " << request);
        }
    }
}

BOOST_AUTO_TEST_CASE(is_able_to_preserve_protocol_when_serialised) {

    // The `protocol_` member must survive a serialise/deserialise round trip,
    // since `Stats` is sent from server to client over the wire using this mechanism.

    Stats saved;
    saved.protocol_ = ecf::Enumerate<ecf::Protocol>::to_string(ecf::Protocol::Ssl).value();

    std::string archive_data;
    ecf::save_as_string(archive_data, saved);

    Stats restored;
    ecf::restore_from_string(archive_data, restored);

    BOOST_REQUIRE_MESSAGE(restored.protocol_ == saved.protocol_,
                          "protocol_ was not preserved by (de)serialisation: expected '"
                              << saved.protocol_ << "', got '" << restored.protocol_ << "'");
}

BOOST_AUTO_TEST_CASE(is_able_to_report_ssl_request_and_selected_certificate) {
    ECF_NAME_THIS_TEST();

    // Returns the values of every report line labelled ECF_SSL, in order
    auto ssl_lines = [](const Stats& stats) {
        std::ostringstream os;
        stats.show(os);
        std::vector<std::string> values;
        std::istringstream lines(os.str());
        for (std::string line; std::getline(lines, line);) {
            if (line.rfind("  ECF_SSL ", 0) == 0) {
                values.push_back(line.substr(Stats::width + 2 /* amount of spaces before label */));
            }
        }
        return values;
    };

    Stats stats;
    BOOST_REQUIRE_EQUAL(ssl_lines(stats).size(), 1U);
    BOOST_CHECK_EQUAL(ssl_lines(stats)[0], "disabled");

    stats.ECF_SSL_             = "X (server-specific certificate first, shared certificate as fallback)";
    stats.ECF_SSL_CERTIFICATE_ = "found shared certificate: server.crt";
    const auto lines           = ssl_lines(stats);
    BOOST_REQUIRE_EQUAL(lines.size(), 2U);
    BOOST_CHECK_EQUAL(lines[0], stats.ECF_SSL_);
    BOOST_CHECK_EQUAL(lines[1], stats.ECF_SSL_CERTIFICATE_);
}

BOOST_AUTO_TEST_CASE(is_able_to_preserve_selected_certificate_when_serialised) {
    ECF_NAME_THIS_TEST();

    Stats saved;
    saved.ECF_SSL_             = "1 (shared certificate first, server-specific certificate as fallback)";
    saved.ECF_SSL_CERTIFICATE_ = "found server-specific certificate: somehost.3141.crt";

    std::string archive_data;
    ecf::save_as_string(archive_data, saved);

    Stats restored;
    ecf::restore_from_string(archive_data, restored);

    BOOST_CHECK_EQUAL(restored.ECF_SSL_, saved.ECF_SSL_);
    BOOST_CHECK_EQUAL(restored.ECF_SSL_CERTIFICATE_, saved.ECF_SSL_CERTIFICATE_);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
