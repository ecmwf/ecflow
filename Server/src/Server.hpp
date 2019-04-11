#ifndef SERVER_HPP_
#define SERVER_HPP_

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

#include "BaseServer.hpp"
#include "Connection.hpp"  // Must come before boost/serialisation headers.

class Server : public BaseServer {
public:
   /// Constructor opens the acceptor and starts waiting for the first incoming connection.
   explicit Server(ServerEnvironment&);
   ~Server() override {}

private:

   /// Handle completion of a accept operation.
   void handle_accept(const boost::system::error_code& e, connection_ptr conn);

   /// Handle completion of a write operation.
   void handle_write(const boost::system::error_code& e, connection_ptr conn);

   /// Handle completion of a read operation.
   void handle_read(const boost::system::error_code& e, connection_ptr conn);

   void start_accept();
   bool shutdown_socket(connection_ptr conn, const std::string& msg) const;
};

#endif
