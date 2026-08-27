/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <chrono>

#include <boost/test/unit_test.hpp>

#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "ecflow/base/ConnectionDiagnosis.hpp"
#include "ecflow/base/ServerProtocolProbe.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// \brief Tests the connection diagnosis against a real ecFlow server
///
/// The unit tests in libs/base stand in a peer that misbehaves in a chosen way, which establishes
/// that each failure shape is diagnosed correctly. What they cannot establish is the positive
/// case: that a real ecFlow server is recognised as one. That is what these tests add, together
/// with the end-to-end behaviour of a mismatched client against a running server.
///
/// The server started here speaks plain TCP/IP, which is the only protocol InvokeServer launches.
/// The remaining rows of the client/server matrix require a server launched with --ssl or --http,
/// and certificates for the SSL rows.
///

using namespace ecf;

namespace {

const auto PROBE_TIMEOUT = std::chrono::seconds{2};

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_ConnectionDiagnosisAgainstServer)

BOOST_AUTO_TEST_CASE(test_a_successful_request_records_no_failure) {
    ECF_NAME_THIS_TEST();

    InvokeServer invokeServer("Client:: ...test_a_successful_request_records_no_failure:", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    BOOST_REQUIRE_MESSAGE(theClient.pingServer() == 0, "ping failed\n" << theClient.errorMsg());

    BOOST_CHECK(theClient.connection_diagnosis().ok());
    BOOST_CHECK(!theClient.connection_diagnosis().is_protocol_mismatch());
    BOOST_CHECK_EQUAL(theClient.effective_protocol(), Protocol::Plain);
    BOOST_CHECK(ecf::explain(theClient.connection_diagnosis()).empty());
}

BOOST_AUTO_TEST_CASE(test_the_probe_recognises_a_real_plain_server) {
    ECF_NAME_THIS_TEST();

    InvokeServer invokeServer("Client:: ...test_the_probe_recognises_a_real_plain_server:", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    BOOST_CHECK(ecf::probes_as_plain(invokeServer.host(), invokeServer.port(), PROBE_TIMEOUT));
    BOOST_CHECK(!ecf::probes_as_http("http", invokeServer.host(), invokeServer.port(), PROBE_TIMEOUT));
    BOOST_CHECK(!ecf::probes_as_tls(invokeServer.host(), invokeServer.port(), PROBE_TIMEOUT));

    auto found = ecf::probe_server_protocol(invokeServer.host(), invokeServer.port(), PROBE_TIMEOUT);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(found.value(), Protocol::Plain);
}

BOOST_AUTO_TEST_CASE(test_an_http_client_against_a_plain_server_is_diagnosed_as_a_mismatch) {
    ECF_NAME_THIS_TEST();

    InvokeServer invokeServer("Client:: ...test_an_http_client_against_a_plain_server:", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    theClient.set_throw_on_error(false);
    theClient.enable_http();

    BOOST_REQUIRE_MESSAGE(theClient.pingServer() == 1, "expected the mismatched request to fail");

    const auto& diagnosis = theClient.connection_diagnosis();

    BOOST_CHECK_EQUAL(theClient.effective_protocol(), Protocol::Http);
    BOOST_CHECK_EQUAL(diagnosis.client_protocol, Protocol::Http);
    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::ClosedWithoutReply);
    BOOST_CHECK(diagnosis.is_protocol_mismatch());

    // Before probing, the peer protocol is unknown, and the explanation must not claim one
    BOOST_CHECK(!diagnosis.peer_protocol.has_value());
    const auto before = ecf::explain(diagnosis);
    BOOST_CHECK_MESSAGE(before.find("Protocol mismatch") != std::string::npos, before);
    BOOST_CHECK_MESSAGE(before.find("appears to be using") == std::string::npos, before);

    // The reported error is the explanation, not the raw transport message, and it opens with the
    // same prefix as a failure reported over any other transport.
    const auto message = theClient.errorMsg();
    BOOST_CHECK_MESSAGE(message.find("Protocol mismatch") != std::string::npos, message);
    BOOST_CHECK_MESSAGE(message.rfind("Error: request( ", 0) == 0, message);
}

BOOST_AUTO_TEST_CASE(test_probing_after_a_mismatch_names_the_protocol_the_server_speaks) {
    ECF_NAME_THIS_TEST();

    InvokeServer invokeServer("Client:: ...test_probing_after_a_mismatch:", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(),
                          "Server failed to start on " << invokeServer.host() << ":" << invokeServer.port());

    ClientInvoker theClient(invokeServer.host(), invokeServer.port());
    theClient.set_throw_on_error(false);
    theClient.enable_http();

    BOOST_REQUIRE_MESSAGE(theClient.pingServer() == 1, "expected the mismatched request to fail");

    auto found = theClient.probe_protocol(PROBE_TIMEOUT);

    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(found.value(), Protocol::Plain);

    const auto& diagnosis = theClient.connection_diagnosis();
    BOOST_REQUIRE(diagnosis.peer_protocol.has_value());
    BOOST_CHECK_EQUAL(diagnosis.peer_protocol.value(), Protocol::Plain);
    BOOST_CHECK(diagnosis.is_protocol_mismatch());

    // The reply carries the same diagnosis, so that neither view of it goes stale
    BOOST_REQUIRE(theClient.server_reply().diagnosis().peer_protocol.has_value());
    BOOST_CHECK_EQUAL(theClient.server_reply().diagnosis().peer_protocol.value(), Protocol::Plain);

    // With the peer protocol determined, the explanation says what to change, and to what
    const auto after = ecf::explain(diagnosis);
    BOOST_CHECK_MESSAGE(after.find("Protocol mismatch") != std::string::npos, after);
    BOOST_CHECK_MESSAGE(after.find("appears to be using TCP/IP") != std::string::npos, after);
}

BOOST_AUTO_TEST_CASE(test_a_stopped_server_is_not_reported_as_a_mismatch) {
    ECF_NAME_THIS_TEST();

    // Nothing is listening on this port: the diagnosis must say so, rather than blame the
    // configuration. Telling the two apart is the reason the diagnosis exists.
    const auto port = SCPort::next();

    ClientInvoker theClient(ecf::string_constants::localhost, port);
    theClient.set_throw_on_error(false);
    theClient.set_connection_attempts(1);
    theClient.set_retry_connection_period(std::chrono::seconds{1});

    BOOST_REQUIRE_MESSAGE(theClient.pingServer() == 1, "expected the request to fail");

    const auto& diagnosis = theClient.connection_diagnosis();
    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::ConnectionRefused);
    BOOST_CHECK(!diagnosis.is_protocol_mismatch());

    // The reply must carry the diagnosis too: it is what reaches a caller that is handed the
    // reply rather than the invoker, such as the ecFlow UI.
    BOOST_CHECK_EQUAL(theClient.server_reply().diagnosis().failure, ConnectionFailure::ConnectionRefused);

    const auto message = theClient.errorMsg();
    BOOST_CHECK_MESSAGE(message.find("Error: request( ") != std::string::npos, message);
    BOOST_CHECK_MESSAGE(message.find("nothing is listening") != std::string::npos, message);
    BOOST_CHECK_MESSAGE(message.find("Protocol mismatch") == std::string::npos, message);
    // The REST front end maps this wording onto a Bad Gateway status (see ApiV1.cpp)
    BOOST_CHECK_MESSAGE(message.find("Failed to connect to ") != std::string::npos, message);

    BOOST_CHECK(!ecf::probe_server_protocol(ecf::string_constants::localhost, port, PROBE_TIMEOUT).has_value());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
