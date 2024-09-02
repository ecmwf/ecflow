/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_server_SslServer_HPP
#define ecflow_server_SslServer_HPP

#include "ecflow/server/BaseServer.hpp"
#include "ecflow/server/SslTcpServer.hpp"

class SslServer : public BaseServer {
public:
    /// Constructor opens the acceptor and starts waiting for the first incoming connection.
    explicit SslServer(boost::asio::io_context& io, ServerEnvironment&);
    ~SslServer() override = default;

private:
    /// AbstractServer functions
    const std::string& ssl() const override;

    SslTcpServer server_;
};

#endif
