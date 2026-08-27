/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/asio/error.hpp>
#include <boost/test/unit_test.hpp>

#ifdef ECF_OPENSSL
    #include <boost/asio/ssl/error.hpp>
    #include <openssl/err.h>
    #include <openssl/ssl.h>
#endif

#include "ecflow/base/ConnectionDiagnosis.hpp"
#include "ecflow/base/ConnectionFailureMapping.hpp"
#include "ecflow/base/HttpClient.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/base/stc/ErrorCmd.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// \brief Tests the structured diagnosis of a failed client/server exchange
///
/// The tests below cover the pure parts of the mechanism: the value semantics of
/// ecf::ConnectionDiagnosis, the mapping of transport errors onto ecf::ConnectionFailure, and the
/// wording produced by ecf::explain(). The behaviour of the transports themselves is covered by
/// TestConnectionDiagnosisAgainstPeers.cpp.
///

using ecf::ConnectionDiagnosis;
using ecf::ConnectionFailure;
using ecf::Protocol;

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_ConnectionDiagnosis)

// Value semantics

BOOST_AUTO_TEST_CASE(test_default_diagnosis_records_no_failure) {
    ECF_NAME_THIS_TEST();

    ConnectionDiagnosis diagnosis;

    BOOST_CHECK(diagnosis.ok());
    BOOST_CHECK(!diagnosis.is_protocol_mismatch());
    BOOST_CHECK(!diagnosis.peer_protocol.has_value());
    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::None);
    BOOST_CHECK(diagnosis.host.empty());
    BOOST_CHECK(diagnosis.port.empty());
    BOOST_CHECK(diagnosis.detail.empty());
}

BOOST_AUTO_TEST_CASE(test_clear_restores_the_default_state) {
    ECF_NAME_THIS_TEST();

    ConnectionDiagnosis diagnosis;
    diagnosis.client_protocol             = Protocol::Https;
    diagnosis.failure                     = ConnectionFailure::HandshakeFailed;
    diagnosis.peer_protocol               = Protocol::Plain;
    diagnosis.protocol_mismatch_suspected = true;
    diagnosis.host                        = "somewhere";
    diagnosis.port                        = "3141";
    diagnosis.detail                      = "something went wrong";

    diagnosis.clear();

    BOOST_CHECK(diagnosis.ok());
    BOOST_CHECK(!diagnosis.peer_protocol.has_value());
    BOOST_CHECK(!diagnosis.protocol_mismatch_suspected);
    BOOST_CHECK_EQUAL(diagnosis.client_protocol, Protocol::Plain);
    BOOST_CHECK(diagnosis.host.empty());
    BOOST_CHECK(diagnosis.port.empty());
    BOOST_CHECK(diagnosis.detail.empty());
}

// The distinction between a suspected and an established mismatch

BOOST_AUTO_TEST_CASE(test_mismatch_is_suspected_when_the_peer_protocol_is_unknown) {
    ECF_NAME_THIS_TEST();

    ConnectionDiagnosis diagnosis;
    diagnosis.failure                     = ConnectionFailure::ClosedWithoutReply;
    diagnosis.protocol_mismatch_suspected = true;

    BOOST_CHECK(diagnosis.is_protocol_mismatch());
}

BOOST_AUTO_TEST_CASE(test_a_determined_peer_protocol_settles_the_question) {
    ECF_NAME_THIS_TEST();

    // The peer was determined to speak what the client speaks: whatever the transport suspected,
    // the failure is not a protocol mismatch.
    ConnectionDiagnosis agreeing;
    agreeing.client_protocol             = Protocol::Plain;
    agreeing.peer_protocol               = Protocol::Plain;
    agreeing.protocol_mismatch_suspected = true;
    agreeing.failure                     = ConnectionFailure::ClosedWithoutReply;

    BOOST_CHECK(!agreeing.is_protocol_mismatch());

    // The peer was determined to speak something else: it is a mismatch, even though nothing was
    // suspected at the time of the failure.
    ConnectionDiagnosis differing;
    differing.client_protocol             = Protocol::Plain;
    differing.peer_protocol               = Protocol::Ssl;
    differing.protocol_mismatch_suspected = false;
    differing.failure                     = ConnectionFailure::ClosedWithoutReply;

    BOOST_CHECK(differing.is_protocol_mismatch());
}

// Error classification: connect

