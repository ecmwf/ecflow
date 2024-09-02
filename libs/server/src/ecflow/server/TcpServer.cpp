/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/server/TcpServer.hpp"

#include <iostream>

#include "ecflow/core/Log.hpp"
#include "ecflow/server/Server.hpp"
#include "ecflow/server/ServerEnvironment.hpp"

using boost::asio::ip::tcp;

using namespace std;
using namespace ecf;

TcpServer::TcpServer(Server* server, boost::asio::io_context& io, ServerEnvironment& serverEnv)
    : TcpBaseServer(server, io, serverEnv) {
    // timer_.stop(); // for timing of commands.

    start_accept();
}

void TcpServer::start_accept() {
    if (serverEnv_.debug())
        cout << "   TcpServer::start_accept()" << endl;
    connection_ptr new_conn = std::make_shared<connection>(boost::ref(io_));
    acceptor_.async_accept(new_conn->socket_ll(),
                           [this, new_conn](const boost::system::error_code& e) { handle_accept(e, new_conn); });
}

void TcpServer::handle_accept(const boost::system::error_code& e, connection_ptr conn) {
    if (serverEnv_.debug())
        cout << "   TcpServer::handle_accept" << endl;

    // Check whether the server was stopped by a signal before this completion
    // handler had a chance to run.
    if (!acceptor_.is_open()) {
        if (serverEnv_.debug())
            cout << "   TcpServer::handle_accept:  acceptor is closed, returning" << endl;
        return;
    }

    if (!e) {
        // Read and interpret message from the client
        // Successfully accepted a new connection. Determine what the
        // client sent to us. The connection::async_read() function will
        // automatically. serialise the inbound_request_ data structure for us.
        conn->async_read(inbound_request_,
                         [this, conn](const boost::system::error_code& error) { this->handle_read(error, conn); });
    }
    else {
        if (serverEnv_.debug())
            cout << "   TcpServer::handle_accept " << e.message() << endl;
        if (e != boost::asio::error::operation_aborted) {
            // An error occurred. Log it
            LogToCout toCoutAsWell;
            LogFlusher logFlusher;
            LOG(Log::ERR, "   TcpServer::handle_accept error occurred  " << e.message());
        }
    }

    // Start an accept operation for a new connection.
    // *NOTE* previously we had *ONLY* called this if there was no errors
    //        However this would means that server would run out work.
    //        When there were errors.!
    // Moved here to follow the examples used in ASIO.
    // However can this get into an infinite loop ???
    start_accept();
}

void TcpServer::handle_read(const boost::system::error_code& e, connection_ptr conn) {
    // start read
    // timer_.start();

    /// Handle completion of a write operation.
    // **********************************************************************************
    // This function *must* finish with write, otherwise it ends up being called recursively
    // ***********************************************************************************
    if (!e) {

        handle_request(); // populates outbound_response_
        // log(Log::DBG," handle_read()  n"  + timer_.format(3,Str::cpu_timer_format()));

        // start write
        // timer_.start();

        // Always *Reply* back to the client, Otherwise client will get EOF
        conn->async_write(outbound_response_,
                          [this, conn](const boost::system::error_code& error) { this->handle_write(error, conn); });
    }
    else {
        handle_read_error(e); // populates outbound_response_
        conn->async_write(outbound_response_,
                          [this, conn](const boost::system::error_code& error) { this->handle_write(error, conn); });
    }
}

void TcpServer::handle_write(const boost::system::error_code& e, connection_ptr conn) {
    // Handle completion of a write operation.
    // Nothing to do. The socket will be closed automatically when the last
    // reference to the connection object goes away.
    if (serverEnv_.debug())
        cout << "   TcpServer::handle_write: client request " << inbound_request_ << " replying with  "
             << outbound_response_ << endl;

    if (e) {
        LogFlusher logFlusher;
        ecf::LogToCout logToCout;
        std::stringstream ss;
        ss << "TcpServer::handle_write: " << e.message() << " : for request " << inbound_request_;
        log(Log::ERR, ss.str());
        return;
    }

    // Do any necessary clean up after outbound_response_  has run. i.e like re-claiming memory
    outbound_response_.cleanup();

    (void)shutdown_socket(conn, "TcpServer::handle_write:");

    // If asked to terminate we do it here rather than in handle_read.
    // So that we have responded to the client.
    // *HOWEVER* only do this if the request was successful.
    //           we do this by checking that the out bound response was ok
    //           i.e a read only user should not be allowed to terminate server.
    handle_terminate_request();

    // log(Log::DBG," handle_write() "  + timer_.format(3,Str::cpu_timer_format()));
}
