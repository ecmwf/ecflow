// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <boost/asio.hpp>

namespace ecf {

///
/// @brief Opens the acceptor, binds it to the endpoint (with SO_REUSEADDR) and starts listening.
///
/// @param[in,out] acceptor Acceptor of the server, not yet open
/// @param[in] endpoint Address and port on which the server accepts connections
/// @throws std::runtime_error naming the failed step, the port and the cause (for example, a port already in use)
///
void open_listening_socket(boost::asio::ip::tcp::acceptor& acceptor, const boost::asio::ip::tcp::endpoint& endpoint);

} // namespace ecf