BOOST_AUTO_TEST_CASE(test_connect_errors_are_classified) {
    ECF_NAME_THIS_TEST();

    using namespace boost::asio;

    BOOST_CHECK_EQUAL(ecf::classify_connect_error(error::host_not_found), ConnectionFailure::HostResolution);
    BOOST_CHECK_EQUAL(ecf::classify_connect_error(error::host_not_found_try_again), ConnectionFailure::HostResolution);
    BOOST_CHECK_EQUAL(ecf::classify_connect_error(error::service_not_found), ConnectionFailure::HostResolution);

    BOOST_CHECK_EQUAL(ecf::classify_connect_error(error::connection_refused), ConnectionFailure::ConnectionRefused);
    BOOST_CHECK_EQUAL(ecf::classify_connect_error(error::host_unreachable), ConnectionFailure::ConnectionRefused);
    BOOST_CHECK_EQUAL(ecf::classify_connect_error(error::network_unreachable), ConnectionFailure::ConnectionRefused);

    BOOST_CHECK_EQUAL(ecf::classify_connect_error(error::timed_out), ConnectionFailure::Timeout);

    BOOST_CHECK_EQUAL(ecf::classify_connect_error(error::access_denied), ConnectionFailure::Other);

    // A success code is never a failure
    BOOST_CHECK_EQUAL(ecf::classify_connect_error(boost::system::error_code{}), ConnectionFailure::None);
}

// Error classification: read

BOOST_AUTO_TEST_CASE(test_read_errors_are_classified) {
    ECF_NAME_THIS_TEST();

    using namespace boost::asio;

    // The whole point of the exercise: a peer that accepts and then closes is reported as such,
    // whichever of the three shapes the close takes.
    BOOST_CHECK_EQUAL(ecf::classify_read_error(error::eof), ConnectionFailure::ClosedWithoutReply);
    BOOST_CHECK_EQUAL(ecf::classify_read_error(error::connection_reset), ConnectionFailure::ClosedWithoutReply);
    BOOST_CHECK_EQUAL(ecf::classify_read_error(error::broken_pipe), ConnectionFailure::ClosedWithoutReply);
    BOOST_CHECK_EQUAL(ecf::classify_read_error(error::connection_aborted), ConnectionFailure::ClosedWithoutReply);

    BOOST_CHECK_EQUAL(ecf::classify_read_error(error::invalid_argument), ConnectionFailure::UndecodableReply);
    BOOST_CHECK_EQUAL(ecf::classify_read_error(error::message_size), ConnectionFailure::UndecodableReply);

    BOOST_CHECK_EQUAL(ecf::classify_read_error(error::timed_out), ConnectionFailure::Timeout);
    BOOST_CHECK_EQUAL(ecf::classify_read_error(error::operation_aborted), ConnectionFailure::Timeout);

    BOOST_CHECK_EQUAL(ecf::classify_read_error(error::no_buffer_space), ConnectionFailure::Other);

    BOOST_CHECK_EQUAL(ecf::classify_read_error(boost::system::error_code{}), ConnectionFailure::None);
}

BOOST_AUTO_TEST_CASE(test_only_failures_after_a_successful_connect_suggest_a_mismatch) {
    ECF_NAME_THIS_TEST();

    // A peer that is listening, and then fails to speak the expected protocol
    BOOST_CHECK(ecf::suggests_protocol_mismatch(ConnectionFailure::ClosedWithoutReply));
    BOOST_CHECK(ecf::suggests_protocol_mismatch(ConnectionFailure::UndecodableReply));
    BOOST_CHECK(ecf::suggests_protocol_mismatch(ConnectionFailure::HandshakeFailed));

    // Nothing is listening, or the peer is listening and speaking correctly
    BOOST_CHECK(!ecf::suggests_protocol_mismatch(ConnectionFailure::None));
    BOOST_CHECK(!ecf::suggests_protocol_mismatch(ConnectionFailure::HostResolution));
    BOOST_CHECK(!ecf::suggests_protocol_mismatch(ConnectionFailure::ConnectionRefused));
    BOOST_CHECK(!ecf::suggests_protocol_mismatch(ConnectionFailure::Timeout));
    BOOST_CHECK(!ecf::suggests_protocol_mismatch(ConnectionFailure::RejectedRequest));
    BOOST_CHECK(!ecf::suggests_protocol_mismatch(ConnectionFailure::Other));

    // A rejected certificate proves the peer *does* speak TLS, so it is never a mismatch
    BOOST_CHECK(!ecf::suggests_protocol_mismatch(ConnectionFailure::CertificateRejected));
}

// Error classification: HTTP

