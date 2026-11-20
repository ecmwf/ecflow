// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include <atomic>
#include <iomanip>
#include <poll.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>

#include <arpa/inet.h>
#include <boost/test/unit_test.hpp>
#include <netinet/in.h>
#include <sys/socket.h>

#include "ecflow/base/ServerToClientResponse.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_OwnershipOlderServer)

namespace {

/// The reply recorded from a server without the ownership command (5.19.0), which cannot decode the request
const std::string older_server_reply = " ->Server:iona@9417 (5.19.0) replied with: Invalid argument";

/// A server answering every request with the error of an older server, as it does for an unknown command
class OlderServer {
public:
    explicit OlderServer(unsigned port, int requests)
        : socket_(::socket(AF_INET, SOCK_STREAM, 0)) {
        BOOST_REQUIRE(socket_ >= 0);
        int yes = 1;
        ::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        sockaddr_in address{};
        address.sin_family      = AF_INET;
        address.sin_port        = htons(static_cast<uint16_t>(port));
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        BOOST_REQUIRE(::bind(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0);
        BOOST_REQUIRE(::listen(socket_, 4) == 0);
        thread_ = std::thread([this, requests]() {
            for (int served = 0; served < requests && !stop_;) {
                pollfd ready{socket_, POLLIN, 0};
                if (::poll(&ready, 1, 100) > 0) {
                    serve_one();
                    ++served;
                }
            }
        });
    }
    OlderServer(const OlderServer&)            = delete;
    OlderServer& operator=(const OlderServer&) = delete;
    ~OlderServer() {
        stop_ = true;
        thread_.join();
        ::close(socket_);
    }

private:
    void serve_one() const {
        int connection = ::accept(socket_, nullptr, nullptr);
        if (connection < 0) {
            return;
        }
        // Read the request (a fixed-length header holding the size in hex, then the data), and ignore it
        std::array<char, 8> header{};
        if (read_all(connection, header.data(), header.size())) {
            std::size_t size = std::stoul(std::string(header.data(), header.size()), nullptr, 16);
            std::string data(size, '\0');
            read_all(connection, data.data(), size);

            ServerToClientResponse response;
            response.set_cmd(PreAllocatedReply::error_cmd(older_server_reply));
            std::string reply;
            ecf::save_as_string(reply, response);
            std::ostringstream reply_header;
            reply_header << std::setw(8) << std::hex << reply.size();
            const std::string message = reply_header.str() + reply;
            (void)::write(connection, message.data(), message.size());
        }
        ::close(connection);
    }

    static bool read_all(int connection, char* buffer, std::size_t size) {
        while (size > 0) {
            ssize_t n = ::read(connection, buffer, size);
            if (n <= 0) {
                return false;
            }
            buffer += n;
            size -= static_cast<std::size_t>(n);
        }
        return true;
    }

    int socket_;
    std::atomic<bool> stop_{false};
    std::thread thread_;
};

} // namespace

BOOST_AUTO_TEST_CASE(test_older_server_error_is_explained_by_the_client) {
    ECF_NAME_THIS_TEST();

    auto port = ecf::test::scaffold::MakePort{}.with(ecf::test::scaffold::AutomaticPortValue{19000}).create();
    OlderServer server(port.value(), 3);

    ClientInvoker client("127.0.0.1", ecf::convert_to<std::string>(port.value()));
    client.set_throw_on_error(false);

    // A single command, and a group holding one
    BOOST_CHECK(client.ownership("read", "/") == 1);
    BOOST_CHECK_MESSAGE(client.errorMsg().find("does not support the ownership command") != std::string::npos,
                        client.errorMsg());
    BOOST_CHECK_MESSAGE(client.errorMsg().find(older_server_reply) != std::string::npos, client.errorMsg());

    BOOST_CHECK(client.group("ownership read /; ping") == 1);
    BOOST_CHECK_MESSAGE(client.errorMsg().find("does not support the ownership command") != std::string::npos,
                        client.errorMsg());

    // Other commands are reported as before
    BOOST_CHECK(client.pingServer() == 1);
    BOOST_CHECK_MESSAGE(client.errorMsg().find("does not support the ownership command") == std::string::npos,
                        client.errorMsg());
    BOOST_CHECK_MESSAGE(client.errorMsg().find(older_server_reply) != std::string::npos, client.errorMsg());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
