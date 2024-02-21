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

#include <httplib.h>

#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/server/BaseServer.hpp"
#include "ecflow/server/ServerEnvironment.hpp"
#include "ecflow/server/SslTcpServer.hpp"
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
        if constexpr (ECF_OPENSSL == 1) {
            return serverEnv_.openssl().ssl();
        }
        else {
            return "";
        }
    }

private:
    U server_;
};

using Server    = DefaultServer<TcpServer>;
using SslServer = DefaultServer<SslTcpServer>;

class HttpServer : public BaseServer {
public:
    explicit HttpServer(boost::asio::io_service& service, ServerEnvironment& env) : BaseServer(service, env) {}
    ~HttpServer() override = default;

    std::string ssl() const override { return ""; }

    void run() {
        httplib::Server server;

        server.Post("/v1/ecflow", [this](const httplib::Request& request, httplib::Response& response) {
            // Buffers to hold request/response
            ClientToServerRequest inbound_request;
            ServerToClientResponse outbound_response;

            // 1) unserialize request body into inbound_request
            ecf::restore_from_string(request.body, inbound_request);

            // 2) handle request, as per TcpBaseServer::handle_request()
            {
                // See what kind of message we got from the client
                if (serverEnv_.debug()) {
                    std::cout << "   TcpBaseServer::handle_request  : client request " << inbound_request << std::endl;
                }

                try {
                    // Service the in bound request, handling the request will populate the outbound_response_
                    // Note:: Handle request will first authenticate
                    outbound_response.set_cmd(inbound_request.handleRequest(this));
                }
                catch (std::exception& e) {
                    outbound_response.set_cmd(PreAllocatedReply::error_cmd(e.what()));
                }

                // Do any necessary clean up after inbound_request_ has run. i.e like re-claiming memory
                inbound_request.cleanup();
            }

            // 3) serialize outbound_response into response body
            std::string outbound;
            ecf::save_as_string(outbound, outbound_response);

            // 4) ship response
            response.set_content(outbound, "text/plain");
        });

        try {
            auto [host, port] = serverEnv_.hostPort();

            bool ret = server.listen(host, ecf::convert_to<int>(port));
            if (ret == false) {
                throw std::runtime_error(std::string("Failed to start HTTP server"));
            }
        }
        catch (const std::exception& e) {
            std::cout << "[HttpServer] Failure running HTTP server:" << e.what() << std::endl;
        }
    }
};

#endif /* ecflow_server_Server_HPP */