BOOST_AUTO_TEST_CASE(test_httplib_errors_are_classified) {
    ECF_NAME_THIS_TEST();

    using ecf::http::classify_httplib_error;

    BOOST_CHECK_EQUAL(classify_httplib_error(httplib::Error::Success), ConnectionFailure::None);

    BOOST_CHECK_EQUAL(classify_httplib_error(httplib::Error::Connection), ConnectionFailure::ConnectionRefused);
    BOOST_CHECK_EQUAL(classify_httplib_error(httplib::Error::BindIPAddress), ConnectionFailure::ConnectionRefused);

    BOOST_CHECK_EQUAL(classify_httplib_error(httplib::Error::ConnectionTimeout), ConnectionFailure::Timeout);
    BOOST_CHECK_EQUAL(classify_httplib_error(httplib::Error::Canceled), ConnectionFailure::Timeout);

    // Connected, then failed while exchanging data: the signature of a peer that is listening but
    // does not speak HTTP.
    BOOST_CHECK_EQUAL(classify_httplib_error(httplib::Error::Read), ConnectionFailure::ClosedWithoutReply);
    BOOST_CHECK_EQUAL(classify_httplib_error(httplib::Error::Write), ConnectionFailure::ClosedWithoutReply);

    BOOST_CHECK_EQUAL(classify_httplib_error(httplib::Error::SSLConnection), ConnectionFailure::HandshakeFailed);
    BOOST_CHECK_EQUAL(classify_httplib_error(httplib::Error::SSLLoadingCerts), ConnectionFailure::HandshakeFailed);
    BOOST_CHECK_EQUAL(classify_httplib_error(httplib::Error::SSLServerVerification),
                      ConnectionFailure::CertificateRejected);

    BOOST_CHECK_EQUAL(classify_httplib_error(httplib::Error::Unknown), ConnectionFailure::Other);
    BOOST_CHECK_EQUAL(classify_httplib_error(httplib::Error::Compression), ConnectionFailure::Other);
    BOOST_CHECK_EQUAL(classify_httplib_error(httplib::Error::ExceedRedirectCount), ConnectionFailure::Other);
}

// Error classification: TLS handshake

#ifdef ECF_OPENSSL

BOOST_AUTO_TEST_CASE(test_a_rejected_certificate_is_told_apart_from_a_peer_that_does_not_speak_tls) {
    ECF_NAME_THIS_TEST();

    // A peer that closes the connection, or answers in plaintext, during the handshake
    BOOST_CHECK_EQUAL(ecf::classify_handshake_error(boost::asio::error::eof), ConnectionFailure::HandshakeFailed);
    BOOST_CHECK_EQUAL(ecf::classify_handshake_error(boost::asio::error::connection_reset),
                      ConnectionFailure::HandshakeFailed);

    // A peer that does speak TLS, but presents a certificate this client does not trust. Misreading
    // this as a protocol mismatch is precisely the defect this separation exists to prevent.
    boost::system::error_code verification_failed(ERR_PACK(ERR_LIB_SSL, 0, SSL_R_CERTIFICATE_VERIFY_FAILED),
                                                  boost::asio::error::get_ssl_category());
    BOOST_CHECK_EQUAL(ecf::classify_handshake_error(verification_failed), ConnectionFailure::CertificateRejected);

    boost::system::error_code unknown_ca(ERR_PACK(ERR_LIB_SSL, 0, SSL_R_TLSV1_ALERT_UNKNOWN_CA),
                                         boost::asio::error::get_ssl_category());
    BOOST_CHECK_EQUAL(ecf::classify_handshake_error(unknown_ca), ConnectionFailure::CertificateRejected);

    // An SSL-category error that is not about the certificate, such as a plaintext peer
    boost::system::error_code wrong_version(ERR_PACK(ERR_LIB_SSL, 0, SSL_R_WRONG_VERSION_NUMBER),
                                            boost::asio::error::get_ssl_category());
    BOOST_CHECK_EQUAL(ecf::classify_handshake_error(wrong_version), ConnectionFailure::HandshakeFailed);

    BOOST_CHECK_EQUAL(ecf::classify_handshake_error(boost::system::error_code{}), ConnectionFailure::None);
}

#endif

// The explanation

BOOST_AUTO_TEST_CASE(test_a_successful_exchange_has_nothing_to_explain) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK(ecf::explain(ConnectionDiagnosis{}).empty());
}

