/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_udp_UDPClient_HPP
#define ecflow_udp_UDPClient_HPP

#include <string>

#include <boost/asio.hpp>

namespace ecf {

namespace internal_detail {

/**
 * The UDP connection implemented using Boost.ASIO, sends a single Datagram in a "fire and forget" protocol.
 */
class BaseUDPConnection {
public:
    using resolver_t      = boost::asio::ip::udp::resolver;
    using endpoints_set_t = resolver_t::results_type;
    using endpoint_t      = endpoints_set_t::endpoint_type;

    BaseUDPConnection(boost::asio::io_context& io, endpoint_t server_endpoint, const std::string& request)
        : socket_(io),
          server_endpoint_(std::move(server_endpoint)) {
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
                                  this->handle_send(error, bytes_transferred);
                              });
    }

    void handle_send(const boost::system::error_code& error, std::size_t bytes_transferred [[maybe_unused]]) {
        if (error) {
            throw std::runtime_error("Unable to send request to ecFlow UDP");
        }
    }

    boost::asio::ip::udp::socket socket_;

    boost::asio::ip::udp::endpoint server_endpoint_;
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

    using resolver_t           = internal_detail::BaseUDPConnection::resolver_t;
    using endpoints_set_t      = resolver_t::results_type;
    using endpoints_iterator_t = endpoints_set_t::iterator;

    BaseUDPClient(hostname_t host, port_t port) : host_{std::move(host)}, port_{std::move(port)} {}

    void send(const data_t& data) {
        boost::asio::io_context io;
        auto resolver        = resolver_t(io);
        auto results         = resolver.resolve(boost::asio::ip::udp::v4(), host_, port_);
        auto server_endpoint = std::begin(results)->endpoint();

        internal_detail::BaseUDPConnection connection(io, server_endpoint, data);
        io.run();
    }

private:
    hostname_t host_;
    port_t port_;
};

using UDPClient = BaseUDPClient;

} // namespace ecf

#endif /* ecflow_udp_UDPClient_HPP */
