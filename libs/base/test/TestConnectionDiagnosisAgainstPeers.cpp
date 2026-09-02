/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/base/Client.hpp"
#include "ecflow/base/ConnectionDiagnosis.hpp"
#include "ecflow/base/HttpClient.hpp"
#include "ecflow/base/ServerProtocolProbe.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// \brief Tests that each client transport diagnoses a failure from what the peer actually did
///
/// A protocol mismatch is, from the point of view of a client, a peer that accepts the connection
/// and then behaves in a way the transport does not expect. These tests stand in a peer that
/// behaves in each of those ways, and check that the resulting diagnosis names what happened.
///
/// Standing in a peer, rather than launching a real ecFlow server in each of its four
/// configurations, keeps the tests deterministic, fast, and free of any need for certificates.
/// The full client/server matrix belongs to the integration tests.
///

using ecf::ConnectionDiagnosis;
using ecf::ConnectionFailure;
using ecf::Protocol;

namespace {

const auto SHORT_TIMEOUT = std::chrono::milliseconds{1500};

///
/// @brief A TCP peer that accepts connections and then misbehaves in a chosen, specific way.
///
/// The peer binds to an ephemeral port on the loopback interface, so that concurrent tests never
/// contend for a fixed port. It serves connections until it is destroyed.
///
class FakePeer {
public:
    ///
    /// @brief What the peer does with each connection it accepts.
    ///
    enum class Behaviour {
        CloseImmediately, ///< Accept, then close without reading or replying
        SendGarbage,      ///< Accept, then send bytes that are not a valid ecFlow frame
        StaySilent,       ///< Accept, then hold the connection open without ever replying
        AnswerHttp        ///< Accept, then answer with a well-formed HTTP error response
    };

    explicit FakePeer(Behaviour behaviour)
        : behaviour_(behaviour),
          acceptor_(io_, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 0)) {
        port_ = ecf::convert_to<std::string>(acceptor_.local_endpoint().port());
        accept_next();
        worker_ = std::thread([this]() { io_.run(); });
    }

    FakePeer(const FakePeer&)            = delete;
    FakePeer& operator=(const FakePeer&) = delete;
    FakePeer(FakePeer&&)                 = delete;
    FakePeer& operator=(FakePeer&&)      = delete;

    ~FakePeer() {
        boost::system::error_code ignored;
        acceptor_.close(ignored);
        io_.stop();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    static std::string host() { return "127.0.0.1"; }
    const std::string& port() const { return port_; }

private:
    void accept_next() {
        auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_);
        acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& error) {
            if (error) {
                return; // The acceptor was closed: the peer is going away
            }
            serve(socket);
            accept_next();
        });
    }

    void serve(const std::shared_ptr<boost::asio::ip::tcp::socket>& socket) {
        switch (behaviour_) {
            case Behaviour::CloseImmediately: {
                boost::system::error_code ignored;
                socket->close(ignored);
                break;
            }
            case Behaviour::SendGarbage: {
                // "ZZZZZZZZ" cannot be read as the hexadecimal payload length that the ecFlow
                // framing expects, so the client rejects the reply as undecodable.
                boost::system::error_code ignored;
                boost::asio::write(*socket, boost::asio::buffer(std::string("ZZZZZZZZnot a frame")), ignored);
                socket->close(ignored);
                break;
            }
            case Behaviour::StaySilent: {
                // Retain the socket, so that it stays open, and never reply on it
                held_.push_back(socket);
                break;
            }
            case Behaviour::AnswerHttp: {
                // Read whatever arrives, then answer with a valid HTTP status line. The status is
                // an error, which is exactly the point: an error status still proves that the peer
                // speaks HTTP.
                auto buffer = std::make_shared<std::array<char, 4096>>();
                socket->async_read_some(boost::asio::buffer(*buffer),
                                        [socket, buffer](const boost::system::error_code&, std::size_t) {
                                            static const std::string response = "HTTP/1.1 503 Service Unavailable\r\n"
                                                                                "Content-Length: 0\r\n"
                                                                                "Connection: close\r\n"
                                                                                "\r\n";
                                            boost::system::error_code ignored;
                                            boost::asio::write(*socket, boost::asio::buffer(response), ignored);
                                            socket->close(ignored);
                                        });
                break;
            }
        }
    }

    Behaviour behaviour_;
    boost::asio::io_context io_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::string port_;
    std::thread worker_;
    std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>> held_;
};

