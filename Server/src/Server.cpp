//============================================================================
// Name        : Server.cpp
// Author      : Avi
// Revision    : $Revision: #173 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//============================================================================

#include <iostream>
#include <boost/bind.hpp>

#include "Server.hpp"
#include "Log.hpp"
#include "ServerEnvironment.hpp"
#include "Version.hpp"

using boost::asio::ip::tcp;

using namespace std;
using namespace ecf;

Server::Server(boost::asio::io_service& io_service, ServerEnvironment& serverEnv ) : BaseServer(io_service,serverEnv)
{
   start_accept();
}

void Server::start_accept()
{
   if (serverEnv_.debug()) cout << "   Server::start_accept()" << endl;
   connection_ptr new_conn = std::make_shared<connection>( boost::ref(io_service_) );
   acceptor_.async_accept( new_conn->socket_ll(),
		                   [this,new_conn](const boost::system::error_code& e ) { handle_accept(e,new_conn); });
}

void Server::handle_accept( const boost::system::error_code& e, connection_ptr conn )
{
   if (serverEnv_.debug()) cout << "   Server::handle_accept" << endl;

   // Check whether the server was stopped by a signal before this completion
   // handler had a chance to run.
   if (!acceptor_.is_open()) {
      if (serverEnv_.debug()) cout << "   Server::handle_accept:  acceptor is closed, returning" << endl;
      return;
   }

   if ( !e ) {
      // Read and interpret message from the client
      // Successfully accepted a new connection. Determine what the
      // client sent to us. The connection::async_read() function will
      // automatically. serialise the inbound_request_ data structure for us.
      conn->async_read( inbound_request_,
                     boost::bind( &Server::handle_read, this,
                                boost::asio::placeholders::error,conn ) );
   }
   else {
      if (serverEnv_.debug()) cout << "   Server::handle_accept " << e.message() << endl;
      if (e != boost::asio::error::operation_aborted) {
         // An error occurred. Log it
         LogToCout toCoutAsWell;
         LogFlusher logFlusher;
         LOG(Log::ERR, "   Server::handle_accept error occurred  " <<  e.message());
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

void Server::handle_read(  const boost::system::error_code& e,connection_ptr conn )
{
   /// Handle completion of a write operation.
   // **********************************************************************************
   // This function *must* finish with write, otherwise it ends up being called recursively
   // ***********************************************************************************
   if ( !e ) {

      // See what kind of message we got from the client
      if (serverEnv_.debug()) std::cout << "   Server::handle_read : client request " << inbound_request_ << endl;

      try {
         // Service the in bound request, handling the request will populate the outbound_response_
         // Note:: Handle request will first authenticate
         outbound_response_.set_cmd( inbound_request_.handleRequest( this ) );
      }
      catch (exception& e) {
         outbound_response_.set_cmd( PreAllocatedReply::error_cmd( e.what()  ));
      }

      // Do any necessary clean up after inbound_request_ has run. i.e like re-claiming memory
      inbound_request_.cleanup();

      // Always *Reply* back to the client, Otherwise client will get EOF
      conn->async_write( outbound_response_,
                          boost::bind(&Server::handle_write,
                                    this,
                                    boost::asio::placeholders::error,
                                    conn ) );
   }
   else {
      // An error occurred.
      // o/ If client has been killed/disconnected/timed out
      //       Server::handle_read : End of file
      //
      // o/ If a *new* client talks to an *old* server, with an unrecognised request/command i.e mixing 4/5 series
      //    we will see:
      //       Connection::handle_read_data .............
      //       Server::handle_read : Invalid argument
      LogToCout toCoutAsWell;
      LogFlusher logFlusher;
      LOG(Log::ERR, "Server::handle_read: " <<  e.message());

      // *Reply* back to the client, This may fail in the client;
      std::pair<std::string,std::string> host_port = hostPort();
      std::string msg = " ->Server:"; msg += host_port.first; msg += "@"; msg += host_port.second; msg += " (";
      msg += Version::raw(); msg += ") replied with: "; msg += e.message();
      outbound_response_.set_cmd( PreAllocatedReply::error_cmd(msg));
      conn->async_write( outbound_response_,
                          boost::bind(&Server::handle_write,
                                    this,
                                    boost::asio::placeholders::error,
                                    conn ) );
   }
}

void Server::handle_write( const boost::system::error_code& e, connection_ptr conn )
{
   // Handle completion of a write operation.
   // Nothing to do. The socket will be closed automatically when the last
   // reference to the connection object goes away.
   if (serverEnv_.debug())
      cout << "   Server::handle_write: client request " << inbound_request_ << " replying with  " << outbound_response_ << endl;

   if (e) {
      LogFlusher logFlusher;
      ecf::LogToCout logToCout;
      std::stringstream ss; ss << "Server::handle_write: " << e.message() << " : for request " << inbound_request_;
      log(Log::ERR,ss.str());
      return;
   }

   // Do any necessary clean up after outbound_response_  has run. i.e like re-claiming memory
   outbound_response_.cleanup();

   (void)shutdown_socket(conn,"Server::handle_write:");

   // If asked to terminate we do it here rather than in handle_read.
   // So that we have responded to the client.
   // *HOWEVER* only do this if the request was successful.
   //           we do this by checking that the out bound response was ok
   //           i.e a read only user should not be allowed to terminate server.
   if (inbound_request_.terminateRequest()) {
      if (serverEnv_.debug()) cout << "   <--Server::handle_write exiting server via terminate() port " << serverEnv_.port() << endl;
      terminate();
   }
}

bool Server::shutdown_socket(connection_ptr conn, const std::string& msg) const
{
   // For portable behaviour with respect to graceful closure of a connected socket,
   // call shutdown() before closing the socket.
   //
   //    conn->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both)
   // This *CAN* throw an error if the client side socket is not connected. client may have been killed *OR* timed out
   // i.e "shutdown: Transport endpoint is not connected"
   //
   // Since this can happen, instead of throwing, we use non-throwing version & just report it
   boost::system::error_code ec;
   conn->socket_ll().shutdown(boost::asio::ip::tcp::socket::shutdown_both,ec);
   if (ec) {
      ecf::LogToCout logToCout;
      LogFlusher logFlusher;
      std::stringstream ss; ss << msg << " socket shutdown both failed: " << ec.message() << " : for request " << inbound_request_;
      log(Log::ERR,ss.str());
      return false;
   }
   return true;
}

