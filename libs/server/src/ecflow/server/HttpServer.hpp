/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_server_HttpServer_HPP
#define ecflow_server_HttpServer_HPP

#include <memory>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "ecflow/server/BaseServer.hpp"

/**
 * The HttpServer class accepts HTTP connections, which are dispatched to appropriate sessions that
 * take care of receiving a Request, processing it using the BaseServer and sending back the Response.
 *
 * The current implementation is based on Boost.ASIO/Beast. This necessary because the trigerring
 * mechanism to periodically traverse the BaseServer/Suite(s) is tightly coupled with Boost.ASIO.
 */

class HttpServer : public std::enable_shared_from_this<HttpServer> {
private:
    BaseServer* server_;
    boost::asio::io_context& io_;
    boost::asio::ip::tcp::acceptor acceptor_;

public:
    HttpServer(BaseServer* server, boost::asio::io_context& io, ServerEnvironment& env);

    std::string ssl() const { return ""; }
    BaseServer* server() const { return server_; }

    void handle_terminate(bool terminate);

private:
    void do_accept();
    void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket, BaseServer* server);
};

#endif /* ecflow_server_HttpServer_HPP */
