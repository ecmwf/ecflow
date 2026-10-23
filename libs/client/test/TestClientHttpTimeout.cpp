// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using namespace ecf;
using namespace ecf::test::scaffold;

namespace {

///
/// @brief A peer that accepts one connection, reads the request, and never replies.
///
/// The peer runs on a thread of its own, from construction until destruction, so that the client under test
/// can block while the peer records what it received.
///
class SilentPeer {
public:
    ///
    /// @brief Starts listening on the given loopback port, and waits for one connection.
    ///
    /// @param[in] host The loopback address to listen on
    /// @param[in] port The port to listen on; reserved by the caller
    /// @throws boost::system::system_error if the listening socket cannot be opened or bound
    ///
    SilentPeer(const std::string& host, unsigned short port)
        : acceptor_(io_, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(host), port)),
          socket_(io_) {
        acceptor_.async_accept(socket_, [this](const boost::system::error_code& error) {
            if (!error) {
                read();
            }
        });
        worker_ = std::thread([this] { io_.run(); });
    }

    SilentPeer(const SilentPeer&)            = delete;
    SilentPeer& operator=(const SilentPeer&) = delete;

    ///
    /// @brief Stops the peer, closing the connection and the listening socket.
    ///
    ~SilentPeer() {
        io_.stop();
        worker_.join();
    }

    ///
    /// @brief Returns the number of request bytes received so far.
    ///
    /// @return The number of bytes read from the accepted connection; 0 when nothing was received
    ///
    [[nodiscard]] std::size_t received() const { return received_; }

private:
    /// Reads whatever the client sends, and keeps the connection open without replying.
    void read() {
        socket_.async_read_some(boost::asio::buffer(buffer_),
                                [this](const boost::system::error_code& error, std::size_t length) {
                                    if (!error) {
                                        received_ += length;
                                        read();
                                    }
                                });
    }

    boost::asio::io_context io_;              ///< Drives the accept and the reads
    boost::asio::ip::tcp::acceptor acceptor_; ///< Listens for the connection of the client
    boost::asio::ip::tcp::socket socket_;     ///< The accepted connection, held open
    std::array<char, 4096> buffer_{};         ///< Receives the request bytes, which are discarded
    std::atomic<std::size_t> received_{0};    ///< The number of request bytes received
    std::thread worker_;                      ///< Runs the execution context
};

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_ClientHttpTimeout)

BOOST_AUTO_TEST_CASE(test_http_client_gives_up_after_the_configured_connect_timeout) {
    ECF_NAME_THIS_TEST();

    // No tokens file exists at this location, so no token is read from the user configuration
    auto directory = MakeDirectory{}.create();
    WithTestEnvironmentVariable tokens("ECF_AUTHTOKENS", (directory.path() / "missing.ecflowapirc").string());

    const std::string host = "127.0.0.1";
    const auto reserved    = MakePort{}.with(AutomaticPortValue{}).create();
    SilentPeer peer(host, static_cast<unsigned short>(reserved.value()));

    ClientInvoker client(host, std::to_string(reserved.value()));
    client.set_throw_on_error(false);
    client.set_connection_attempts(1);
    client.enable_http();
    client.set_connect_timeout(std::chrono::seconds{1});

    const auto start = std::chrono::steady_clock::now();
    BOOST_REQUIRE_MESSAGE(client.pingServer() == 1, "expected the request to fail");
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

    // The request reached the peer, so the failure is not one that happened before any exchange
    BOOST_CHECK_MESSAGE(peer.received() > 0, "expected the request to reach the peer: " << client.errorMsg());

    // The reply did not arrive in time, on a connection that the peer kept open
    const auto& diagnosis = client.connection_diagnosis();
    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::Timeout);
    BOOST_CHECK(!diagnosis.is_protocol_mismatch());

    // The client waited for the configured timeout, and not for the timeout of the ping command (10 seconds), nor
    // for the former fixed timeout (120 seconds); the upper bound leaves room for a loaded machine
    BOOST_CHECK_MESSAGE(elapsed >= std::chrono::milliseconds{900},
                        "expected the request to wait for about 1 second, but it failed after " << elapsed.count()
                                                                                                << "ms");
    BOOST_CHECK_MESSAGE(elapsed < std::chrono::seconds{5},
                        "expected the request to time out after about 1 second, but it took " << elapsed.count()
                                                                                              << "ms");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
