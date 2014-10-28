/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Client
// Author      : Avi
// Revision    : $Revision: #33 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <fstream>
#include <iostream>
#include <sstream>
#include <assert.h>

#include <boost/bind.hpp>

#include "Client.hpp"
#include "StcCmd.hpp"

//#define DEBUG_CLIENT 1;

#ifdef DEBUG_CLIENT
#include <boost/date_time/posix_time/time_formatters.hpp>  // requires boost date and time lib, for to_simple_string
#endif

/// The timeout will typically happen when the server has died, but socket is still open
/// If we do not have a timeout, it will hang indefinitely

/// Constructor starts the asynchronous connect operation.
Client::Client( boost::asio::io_service& io_service,
				Cmd_ptr cmd_ptr,
				const std::string& host,
				const std::string& port,
				int timeout
			  )
: stopped_(false),host_( host ), port_( port ), connection_( io_service ),deadline_(io_service),timeout_(timeout)
{
	/// Avoid sending a NULL request to the server
	if (!cmd_ptr.get())  throw std::runtime_error("Client::Client: No request specified !");

	// The timeout can be set externally for testing, however when its not set the timeout is obtained from the command
	// Vary the timeout, according to the command, hence loading the definition has longer timeout than ping
	if (0 == timeout_) {
	   timeout_ = cmd_ptr->timeout();
	}

#ifdef DEBUG_CLIENT
   std::cout << "   Client::Client() timeout(" << timeout_ << ") " << host_ << ":" << port_ << " "; cmd_ptr->print(std::cout); std::cout << std::endl;
#endif

  	outbound_request_.set_cmd( cmd_ptr );

  	// Host name resolution is performed using a resolver, where host and service
  	// names(or ports) are looked up and converted into one or more end points
 	boost::asio::ip::tcp::resolver resolver( io_service );
	boost::asio::ip::tcp::resolver::query query( host_, port_ );
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve( query );

	// The list of end points obtained could contain both IPv4 and IPv6 end points,
	// so a program may try each of them until it finds one that works.
	// This keeps the Client program independent of a specific IP version.
	start(endpoint_iterator);
}

Client::~Client() {
#ifdef DEBUG_CLIENT
	std::cout << "   Client::~Client(): connection_.socket().is_open()=" << connection_.socket().is_open() << std::endl;
#endif
}

/// Private ==============================================================================

// This function terminates all the actors to shut down the connection. It
// may be called by the user of the client class, or by the class itself in
// response to graceful termination or an unrecoverable error.
void Client::start(boost::asio::ip::tcp::resolver::iterator endpoint_iter)
{
   // Start the connect actor.
   start_connect(endpoint_iter);

   // Start the deadline actor. You will note that we're not setting any
   // particular deadline here. Instead, the connect and input actors will
   // update the deadline prior to each asynchronous operation.
   deadline_.async_wait(boost::bind(&Client::check_deadline, this));
}

bool Client::start_connect(boost::asio::ip::tcp::resolver::iterator endpoint_iterator )
{
   if ( endpoint_iterator != boost::asio::ip::tcp::resolver::iterator() )
   {
#ifdef DEBUG_CLIENT
      std::cout << "   Client::start_connect: Trying " << endpoint_iterator->endpoint() << "..." << std::endl;
#endif

      // expires_from_now cancels any pending asynchronous waits, and returns the number of asynchronous waits that were cancelled.
      // If it returns 0 then you were too late and the wait handler has already been executed, or will soon be executed.
      // If it returns 1 then the wait handler was successfully cancelled.

      // Set a deadline for the connect operation.
      deadline_.expires_from_now(boost::posix_time::seconds(timeout_));

      boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
      connection_.socket().async_connect(
               endpoint,
               boost::bind(
                        &Client::handle_connect,
                        this,
                        boost::asio::placeholders::error,
                        endpoint_iterator ) );
   }
   else {
      // ran out of end points
      return false;
   }
   return true;
}

