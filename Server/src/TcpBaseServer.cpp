/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : TcpBaseServer.cpp
// Author      : Avi
// Revision    : $Revision: #173 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iostream>

#include "TcpBaseServer.hpp"
#include "BaseServer.hpp"
#include "Version.hpp"

using boost::asio::ip::tcp;

using namespace std;
using namespace ecf;

TcpBaseServer::TcpBaseServer(BaseServer* server,boost::asio::io_service& io_service, ServerEnvironment& serverEnv )
:  server_(server),io_service_(io_service), serverEnv_(serverEnv),
   acceptor_(io_service)
{
   // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
   boost::asio::ip::tcp::endpoint endpoint(serverEnv.tcp_protocol(), serverEnv.port());
   acceptor_.open(endpoint.protocol());
   acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
   acceptor_.bind(endpoint);
   acceptor_.listen();   // address is use error, when it comes, bombs out here
}

void TcpBaseServer::handle_request()
{
   // See what kind of message we got from the client
   if (serverEnv_.debug()) std::cout << "   TcpBaseServer::handle_request  : client request " << inbound_request_ << endl;

   try {
      // Service the in bound request, handling the request will populate the outbound_response_
      // Note:: Handle request will first authenticate
      outbound_response_.set_cmd( inbound_request_.handleRequest( server_ ) );
   }
   catch (exception& e) {
      outbound_response_.set_cmd( PreAllocatedReply::error_cmd( e.what()  ));
   }

   // Do any necessary clean up after inbound_request_ has run. i.e like re-claiming memory
   inbound_request_.cleanup();
}


void TcpBaseServer::handle_read_error( const boost::system::error_code& e )
{
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
   std::pair<std::string,std::string> host_port = server_->hostPort();
   std::string msg = " ->Server:"; msg += host_port.first; msg += "@"; msg += host_port.second; msg += " (";
   msg += Version::raw(); msg += ") replied with: "; msg += e.message();
   outbound_response_.set_cmd( PreAllocatedReply::error_cmd(msg));
}


void TcpBaseServer::handle_terminate_request()
{
   // If asked to terminate we do it here rather than in handle_read.
   // So that we have responded to the client.
   // *HOWEVER* only do this if the request was successful.
   //           we do this by checking that the out bound response was ok
   //           i.e a read only user should not be allowed to terminate server.
   if (inbound_request_.terminateRequest()) {
      if (serverEnv_.debug()) cout << "   <-- TcpBaseServer::handle_terminate_request  exiting server via terminate() port " << serverEnv_.port() << endl;
      terminate();
   }
}

void TcpBaseServer::terminate()
{
   // The server is terminated by cancelling all outstanding asynchronous
   // operations. Once all operations have finished the io_service::run() call  will exit.
   if (serverEnv_.debug()) cout << "   Server::terminate(): posting call to Server::handle_terminate" << endl;

   // Post a call to the stop function so that Server::stop() is safe to call from any thread.
   io_service_.post( [this]() {handle_terminate();} ); // boost::bind(&Server::handle_terminate, this));
}

void TcpBaseServer::handle_terminate()
{
   // if (serverEnv_.debug()) cout << boost::this_thread::get_id() << "   Server::handle_terminate() : cancelling checkpt and traverser timers, and signals" << endl;
   if (serverEnv_.debug()) cout << "   Server::handle_terminate() : cancelling checkpt and traverser timers, and signals" << endl;

   server_->handle_terminate();

   acceptor_.close();

   // Stop the io_service object's event processing loop. Will cause run to return immediately
   io_service_.stop();
}
