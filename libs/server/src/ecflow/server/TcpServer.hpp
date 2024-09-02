/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_server_TcpServer_HPP
#define ecflow_server_TcpServer_HPP

#include "ecflow/base/Connection.hpp"
#include "ecflow/server/TcpBaseServer.hpp"
class Server;

class TcpServer : public TcpBaseServer {
public:
    /// Constructor opens the acceptor and starts waiting for the first incoming connection.
    explicit TcpServer(Server*, boost::asio::io_context& io, ServerEnvironment&);
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
