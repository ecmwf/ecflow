/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_server_Server_HPP
#define ecflow_server_Server_HPP

#include "ecflow/server/BaseServer.hpp"
#include "ecflow/server/TcpServer.hpp"

class Server : public BaseServer {
public:
    /// Constructor opens the acceptor and starts waiting for the first incoming connection.
    explicit Server(boost::asio::io_service& io_service, ServerEnvironment&);
    ~Server() override = default;

private:
    TcpServer tcp_server_;
};

#endif /* ecflow_server_Server_HPP */
