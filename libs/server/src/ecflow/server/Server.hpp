/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
