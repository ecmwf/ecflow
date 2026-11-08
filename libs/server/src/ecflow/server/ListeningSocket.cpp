// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/server/ListeningSocket.hpp"

#include <stdexcept>
#include <string>

namespace ecf {

namespace {

void throw_on_error(const boost::system::error_code& ec,
                    const std::string& step,
                    const boost::asio::ip::tcp::endpoint& endpoint) {
    if (ec) {
        throw std::runtime_error("Could not " + step + " port " + std::to_string(endpoint.port()) + ": " +
                                 ec.message());
    }
}

} // namespace

void open_listening_socket(boost::asio::ip::tcp::acceptor& acceptor, const boost::asio::ip::tcp::endpoint& endpoint) {
    boost::system::error_code ec;

    acceptor.open(endpoint.protocol(), ec);
    throw_on_error(ec, "open a socket for", endpoint);

    acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
    throw_on_error(ec, "set the socket options for", endpoint);

    acceptor.bind(endpoint, ec);
    throw_on_error(ec, "bind", endpoint);

    acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
    throw_on_error(ec, "listen on", endpoint);
}

} // namespace ecf