BOOST_AUTO_TEST_CASE(test_the_explanation_names_both_protocols_and_the_endpoint) {
    ECF_NAME_THIS_TEST();

    ConnectionDiagnosis diagnosis;
    diagnosis.client_protocol = Protocol::Plain;
    diagnosis.peer_protocol   = Protocol::Ssl;
    diagnosis.failure         = ConnectionFailure::ClosedWithoutReply;
    diagnosis.host            = "hostname";
    diagnosis.port            = "3141";

    const std::string explanation = ecf::explain(diagnosis);

    BOOST_CHECK_MESSAGE(explanation.find("Protocol mismatch") != std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find("TCP/IP") != std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find("TCP/IP with SSL") != std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find("hostname:3141") != std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find("closed it without a reply") != std::string::npos ||
                            explanation.find("closed without a reply") != std::string::npos,
                        explanation);
}

BOOST_AUTO_TEST_CASE(test_the_explanation_does_not_claim_a_peer_protocol_it_does_not_know) {
    ECF_NAME_THIS_TEST();

    ConnectionDiagnosis diagnosis;
    diagnosis.client_protocol             = Protocol::Http;
    diagnosis.failure                     = ConnectionFailure::ClosedWithoutReply;
    diagnosis.protocol_mismatch_suspected = true;
    diagnosis.host                        = "hostname";
    diagnosis.port                        = "3141";

    const std::string explanation = ecf::explain(diagnosis);

    BOOST_CHECK_MESSAGE(explanation.find("Protocol mismatch") != std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find("appears to be using") == std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find("same protocol") != std::string::npos, explanation);
}

BOOST_AUTO_TEST_CASE(test_a_failure_that_is_not_a_mismatch_is_not_reported_as_one) {
    ECF_NAME_THIS_TEST();

    ConnectionDiagnosis diagnosis;
    diagnosis.client_protocol = Protocol::Plain;
    diagnosis.failure         = ConnectionFailure::ConnectionRefused;
    diagnosis.host            = "hostname";
    diagnosis.port            = "3141";

    const std::string explanation = ecf::explain(diagnosis);

    BOOST_CHECK_MESSAGE(explanation.find("Protocol mismatch") == std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find("Cannot communicate with the server") != std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find("nothing is listening") != std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find("server is running") != std::string::npos, explanation);
}

BOOST_AUTO_TEST_CASE(test_a_rejected_certificate_advises_about_certificates_not_about_protocols) {
    ECF_NAME_THIS_TEST();

    ConnectionDiagnosis diagnosis;
    diagnosis.client_protocol = Protocol::Ssl;
    diagnosis.peer_protocol   = Protocol::Ssl;
    diagnosis.failure         = ConnectionFailure::CertificateRejected;
    diagnosis.host            = "hostname";
    diagnosis.port            = "3141";

    const std::string explanation = ecf::explain(diagnosis);

    BOOST_CHECK_MESSAGE(explanation.find("Protocol mismatch") == std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find("certificate") != std::string::npos, explanation);
}

BOOST_AUTO_TEST_CASE(test_a_refused_request_is_not_reported_as_a_failure_to_communicate) {
    ECF_NAME_THIS_TEST();

    // The server answered; it simply declined the request. Saying that communication failed would
    // send a user looking at the network instead of at the request.
    ConnectionDiagnosis diagnosis;
    diagnosis.client_protocol = Protocol::Http;
    diagnosis.peer_protocol   = Protocol::Http;
    diagnosis.failure         = ConnectionFailure::RejectedRequest;
    diagnosis.host            = "hostname";
    diagnosis.port            = "3141";

    const std::string explanation = ecf::explain(diagnosis);

    BOOST_CHECK_MESSAGE(explanation.find("Request refused") != std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find("Protocol mismatch") == std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find("Cannot communicate") == std::string::npos, explanation);
}

