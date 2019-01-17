/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description : ONLY used if ECFLOW_MT is defined
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#ifdef ECFLOW_MT
#if defined(HPUX)
#include <sys/select.h> // hp-ux uses pselect
#endif

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

#include "CConnection.hpp"
#include "Log.hpp"
#include <vector>

#include "Log.hpp"
#include "Serialization.hpp"
#include "Server.hpp"

using namespace std;
using namespace ecf;

CConnection::CConnection(boost::asio::io_service& io_service,server* server)
  : socket_(io_service),
    server_(server)
{
}

boost::asio::ip::tcp::socket& CConnection::socket()
{
  return socket_;
}

void CConnection::start()
{
   //cout << boost::this_thread::get_id() << " CConnection::start()\n";
   boost::asio::async_read(socket_, boost::asio::buffer(inbound_header_),
                           server_->strand_.wrap(
                                    boost::bind(&CConnection::handle_read_header,
                                                shared_from_this(),
                                                boost::asio::placeholders::error)));
}

void CConnection::handle_read_header(const boost::system::error_code& e)
{
   //cout << boost::this_thread::get_id() << " CConnection::handle_read_header\n";
   if (e) {
      LogToCout toCoutAsWell;
      LOG(Log::ERR, "   CConnection::handle_read_header error occurred  " <<  e.message());
   }
   else {
      // Determine the length of the serialized data.
      std::istringstream is(std::string(inbound_header_, header_length));
      std::size_t inbound_data_size = 0;
      if (!(is >> std::hex >> inbound_data_size)) {

         // Header doesn't seem to be valid. Inform the caller.
         boost::system::error_code error(boost::asio::error::invalid_argument);
         LogToCout toCoutAsWell;
         LOG(Log::ERR, "   CConnection::handle_read_header error occurred  " <<  e.message());
         return;
      }

      // Start an asynchronous call to receive the data.
      inbound_data_.resize(inbound_data_size);
      boost::asio::async_read(socket_,boost::asio::buffer(inbound_data_),
                              server_->strand_.wrap(
                                       boost::bind(&CConnection::handle_read_data,
                                                   shared_from_this(),
                                                   boost::asio::placeholders::error)));
   }
}

void CConnection::handle_read_data(const boost::system::error_code& e)
{
   //cout << boost::this_thread::get_id() << " CConnection::handle_read_data\n";
   if (e) {
      LogToCout toCoutAsWell;
      LOG(Log::ERR, "   CConnection::handle_read_data error occurred  " << e.message());
      return;
   }

   // Extract the data structure from the data just received.
   try {
      std::string archive_data(&inbound_data_[0], inbound_data_.size());
      ecf::restore_from_string(archive_data,inbound_request_);
   }
   catch (const boost::archive::archive_exception& ae ) {
      // Unable to decode data.
      ecf::LogToCout logToCout;
      LOG(ecf::Log::ERR,"CConnection::handle_read_data boost::archive::archive_exception " << ae.what());
      return;
   }
   catch (std::exception& ) {
      // Unable to decode data.
      ecf::LogToCout logToCout;
      LOG(ecf::Log::ERR,"CConnection::handle_read_data Unable to decode data");
      return;
   }


   // See what kind of message we got from the client
   // std::cout << boost::this_thread::get_id() << "   server::handle_read : client request " << inbound_request_ << "\n";
   try {
      // Service the in bound request, handling the request will populate the outbound_response_
      // Note:: Handle request will first authenticate
      outbound_response_.set_cmd( inbound_request_.handleRequest( server_ ) );
   }
   catch (exception& ex) {
      outbound_response_.set_cmd( PreAllocatedReply::error_cmd( ex.what()  ));
   }

   // To improve performance, when the reply, is OK, don't bother replying back to the client
   // The client will receive a EOF, and perceive this as OK.
   // Need to specifically *ignore* for terminate, otherwise server will not shutdown cleanly
   if (!inbound_request_.terminateRequest() && outbound_response_.get_cmd()->isOkCmd()) {
      // cleanly close down the connection
      socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
      socket_.close();
      return;
   }

   reply_back_to_client();
}

