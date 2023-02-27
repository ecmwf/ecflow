/*
 * Copyright 2009-2023 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_UDP_UDPCLIENT_HPP
#define ECFLOW_UDP_UDPCLIENT_HPP

#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <unistd.h>
#include <utility>

#include <boost/asio.hpp>
#include <sys/socket.h>
#include <sys/types.h>

namespace ecf {

namespace internal_detail {

/**
 * The UDP connection implemented using Boost.ASIO, sends a single Datagram in a "fire and forget" protocol.
 */
class BaseUDPConnection {
public:
    BaseUDPConnection(boost::asio::io_service& io_service,
                      boost::asio::ip::udp::endpoint server_endpoint,
                      const std::string& request)
        : socket_(io_service),
          server_endpoint_(std::move(server_endpoint)),
          buffer_{} {
        // Open socket connection
        socket_.open(boost::asio::ip::udp::v4());
        // Start 'protocol'...
        start(request);
    }

private:
    void start(const std::string& request) {
        std::array<char, 65'507> request_buffer{};
        std::copy_n(std::begin(request), request.size() + 1, std::begin(request_buffer));

        // Send request
        socket_.async_send_to(boost::asio::buffer(request_buffer, request.size() + 1),
                              server_endpoint_,
                              [this](const boost::system::error_code& error, size_t bytes_transferred) {
                                  this->handleSend(error, bytes_transferred);
                              });
    }

    void handleSend(const boost::system::error_code& error, std::size_t bytes_transferred) {
        if (error) {
            throw std::runtime_error("Unable to send request to ecFlow UDP");
        }
    }

    boost::asio::ip::udp::socket socket_;

    boost::asio::ip::udp::endpoint server_endpoint_;
    std::array<char, 65'507> buffer_;
};

} // namespace internal_detail

/**
 * The UDP client implemented using Boost.ASIO.
 */
class BaseUDPClient {
public:
    using hostname_t = std::string;
    using port_t     = std::string;
    using data_t     = std::string;

public:
    BaseUDPClient(hostname_t host, port_t port) : host_{std::move(host)}, port_{std::move(port)} {}

    void send(const data_t& data) {
        boost::asio::io_service io_service;
        boost::asio::ip::udp::resolver resolver(io_service);
        boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), host_, port_);
        boost::asio::ip::udp::endpoint server_endpoint = *resolver.resolve(query);

        internal_detail::BaseUDPConnection connection(io_service, server_endpoint, data);
        io_service.run();
    }

private:
    hostname_t host_;
    port_t port_;
};

/**
 * The UDP client implemented directly using Linux facilities
 * (as per ecKit, see https://github.com/ecmwf/eckit/blob/develop/src/eckit/net/UDPClient.cc).
 */
class ECKITUDPClient {
public:
    using hostname_t = std::string;
    using port_t     = std::string;
    using data_t     = std::string;

public:
    ECKITUDPClient(hostname_t hostname, port_t port)
        : hostname_(std::move(hostname)),
          port_(std::move(port)),
          socketfd_(0),
          servinfo_(nullptr),
          addr_(nullptr) {
        init();
    }
    ECKITUDPClient(const ECKITUDPClient&) = delete;
    ECKITUDPClient(ECKITUDPClient&&)      = delete;

    ~ECKITUDPClient() noexcept(false) {
        ::freeaddrinfo(servinfo_);
        auto code = ::close(socketfd_);
        if (code < 0) {
            throw std::runtime_error("unable to close socket");
        }
    }

    void init() {
        struct addrinfo hints;

        ::memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

        if (int err; (err = ::getaddrinfo(hostname_.c_str(), port_.c_str(), &hints, &servinfo_)) != 0) {
            std::ostringstream msg;
            msg << "getaddrinfo failed in UDPClient with "
                << " hostname=" << hostname_ << " port=" << port_ << " --  " << ::gai_strerror(err);
            throw std::runtime_error(msg.str());
        }

        // loop through all the addrinfo results and make a socket
        for (addr_ = servinfo_; addr_ != nullptr; addr_ = addr_->ai_next) {
            if ((socketfd_ = ::socket(addr_->ai_family, addr_->ai_socktype, addr_->ai_protocol)) == -1) {
                continue;
            }
            break;
        }

        if (!addr_) {
            std::ostringstream msg;
            msg << "UDPClient failed to create a socket";
            throw std::runtime_error(msg.str());
        }
    }

    void send(const data_t& data) { send(data.data(), data.size() + 1); }

    void send(const void* buffer, size_t length) {
        ssize_t sent = ::sendto(socketfd_, buffer, length, 0, addr_->ai_addr, addr_->ai_addrlen);
        if (sent == -1) {
            std::ostringstream msg;
            msg << "UDPClient failed to send " << length << " to host " << hostname_;
            throw std::runtime_error(msg.str());
        }
    }

    void print(std::ostream& s) const {
        s << "UDPClient[hostname=" << hostname_ << ",port=" << port_ << ",socketfd=" << socketfd_ << "]";
    }

private: // members
    std::string hostname_;
    std::string port_;
    int socketfd_;

    struct addrinfo* servinfo_;
    struct addrinfo* addr_;
};

#if defined(ECFLOW_UDP_CLIENT_BASED_ON_ECKIT)
using UDPClient = ECKITUDPClient;
#else
using UDPClient = BaseUDPClient;
    #if !defined(ECFLOW_UDP_CLIENT_BASED_ON_BOOST_ASIO)
        #warning \
            "Using UDP client based on Boost.ASIO. Explicitly #define ECFLOW_UDP_CLIENT_BASED_ON_BOOST_ASIO to suppress this warning"
    #endif
#endif

} // namespace ecf

#endif
