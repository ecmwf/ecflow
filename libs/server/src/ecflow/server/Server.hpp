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

#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/server/BaseServer.hpp"
#include "ecflow/server/HttpServer.hpp"
#include "ecflow/server/ServerEnvironment.hpp"
#ifdef ECF_OPENSSL
    #include "ecflow/server/SslTcpServer.hpp"
#endif
#include "ecflow/server/TcpServer.hpp"

template <typename U>
class DefaultServer : public BaseServer {
public:
    /// Constructor opens the acceptor and starts waiting for the first incoming connection.
    explicit DefaultServer(boost::asio::io_context& io, ServerEnvironment& env)
        : BaseServer(io, env),
          server_(this, io, env) {}
    ~DefaultServer() override = default;

    std::string ssl() const override {
#ifdef ECF_OPENSSL
        return serverEnv_.openssl().ssl();
#else
        return "";
#endif
    }

private:
    U server_;
};

using BasicServer = DefaultServer<TcpServer>;
#ifdef ECF_OPENSSL
using BasicSslServer = DefaultServer<SslTcpServer>;
#endif
using BasicHttpServer = DefaultServer<HttpServer>;

#endif /* ecflow_server_Server_HPP */
