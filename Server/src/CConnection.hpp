#ifndef CCONNECTION_HPP
#define CCONNECTION_HPP
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $
//
// Copyright 2009-2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description : ONLY used if ECFLOW_MT is defined
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "ClientToServerRequest.hpp"
#include "ServerToClientResponse.hpp"

class server;

/// Represents a single connection from a client.
class CConnection : public boost::enable_shared_from_this<CConnection>, private boost::noncopyable {
public:
   /// Construct a connection with the given io_service.
   explicit CConnection( boost::asio::io_service& io_service, server* );

   /// Get the socket associated with the connection.
   boost::asio::ip::tcp::socket& socket();

   /// Start the first asynchronous operation for the connection.
   void start();

private:
   /// Handle completion of a read operation.
   void handle_read( const boost::system::error_code& e, std::size_t bytes_transferred );

   void handle_read_data(const boost::system::error_code& e);
   void handle_read_header(const boost::system::error_code& e);
   void reply_back_to_client();

   /// Handle completion of a write operation.
   void handle_write( const boost::system::error_code& e );

private:
   /// Socket for the connection.
   boost::asio::ip::tcp::socket socket_;

   /// Strand to ensure the connection's handlers are not called concurrently.
   server* server_;

   /// The data, typically loaded once, and then sent to many clients
   ClientToServerRequest inbound_request_;     /// The incoming request.
   ServerToClientResponse outbound_response_;   /// The reply to be sent back to the client.


   std::string outbound_header_;        /// Holds an out-bound header.
   std::string outbound_data_;          /// Holds the out-bound data.
   enum { header_length = 8 };          /// The size of a fixed length header.
   char inbound_header_[header_length]; /// Holds an in-bound header.
   std::vector<char> inbound_data_;     /// Holds the in-bound data.
};

typedef boost::shared_ptr<CConnection> CConnection_ptr;

#endif