///
/// @brief Provides a port on the loopback interface with nothing listening on it.
///
/// The port is obtained by binding an acceptor and then closing it, which makes the port free
/// again while keeping the chance of a collision negligible.
///
std::string unused_port() {
    boost::asio::io_context io;
    boost::asio::ip::tcp::acceptor acceptor(
        io, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 0));
    auto port = ecf::convert_to<std::string>(acceptor.local_endpoint().port());
    acceptor.close();
    return port;
}

/// Runs a plain TCP/IP request against the given endpoint, and returns the resulting diagnosis.
ConnectionDiagnosis run_plain_request(const std::string& host, const std::string& port) {
    ConnectionDiagnosis diagnosis;
    try {
        boost::asio::io_context io;
        Client client(io, std::make_shared<CtsCmd>(CtsCmd::PING), host, port, SHORT_TIMEOUT, &diagnosis);
        io.run();
    }
    catch (const std::exception&) {
        // The transport reports several failures by throwing. The diagnosis outlives the throw,
        // which is the reason the caller owns the storage.
    }
    return diagnosis;
}

/// Runs an HTTP request against the given endpoint, and returns the resulting diagnosis.
ConnectionDiagnosis run_http_request(const std::string& host, const std::string& port) {
    ConnectionDiagnosis diagnosis;
    try {
        HttpClient client(std::make_shared<CtsCmd>(CtsCmd::PING), "http", host, port, 2, &diagnosis);
        client.run();
        ServerReply reply;
        client.handle_server_response(reply, false);
    }
    catch (const std::exception&) {
        // handle_server_response() reports a non-success status by throwing
    }
    return diagnosis;
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_ConnectionDiagnosisAgainstPeers)

// The plain TCP/IP transport

BOOST_AUTO_TEST_CASE(test_plain_client_reports_nothing_listening_as_a_refused_connection) {
    ECF_NAME_THIS_TEST();

    auto diagnosis = run_plain_request(FakePeer::host(), unused_port());

    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::ConnectionRefused);
    BOOST_CHECK_EQUAL(diagnosis.client_protocol, Protocol::Plain);
    // Nothing accepted the connection, so nothing suggests a protocol mismatch. Reporting one here
    // would send a user chasing a configuration problem instead of starting the server.
    BOOST_CHECK(!diagnosis.is_protocol_mismatch());
}

BOOST_AUTO_TEST_CASE(test_plain_client_reports_a_peer_that_accepts_and_closes) {
    ECF_NAME_THIS_TEST();

    // This is what an SSL server, and what an HTTP server, both look like to a plain client: the
    // connection is accepted, and then dropped without a reply.
    FakePeer peer(FakePeer::Behaviour::CloseImmediately);

    auto diagnosis = run_plain_request(FakePeer::host(), peer.port());

    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::ClosedWithoutReply);
    BOOST_CHECK_EQUAL(diagnosis.client_protocol, Protocol::Plain);
    BOOST_CHECK_EQUAL(diagnosis.host, FakePeer::host());
    BOOST_CHECK_EQUAL(diagnosis.port, peer.port());
    BOOST_CHECK(diagnosis.is_protocol_mismatch());
}

BOOST_AUTO_TEST_CASE(test_plain_client_reports_an_undecodable_reply) {
    ECF_NAME_THIS_TEST();

    FakePeer peer(FakePeer::Behaviour::SendGarbage);

    auto diagnosis = run_plain_request(FakePeer::host(), peer.port());

    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::UndecodableReply);
    BOOST_CHECK(diagnosis.is_protocol_mismatch());
}

BOOST_AUTO_TEST_CASE(test_plain_client_reports_a_silent_peer_as_a_timeout) {
    ECF_NAME_THIS_TEST();

    FakePeer peer(FakePeer::Behaviour::StaySilent);

    auto diagnosis = run_plain_request(FakePeer::host(), peer.port());

    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::Timeout);
    // A peer that holds the connection open may simply be slow, so a timeout is not, on its own,
    // evidence of a mismatch.
    BOOST_CHECK(!diagnosis.is_protocol_mismatch());
}

// The HTTP transport

BOOST_AUTO_TEST_CASE(test_http_client_reports_nothing_listening_as_a_refused_connection) {
    ECF_NAME_THIS_TEST();

    auto diagnosis = run_http_request(FakePeer::host(), unused_port());

    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::ConnectionRefused);
    BOOST_CHECK_EQUAL(diagnosis.client_protocol, Protocol::Http);
    BOOST_CHECK(!diagnosis.is_protocol_mismatch());
}

