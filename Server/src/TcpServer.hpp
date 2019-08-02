#ifndef TCP_SERVER_HPP_
#define TCP_SERVER_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : TcpServer.cpp
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

#include "TcpBaseServer.hpp"
#include "Connection.hpp"
class Server;

class TcpServer : public TcpBaseServer {
public:
   /// Constructor opens the acceptor and starts waiting for the first incoming connection.
   explicit TcpServer(Server*,boost::asio::io_service& io_service,ServerEnvironment&);
   ~TcpServer() {}

private:
   /// Handle completion of a accept operation.
   void handle_accept(const boost::system::error_code& e, connection_ptr conn);

   /// Handle completion of a write operation.
   void handle_write(const boost::system::error_code& e, connection_ptr conn);

   /// Handle completion of a read operation.
   void handle_read(const boost::system::error_code& e, connection_ptr conn);

   void start_accept();
};

#endif
