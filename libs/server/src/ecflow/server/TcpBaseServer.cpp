/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/server/TcpBaseServer.hpp"

#include <iostream>

#include "ecflow/base/cts/task/AbortCmd.hpp"         // required to enforce cereal registration
#include "ecflow/base/cts/task/CompleteCmd.hpp"      // required to enforce cereal registration
#include "ecflow/base/cts/task/CtsWaitCmd.hpp"       // required to enforce cereal registration
#include "ecflow/base/cts/task/EventCmd.hpp"         // required to enforce cereal registration
#include "ecflow/base/cts/task/InitCmd.hpp"          // required to enforce cereal registration
#include "ecflow/base/cts/task/LabelCmd.hpp"         // required to enforce cereal registration
#include "ecflow/base/cts/task/MeterCmd.hpp"         // required to enforce cereal registration
#include "ecflow/base/cts/task/QueueCmd.hpp"         // required to enforce cereal registration
#include "ecflow/base/cts/user/AlterCmd.hpp"         // required to enforce cereal registration
#include "ecflow/base/cts/user/BeginCmd.hpp"         // required to enforce cereal registration
#include "ecflow/base/cts/user/CFileCmd.hpp"         // required to enforce cereal registration
#include "ecflow/base/cts/user/CSyncCmd.hpp"         // required to enforce cereal registration
#include "ecflow/base/cts/user/CheckPtCmd.hpp"       // required to enforce cereal registration
#include "ecflow/base/cts/user/ClientHandleCmd.hpp"  // required to enforce cereal registration
#include "ecflow/base/cts/user/CtsCmd.hpp"           // required to enforce cereal registration
#include "ecflow/base/cts/user/CtsNodeCmd.hpp"       // required to enforce cereal registration
#include "ecflow/base/cts/user/DeleteCmd.hpp"        // required to enforce cereal registration
#include "ecflow/base/cts/user/EditScriptCmd.hpp"    // required to enforce cereal registration
#include "ecflow/base/cts/user/ForceCmd.hpp"         // required to enforce cereal registration
#include "ecflow/base/cts/user/FreeDepCmd.hpp"       // required to enforce cereal registration
#include "ecflow/base/cts/user/GroupCTSCmd.hpp"      // required to enforce cereal registration
#include "ecflow/base/cts/user/LoadDefsCmd.hpp"      // required to enforce cereal registration
#include "ecflow/base/cts/user/LogCmd.hpp"           // required to enforce cereal registration
#include "ecflow/base/cts/user/LogMessageCmd.hpp"    // required to enforce cereal registration
#include "ecflow/base/cts/user/MoveCmd.hpp"          // required to enforce cereal registration
#include "ecflow/base/cts/user/OrderNodeCmd.hpp"     // required to enforce cereal registration
#include "ecflow/base/cts/user/PathsCmd.hpp"         // required to enforce cereal registration
#include "ecflow/base/cts/user/PlugCmd.hpp"          // required to enforce cereal registration
#include "ecflow/base/cts/user/QueryCmd.hpp"         // required to enforce cereal registration
#include "ecflow/base/cts/user/ReplaceNodeCmd.hpp"   // required to enforce cereal registration
#include "ecflow/base/cts/user/RequeueNodeCmd.hpp"   // required to enforce cereal registration
#include "ecflow/base/cts/user/RunNodeCmd.hpp"       // required to enforce cereal registration
#include "ecflow/base/cts/user/ServerVersionCmd.hpp" // required to enforce cereal registration
#include "ecflow/base/cts/user/ShowCmd.hpp"          // required to enforce cereal registration
#include "ecflow/base/cts/user/ZombieCmd.hpp"        // required to enforce cereal registration
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/server/BaseServer.hpp"
#include "ecflow/server/ServerEnvironment.hpp"

using boost::asio::ip::tcp;

using namespace std;
using namespace ecf;

TcpBaseServer::TcpBaseServer(BaseServer* server, boost::asio::io_context& io, ServerEnvironment& serverEnv)
    : server_(server),
      io_(io),
      serverEnv_(serverEnv),
      acceptor_(io) {
    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    boost::asio::ip::tcp::endpoint endpoint(serverEnv.tcp_protocol(), serverEnv.port());
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen(); // address is use error, when it comes, bombs out here
}

void TcpBaseServer::handle_request() {
    // See what kind of message we got from the client
    if (serverEnv_.debug())
        std::cout << "   TcpBaseServer::handle_request  : client request " << inbound_request_ << endl;

    try {
        // Service the in bound request, handling the request will populate the outbound_response_
        // Note:: Handle request will first authenticate
        outbound_response_.set_cmd(inbound_request_.handleRequest(server_));
    }
    catch (exception& e) {
        outbound_response_.set_cmd(PreAllocatedReply::error_cmd(e.what()));
    }

    // Do any necessary clean up after inbound_request_ has run. i.e like re-claiming memory
    inbound_request_.cleanup();
}

void TcpBaseServer::handle_read_error(const boost::system::error_code& e) {
    // An error occurred.
    // o/ If client has been killed/disconnected/timed out
    //       TcpServer::handle_read : End of file
    //
    // o/ If a *new* client talks to an *old* server, with an unrecognised request/command i.e mixing 4/5 series
    //    we will see:
    //       Connection::handle_read_data .............
    //       TcpServer::handle_read : Invalid argument
    LogToCout toCoutAsWell;
    LogFlusher logFlusher;
    LOG(Log::ERR, "TcpBaseServer::handle_read_error: " << e.message());

    // *Reply* back to the client, This may fail in the client;
    std::pair<std::string, std::string> host_port = server_->hostPort();
    std::string msg                               = " ->Server:";
    msg += host_port.first;
    msg += "@";
    msg += host_port.second;
    msg += " (";
    msg += Version::raw();
    msg += ") replied with: ";
    msg += e.message();
    outbound_response_.set_cmd(PreAllocatedReply::error_cmd(msg));
}

void TcpBaseServer::handle_terminate_request() {
    // If asked to terminate we do it here rather than in handle_read.
    // So that we have responded to the client.
    // *HOWEVER* only do this if the request was successful.
    //           we do this by checking that the out bound response was ok
    //           i.e a read only user should not be allowed to terminate server.
    if (inbound_request_.terminateRequest()) {
        if (serverEnv_.debug())
            cout << "   <-- TcpBaseServer::handle_terminate_request  exiting server via terminate() port "
                 << serverEnv_.port() << endl;
        terminate();
    }
}

void TcpBaseServer::terminate() {
    // The server is terminated by cancelling all outstanding asynchronous
    // operations. Once all operations have finished the io_context::run() call  will exit.
    if (serverEnv_.debug())
        cout << "   Server::terminate(): posting call to Server::handle_terminate" << endl;

    // Post a call to the stop function so that Server::stop() is safe to call from any thread.
    boost::asio::post(io_, [this]() { handle_terminate(); });
}

void TcpBaseServer::handle_terminate() {
    // if (serverEnv_.debug()) cout << boost::this_thread::get_id() << "   Server::handle_terminate() : cancelling
    // checkpt and traverser timers, and signals" << endl;
    if (serverEnv_.debug())
        cout << "   Server::handle_terminate() : cancelling checkpt and traverser timers, and signals" << endl;

    server_->handle_terminate();

    acceptor_.close();

    // Stop the io_context object's event processing loop. Will cause run to return immediately
    io_.stop();
}