BOOST_AUTO_TEST_CASE(test_http_client_reports_a_peer_that_does_not_answer_http) {
    ECF_NAME_THIS_TEST();

    // This is what a plain, or an SSL, ecFlow server looks like to an HTTP client. Before the
    // diagnosis existed, this was indistinguishable from nothing listening at all.
    FakePeer peer(FakePeer::Behaviour::CloseImmediately);

    auto diagnosis = run_http_request(FakePeer::host(), peer.port());

    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::ClosedWithoutReply);
    BOOST_CHECK_EQUAL(diagnosis.client_protocol, Protocol::Http);
    BOOST_CHECK(diagnosis.is_protocol_mismatch());
    BOOST_CHECK(!diagnosis.peer_protocol.has_value());
}

BOOST_AUTO_TEST_CASE(test_http_client_reports_an_http_error_status_as_a_refused_request) {
    ECF_NAME_THIS_TEST();

    FakePeer peer(FakePeer::Behaviour::AnswerHttp);

    auto diagnosis = run_http_request(FakePeer::host(), peer.port());

    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::RejectedRequest);
    // The peer answered with a well-formed HTTP response, which settles what it speaks
    BOOST_REQUIRE(diagnosis.peer_protocol.has_value());
    BOOST_CHECK_EQUAL(diagnosis.peer_protocol.value(), Protocol::Http);
    BOOST_CHECK(!diagnosis.is_protocol_mismatch());
}

// The protocol probe

BOOST_AUTO_TEST_CASE(test_the_probe_identifies_a_peer_that_speaks_http) {
    ECF_NAME_THIS_TEST();

    FakePeer peer(FakePeer::Behaviour::AnswerHttp);

    BOOST_CHECK(ecf::probes_as_http("http", FakePeer::host(), peer.port(), SHORT_TIMEOUT));
    BOOST_CHECK(!ecf::probes_as_plain(FakePeer::host(), peer.port(), SHORT_TIMEOUT));

    auto found = ecf::probe_server_protocol(FakePeer::host(), peer.port(), SHORT_TIMEOUT);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(found.value(), Protocol::Http);
}

BOOST_AUTO_TEST_CASE(test_the_probe_reports_nothing_for_a_peer_it_cannot_identify) {
    ECF_NAME_THIS_TEST();

    // A peer that accepts and closes speaks none of the four protocols. Answering "unknown" is the
    // required behaviour: naming a protocol here would be a guess, and a wrong diagnosis is worse
    // than an incomplete one.
    FakePeer peer(FakePeer::Behaviour::CloseImmediately);

    BOOST_CHECK(!ecf::probes_as_plain(FakePeer::host(), peer.port(), SHORT_TIMEOUT));
    BOOST_CHECK(!ecf::probes_as_http("http", FakePeer::host(), peer.port(), SHORT_TIMEOUT));
    BOOST_CHECK(!ecf::probes_as_tls(FakePeer::host(), peer.port(), SHORT_TIMEOUT));

    BOOST_CHECK(!ecf::probe_server_protocol(FakePeer::host(), peer.port(), SHORT_TIMEOUT).has_value());
}

BOOST_AUTO_TEST_CASE(test_the_probe_reports_nothing_when_nothing_is_listening) {
    ECF_NAME_THIS_TEST();

    const auto port = unused_port();

    BOOST_CHECK(!ecf::probes_as_plain(FakePeer::host(), port, SHORT_TIMEOUT));
    BOOST_CHECK(!ecf::probes_as_http("http", FakePeer::host(), port, SHORT_TIMEOUT));
    BOOST_CHECK(!ecf::probes_as_tls(FakePeer::host(), port, SHORT_TIMEOUT));

    BOOST_CHECK(!ecf::probe_server_protocol(FakePeer::host(), port, SHORT_TIMEOUT).has_value());
}

// The explanation, built from what actually happened

BOOST_AUTO_TEST_CASE(test_the_explanation_of_a_real_failure_names_the_endpoint_and_the_protocol) {
    ECF_NAME_THIS_TEST();

    FakePeer peer(FakePeer::Behaviour::CloseImmediately);

    auto diagnosis         = run_plain_request(FakePeer::host(), peer.port());
    const auto explanation = ecf::explain(diagnosis);

    BOOST_CHECK_MESSAGE(explanation.find("Protocol mismatch") != std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find("TCP/IP") != std::string::npos, explanation);
    BOOST_CHECK_MESSAGE(explanation.find(peer.port()) != std::string::npos, explanation);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