void Client::handle_connect(  const boost::system::error_code& e,
                        boost::asio::ip::tcp::resolver::iterator endpoint_iterator )
{
#ifdef DEBUG_CLIENT
   std::cout << "   Client::handle_connect stopped_=" << stopped_ << std::endl;
#endif

  if (stopped_)
      return;

  // The async_connect() function automatically opens the socket at the start
  // of the asynchronous operation. If the socket is closed at this time then
  // the timeout handler must have run first.
  if (!connection_.socket().is_open())
  {
#ifdef DEBUG_CLIENT
     std::cout << "   Client::handle_connect: *Connect timeout*:  Trying next end point" << std::endl;
#endif
     // Try the next available end point.
     if (!start_connect( ++endpoint_iterator)) {
        // Ran out of end points, An error occurred
        stop();
        std::stringstream ss;
        if (e) ss << "Client::handle_connect: Ran out of end points : connection error( " << e.message() << " ) for request( " << outbound_request_ << " ) on " << host_ << ":" << port_;
        else   ss << "Client::handle_connect: Ran out of end points : connection error for request( " << outbound_request_ << " ) on " << host_ << ":" << port_;
        throw std::runtime_error(ss.str());
     }
  }
  else if (e) {

#ifdef DEBUG_CLIENT
     std::cout << "   Client::handle_connect Connect error: " << e.message() << " . Trying next end point" << std::endl;
#endif

     // Some kind of error. We need to close the socket used in the previous connection attempt
     // before starting a new one.
     connection_.socket().close();

     // Try the next end point.
     if (!start_connect( ++endpoint_iterator)) {
        // Ran out of end points. An error occurred.
        stop();
        std::stringstream ss; ss << "Client::handle_connect: Ran out of end points: connection error( " << e.message() << " ) for request( " << outbound_request_ << " ) on " << host_ << ":" << port_;
        throw std::runtime_error(ss.str());
     }
  }
  else {
#ifdef DEBUG_CLIENT
     std::cout << "   Client::handle_connect **Successfully** established connection to the server: Sending Out bound request = " << outbound_request_ << std::endl;
#endif
     // **Successfully** established connection to the server
     // Start operation to *SEND* a request to the server
     start_write();
  }
}

void Client::start_write()
{
   // expires_from_now cancels any pending asynchronous waits, and returns the number of asynchronous waits that were cancelled.
   // If it returns 0 then you were too late and the wait handler has already been executed, or will soon be executed.
   // If it returns 1 then the wait handler was successfully cancelled.

   // Set a deadline for the write operation.
   deadline_.expires_from_now(boost::posix_time::seconds(timeout_));

   connection_.async_write(
                        outbound_request_,
                        boost::bind(
                                    &Client::handle_write,
                                    this,
                                    boost::asio::placeholders::error ) );
}

void Client::handle_write( const boost::system::error_code& e )
{
#ifdef DEBUG_CLIENT
      std::cout << "   Client::handle_write stopped_ = " << stopped_ << std::endl;
#endif
   if (stopped_)
       return;

	if ( !e ) {

#ifdef DEBUG_CLIENT
		std::cout << "   Client::handle_write OK: Check for server reply" << std::endl;
#endif
		// Check to see if the server was happy with our request.
		// If all is OK, the server may choose not to reply(cuts down on network traffic)
		// In which case handle_read will get a End of File error.
		start_read();
	}
	else {

		// An error occurred.
	   stop();

		std::stringstream ss;  ss << "Client::handle_write: error (" << e.message() << " ) for request( " << outbound_request_ << " ) on " << host_ << ":" <<  port_;
 		throw std::runtime_error(ss.str());
 	}

	// Nothing to do. The socket will be closed automatically when the last
	// reference to the connection object goes away.
}