void CConnection::reply_back_to_client()
{
#ifdef DEBUG_CONNECTION
      std::cout << "CConnection::reply_back_to_client, Serialise the data first so we know how large it is\n";
#endif
      // Serialise the data first so we know how large it is.
      try {
         ecf::save_as_string(outbound_data_,outbound_response_);
      } catch (const boost::archive::archive_exception& ae ) {
         // Unable to decode data. Something went wrong, inform the caller.
         ecf::LogToCout logToCout;
         LOG(ecf::Log::ERR,"Connection::async_write boost::archive::archive_exception " << ae.what());
         return;
      }

      // Format the header.
      std::ostringstream header_stream;
      header_stream << std::setw(header_length) << std::hex << outbound_data_.size();
      if (!header_stream || header_stream.str().size() != header_length) {
         // Something went wrong, inform the caller.
         ecf::LogToCout logToCout;
         LOG(ecf::Log::ERR,"CConnection::reply_back_to_client: could not format header");
         return;
      }
      outbound_header_ = header_stream.str();


#ifdef DEBUG_CONNECTION
      std::cout << "Connection::async_write Write the serialized data to the socket. \n";
#endif
      // Write the serialized data to the socket. We use "gather-write" to send
      // both the header and the data in a single write operation.
      std::vector<boost::asio::const_buffer> buffers; buffers.reserve(2);
      buffers.push_back(boost::asio::buffer(outbound_header_));
      buffers.push_back(boost::asio::buffer(outbound_data_));
      boost::asio::async_write(socket_, buffers,
                               server_->strand_.wrap(
                                        boost::bind(&CConnection::handle_write, shared_from_this(),
                                                    boost::asio::placeholders::error)));

#ifdef DEBUG_CONNECTION
      std::cout << "Connection::async_write END \n";
#endif

  // If an error occurs then no new asynchronous operations are started. This
  // means that all shared_ptr references to the CConnection object will
  // disappear and the object will be destroyed automatically after this
  // handler returns. The CConnection class's destructor closes the socket.
}

void CConnection::handle_write(const boost::system::error_code& e)
{
  // Handle completion of a write operation.
  // Nothing to do. The socket will be closed automatically when the last
  // reference to the connection object goes away.
  //cout << boost::this_thread::get_id() << "   server::handle_write: client request " << inbound_request_ << " replying with  " << outbound_response_ << "\n";

  if (e)
  {
     ecf::LogToCout logToCout;
     LOG(ecf::Log::ERR,"Connection::handle_write: " << e.message());

     // No new asynchronous operations are started. This means that all shared_ptr
     // references to the CConnection object will disappear and the object will be
     // destroyed automatically after this handler returns. The CConnection class's
     // destructor closes the socket.
     return;
   }


  // Initiate graceful connection closure.
  // For portable behaviour with respect to graceful closure of a connected socket, call shutdown() before closing the socket.
  // This *CAN* throw an error if the client side socket is not connected. client may have been killed.
  // i.e "shutdown: Transport endpoint is not connected"
  boost::system::error_code ec;
  socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
  if (ec) {
     ecf::LogToCout logToCout;
     LOG(Log::ERR,"server::handle_write: socket shutdown both failed: " << ec.message());
  }


  // If asked to terminate we do it here rather than in handle_read.
  // So that we have responded to the client.
  // *HOWEVER* only do this if the request was successful.
  //           we do this by checking that the out bound response was ok
  //           i.e a read only user should not be allowed to terminate server.
  if (inbound_request_.terminateRequest() && outbound_response_.get_cmd()->isOkCmd()) {
     // cout << "   <--server::handle_write exiting server via terminate() port " << endl;

     server_->terminate();
  }

  // No new asynchronous operations are started. This means that all shared_ptr
  // references to the CConnection object will disappear and the object will be
  // destroyed automatically after this handler returns. The CConnection class's
  // destructor closes the socket.
}
#endif
