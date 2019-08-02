#ifndef TCP_BASE_SERVER_HPP_
#define TCP_BASE_SERVER_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Server.cpp
// Author      : Avi
// Revision    : $Revision: #62 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/asio.hpp>
#include "ServerEnvironment.hpp"
#include "ClientToServerRequest.hpp"
#include "ServerToClientResponse.hpp"
#include "Log.hpp"

class BaseServer;

class TcpBaseServer {
public:
   /// Constructor opens the acceptor and starts waiting for the first incoming connection.
   explicit TcpBaseServer(BaseServer*,boost::asio::io_service& io_service,ServerEnvironment&);
   ~TcpBaseServer() {}

   void handle_request();
   void handle_read_error(  const boost::system::error_code& e );

   /// Terminate the server gracefully. Need to cancel all timers, close all sockets
   /// Server will hang if there are any pending async handlers
   void handle_terminate_request();
   void terminate();
   void handle_terminate();


   template<typename T>
   bool shutdown_socket(T conn, const std::string& msg) const
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
         ecf::LogFlusher logFlusher;
         std::stringstream ss; ss << msg << " socket shutdown both failed: " << ec.message() << " : for request " << inbound_request_;
         ecf::log(ecf::Log::ERR,ss.str());
         return false;
      }
      return true;
   }

protected:
   BaseServer* server_;
   boost::asio::io_service& io_service_;
   ServerEnvironment& serverEnv_;

   /// The acceptor object used to accept incoming socket connections.
   boost::asio::ip::tcp::acceptor acceptor_;

   /// The data, typically loaded once, and then sent to many clients
   ClientToServerRequest  inbound_request_;
   ServerToClientResponse outbound_response_;
};

#endif