void Client::start_read()
{
   // expires_from_now cancels any pending asynchronous waits, and returns the number of asynchronous waits that were cancelled.
   // If it returns 0 then you were too late and the wait handler has already been executed, or will soon be executed.
   // If it returns 1 then the wait handler was successfully cancelled.

   // Set a deadline for the read operation.
   deadline_.expires_from_now(boost::posix_time::seconds(timeout_));

   connection_.async_read(
                     inbound_response_,
                     boost::bind(
                                 &Client::handle_read,
                                 this,
                                 boost::asio::placeholders::error ) );
}


void Client::handle_read( const boost::system::error_code& e )
{
#ifdef DEBUG_CLIENT
      std::cout << "   Client::handle_read stopped_ = " << stopped_ << std::endl;
#endif
   if (stopped_)
       return;

   // close socket, & cancel timer.
   stop();

	if ( !e ) {
#ifdef DEBUG_CLIENT
		std::cout << "   Client::handle_read OK \n";
		std::cout << "      outbound_request_ = " << outbound_request_ << "\n";
		std::cout << "      inbound_response_ = " << inbound_response_ << "\n";
#endif
		/// ***********************************************************
		/// ClientInvoker will call back, to handle_server_response.
 		/// ***********************************************************
		// Successfully handled request
	}
	else {
	   //
		// A connection error occurred.
		// In cases where ( to cut down network traffic), the server does a shutdown/closes
	   // the socket without replying we will get End of File error.
	   //
		// i.e. client requests a response from the server, and it does not reply(or replies with shutdown/close)
	   //

	   // This code will handle  a no reply from the server & hence reduce network traffic
	   // Server has shutdown and closed the socket.
	   // See void Server::handle_read(...)
		if (e.value() == boost::asio::error::eof) {
			// Treat a  *no* reply as OK, so that handle_server_response() returns OK
#ifdef DEBUG_CLIENT
			std::cout << "   Client::handle_read: No reply from server: Treat as OK" << std::endl;
#endif
			inbound_response_.set_cmd( STC_Cmd_ptr(new StcCmd(StcCmd::OK)) );
			return;
 		}

		std::stringstream ss;
		ss << "Client::handle_read: connection error( " << e.message() << " ) for request( " << outbound_request_ << " ) on " << host_ << ":" << port_;
		throw std::runtime_error(ss.str());
	}

	// Since we are not starting a new operation the io_service will run out of
	// work to do and the Client will exit.
}

void Client::stop()
{
   stopped_ = true;
   connection_.socket().close();
   deadline_.cancel();
}

/// Handle completion of a write operation.
/// Handle completion of a read operation.
bool Client::handle_server_response( ServerReply& server_reply, bool debug ) const
{
	if (debug) std::cout << "   Client::handle_server_response" << std::endl;
	return inbound_response_.handle_server_response(server_reply, outbound_request_.get_cmd(), debug);
}

void Client::check_deadline()
{
#ifdef DEBUG_CLIENT
      std::cout << "   Client::check_deadline stopped_=" << stopped_
                << " expires(" << to_simple_string(deadline_.expires_at())
                << ") time now(" << to_simple_string(boost::asio::deadline_timer::traits_type::now()) << ")" << std::endl;
#endif
   if (stopped_)
      return;

   // Check whether the deadline has passed. We compare the deadline against
   // the current time since a new asynchronous operation may have moved the
   // deadline before this actor had a chance to run.
   if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now())
   {
#ifdef DEBUG_CLIENT
      std::cout << "   Client::check_deadline timed out" << std::endl;
#endif

      // The deadline has passed. The socket is closed so that any outstanding
      // asynchronous operations are cancelled.
      stop();

      std::stringstream ss;
      ss << "Client::check_deadline: timed out after " << timeout_ << " seconds for request( " << outbound_request_ << " ) on " << host_ << ":" << port_;
      throw std::runtime_error(ss.str());
   }

   // Put the actor back to sleep.
   deadline_.async_wait(boost::bind(&Client::check_deadline, this));
}
