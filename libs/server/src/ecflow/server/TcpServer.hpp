/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_server_TcpServer_HPP
#define ecflow_server_TcpServer_HPP

#include "ecflow/base/Connection.hpp"
#include "ecflow/server/TcpBaseServer.hpp"

class BaseServer;

class TcpServer : public TcpBaseServer {
public:
    /// Constructor opens the acceptor and starts waiting for the first incoming connection.
    explicit TcpServer(BaseServer*, boost::asio::io_context& io, ServerEnvironment&);
    ~TcpServer() = default;

private:
    /// Handle completion of a accept operation.
    void handle_accept(const boost::system::error_code& e, connection_ptr conn);

    /// Handle completion of a write operation.
    void handle_write(const boost::system::error_code& e, connection_ptr conn);

    /// Handle completion of a read operation.
    void handle_read(const boost::system::error_code& e, connection_ptr conn);

    void start_accept();

    // boost::timer::cpu_timer timer_; // time_cmds for debug
};

#endif /* ecflow_server_TcpServer_HPP */
