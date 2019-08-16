#ifndef SSL_TCP_SERVER_HPP_
#define SSL_TCP_SERVER_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #62 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description : ECFLOW Server. Based on ASIO
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "ssl_connection.hpp"
#include "TcpBaseServer.hpp"
class SslServer;

class SslTcpServer : public TcpBaseServer {
public:
   /// Constructor opens the acceptor and starts waiting for the first incoming connection.
   explicit SslTcpServer(SslServer*,boost::asio::io_service& io_service,ServerEnvironment&);
   ~SslTcpServer() {}

private:
   /// ssl and connection functions
   void handle_handshake(const boost::system::error_code& error,ssl_connection_ptr conn);

   /// Handle completion of a accept operation.
   void handle_accept(const boost::system::error_code& e, ssl_connection_ptr conn);

   /// Handle completion of a write operation.
   void handle_write(const boost::system::error_code& e, ssl_connection_ptr conn);

   /// Handle completion of a read operation.
   void handle_read(const boost::system::error_code& e, ssl_connection_ptr conn);

   void start_accept();
};

#endif
