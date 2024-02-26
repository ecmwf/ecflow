/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_udp_UDPServer_HPP
#define ecflow_udp_UDPServer_HPP

#include <string>

#include <boost/asio.hpp>

#include "ecflow/udp/RequestHandler.hpp"

namespace ecf {

namespace internal_detail {

template <typename HANDLER>
class BaseUdpServerConnection {
public:
    BaseUdpServerConnection(HANDLER handler,
                            boost::asio::io_service& io_service,
                            const boost::asio::ip::udp::endpoint& server_endpoint)
        : handler_{std::move(handler)},
          socket_(io_service, server_endpoint),
          client_endpoint_{},
          buffer_{} {
        start();
    }

private:
    void start() {
        // Receive request
        socket_.async_receive_from(boost::asio::buffer(buffer_),
                                   client_endpoint_,
                                   [this](const boost::system::error_code& error, size_t bytes_transferred) {
                                       this->handleReceive(error, bytes_transferred);
                                   });
    }

    void handleReceive(const boost::system::error_code& error, std::size_t bytes_transferred) {
        if (!error) {
            // Take inbound request from (inbound) buffer...
            std::string request;
            std::copy_n(std::begin(buffer_), bytes_transferred, std::back_inserter(request));

            // Process request
            handler_.handle(request);
        }
        else {
            std::cerr << "    BaseUDPServer (error): failure when receiving message" << std::endl;
        }

        start();
    }

private:
    HANDLER handler_;
    boost::asio::ip::udp::socket socket_;

    // Used as information passed between async calls
    boost::asio::ip::udp::endpoint client_endpoint_;
    std::array<char, 65'507> buffer_;
};

} // namespace internal_detail

/**
 * The UDP server listens for network packets, and delegates the requests to the given HANDLER
 */
template <typename HANDLER>
class BaseUdpServer {
public:
    BaseUdpServer(HANDLER handler, uint16_t port)
        : io_service_{},
          server_endpoint_{boost::asio::ip::udp::v4(), port},
          connection_{handler, io_service_, server_endpoint_} {}
    BaseUdpServer(const BaseUdpServer&) = delete;
    BaseUdpServer(BaseUdpServer&&)      = delete;

    void run() { io_service_.run(); }

private:
    boost::asio::io_service io_service_;
    boost::asio::ip::udp::endpoint server_endpoint_;
    internal_detail::BaseUdpServerConnection<HANDLER> connection_;
};

using UDPServer = BaseUdpServer<RequestHandler>;

} // namespace ecf

#endif /* ecflow_udp_UDPServer_HPP */