BOOST_AUTO_TEST_CASE(test_the_low_level_detail_is_carried_into_the_explanation) {
    ECF_NAME_THIS_TEST();

    ConnectionDiagnosis diagnosis;
    diagnosis.failure = ConnectionFailure::Other;
    diagnosis.detail  = "a very specific transport message";

    BOOST_CHECK(ecf::explain(diagnosis).find("a very specific transport message") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_every_failure_class_produces_an_explanation) {
    ECF_NAME_THIS_TEST();

    for (auto failure : ecf::Enumerate<ConnectionFailure>::enums()) {
        ConnectionDiagnosis diagnosis;
        diagnosis.failure = failure;
        diagnosis.host    = "hostname";
        diagnosis.port    = "3141";

        const std::string explanation = ecf::explain(diagnosis);
        if (failure == ConnectionFailure::None) {
            BOOST_CHECK(explanation.empty());
        }
        else {
            BOOST_CHECK_MESSAGE(!explanation.empty(),
                                "no explanation for " << ecf::Enumerate<ConnectionFailure>::as_string(failure));
        }
    }
}

// The shape of a reported failure

BOOST_AUTO_TEST_CASE(test_every_failure_report_opens_with_the_same_prefix) {
    ECF_NAME_THIS_TEST();

    CtsCmd request(CtsCmd::STATS);

    BOOST_CHECK_EQUAL(failed_request_prefix(request), "Error: request( --stats ) failed! ");
}

BOOST_AUTO_TEST_CASE(test_a_client_side_error_reply_is_explained_not_quoted_as_a_server_reply) {
    ECF_NAME_THIS_TEST();

    // The SSL transport reports a failed handshake by manufacturing an error reply, rather than by
    // throwing. Without consulting the diagnosis, that reply quoted the raw handshake message, so
    // the same protocol mismatch read differently over SSL than over every other transport.
    Cmd_ptr request = std::make_shared<CtsCmd>(CtsCmd::STATS);

    ServerReply reply;
    ConnectionDiagnosis diagnosis;
    diagnosis.client_protocol             = Protocol::Ssl;
    diagnosis.failure                     = ConnectionFailure::HandshakeFailed;
    diagnosis.protocol_mismatch_suspected = true;
    diagnosis.host                        = "hostname";
    diagnosis.port                        = "3141";
    diagnosis.detail                      = "wrong version number (SSL routines)";
    reply.set_diagnosis(diagnosis);

    ErrorCmd error("SslClient::handle_handshake: error(wrong version number (SSL routines)) on "
                   "hostname:3141 possibly non-ssl server");
    BOOST_CHECK(!error.handle_server_response(reply, request, false));

    const std::string& message = reply.error_msg();

    BOOST_CHECK_MESSAGE(message.rfind("Error: request( --stats ) failed! ", 0) == 0, message);
    BOOST_CHECK_MESSAGE(message.find("Protocol mismatch") != std::string::npos, message);
    BOOST_CHECK_MESSAGE(message.find("TCP/IP with SSL") != std::string::npos, message);
    // Nothing was received from the server, so the report must not claim otherwise
    BOOST_CHECK_MESSAGE(message.find("Server reply:") == std::string::npos, message);
    BOOST_CHECK_MESSAGE(message.find("possibly non-ssl server") == std::string::npos, message);
}

BOOST_AUTO_TEST_CASE(test_a_genuine_server_error_is_still_reported_as_a_server_reply) {
    ECF_NAME_THIS_TEST();

    // The exchange succeeded; the server itself refused the request. Its message is what the user
    // needs to see, and the diagnosis records no failure.
    Cmd_ptr request = std::make_shared<CtsCmd>(CtsCmd::STATS);

    ServerReply reply;
    BOOST_REQUIRE(reply.diagnosis().ok());

    ErrorCmd error("Authorisation failure: the user is not allowed to run this command");
    BOOST_CHECK(!error.handle_server_response(reply, request, false));

    const std::string& message = reply.error_msg();

    BOOST_CHECK_MESSAGE(message.rfind("Error: request( --stats ) failed! ", 0) == 0, message);
    BOOST_CHECK_MESSAGE(message.find("Server reply: Authorisation failure") != std::string::npos, message);
    BOOST_CHECK_MESSAGE(message.find("Protocol mismatch") == std::string::npos, message);
}

// Integration with ServerReply

BOOST_AUTO_TEST_CASE(test_the_server_reply_carries_and_clears_the_diagnosis) {
    ECF_NAME_THIS_TEST();

    ServerReply reply;
    BOOST_CHECK(reply.diagnosis().ok());

    ConnectionDiagnosis diagnosis;
    diagnosis.client_protocol = Protocol::Https;
    diagnosis.failure         = ConnectionFailure::HandshakeFailed;
    diagnosis.host            = "hostname";
    diagnosis.port            = "3141";
    reply.set_diagnosis(diagnosis);

    BOOST_CHECK(!reply.diagnosis().ok());
    BOOST_CHECK_EQUAL(reply.diagnosis().failure, ConnectionFailure::HandshakeFailed);
    BOOST_CHECK_EQUAL(reply.diagnosis().client_protocol, Protocol::Https);

    // Every invocation starts from a clean diagnosis, so that a failure is never attributed to a
    // later, unrelated request.
    reply.clear_for_invoke(false);
    BOOST_CHECK(reply.diagnosis().ok());
    BOOST_CHECK(reply.diagnosis().host.empty());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
