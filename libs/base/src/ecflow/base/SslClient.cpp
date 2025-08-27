/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/SslClient.hpp"

#include <stdexcept>

#include "ecflow/base/stc/ErrorCmd.hpp"
#include "ecflow/base/stc/StcCmd.hpp"

// #define DEBUG_CLIENT 1;

#ifdef DEBUG_CLIENT
    #include <iostream>
#endif

/// The timeout will typically happen when the server has died, but socket is still open
/// If we do not have a timeout, it will hang indefinitely

/// Constructor starts the asynchronous connect operation.
SslClient::SslClient(boost::asio::io_context& io,
                     boost::asio::ssl::context& context,
                     Cmd_ptr cmd_ptr,
                     const std::string& host,
                     const std::string& port,
                     int timeout)
    : stopped_(false),
      host_(host),
      port_(port),
      connection_(io, context),
      deadline_(io),
      timeout_(timeout) {
    /// Avoid sending a NULL request to the server
    if (!cmd_ptr.get())
        throw std::runtime_error("SslClient::SslClient: No request specified !");

    // The timeout can be set externally for testing.
    // When the timeout is not set it is obtained from the command.
    // The timeout is defined per command -- this allows, for example, loading the definition to have a longer timeout
    // than ping.
    if (0 == timeout_) {
        timeout_ = cmd_ptr->timeout();
    }

#ifdef DEBUG_CLIENT
    std::cout << "   SslClient::SslClient() timeout(" << timeout_ << ") " << host_ << ":" << port_ << " ";
    std::string output;
    cmd_ptr->print(output);
    std::cout << output << std::endl;
#endif

    outbound_request_.set_cmd(cmd_ptr);

    // Host name resolution is performed using a resolver, where host and service
    // names(or ports) are looked up and converted into one or more end points
    auto resolver           = resolver_t(io);
    auto results            = resolver.resolve(host_, port_);
    auto endpoints_iterator = results.begin();

    // The list of end points obtained could contain both IPv4 and IPv6 end points,
    // so a program may try each of them until it finds one that works.
    // This keeps the SslClient program independent of a specific IP version.
    start(endpoints_iterator);
}

SslClient::~SslClient() {
#ifdef DEBUG_CLIENT
    std::cout << "   SslClient::~SslClient(): connection_.socket().is_open()=" << connection_.socket_ll().is_open()
              << std::endl;
#endif
}

/// Private ==============================================================================

// This function terminates all the actors to shut down the connection. It
// may be called by the user of the client class, or by the class itself in
// response to graceful termination or an unrecoverable error.
void SslClient::start(endpoints_iterator_t endpoints_iterator) {
    // Start the connect actor.
    start_connect(endpoints_iterator);

    // Start the deadline actor. You will note that we're not setting any
    // particular deadline here. Instead, the connect and input actors will
    // update the deadline prior to each asynchronous operation.
    deadline_.async_wait([this](const boost::system::error_code&) { check_deadline(); });
}

bool SslClient::start_connect(endpoints_iterator_t endpoints_iterator) {
    if (endpoints_iterator != endpoints_iterator_t()) {
#ifdef DEBUG_CLIENT
        std::cout << "   SslClient::start_connect: Trying " << endpoints_iterator->endpoint() << "..." << std::endl;
#endif

        // expires_from_now cancels any pending asynchronous waits, and returns the number of asynchronous waits that
        // were cancelled. If it returns 0 then you were too late and the wait handler has already been executed, or
        // will soon be executed. If it returns 1 then the wait handler was successfully cancelled.

        // Set a deadline for the connect operation.
        deadline_.expires_from_now(boost::posix_time::seconds(timeout_));

        boost::asio::ip::tcp::endpoint endpoint = *endpoints_iterator;
        connection_.socket_ll().async_connect(endpoint,
                                              [this, endpoints_iterator](const boost::system::error_code& error) {
                                                  this->handle_connect(error, endpoints_iterator);
                                              });
    }
    else {
        // ran out of end points
        return false;
    }
    return true;
}

void SslClient::handle_connect(const boost::system::error_code& e, endpoints_iterator_t endpoints_iterator) {
#ifdef DEBUG_CLIENT
    std::cout << "   SslClient::handle_connect stopped_=" << stopped_ << std::endl;
#endif

    if (stopped_)
        return;

    // The async_connect() function automatically opens the socket at the start
    // of the asynchronous operation. If the socket is closed at this time then
    // the timeout handler must have run first.
    if (!connection_.socket_ll().is_open()) {
#ifdef DEBUG_CLIENT
        std::cout << "   SslClient::handle_connect: *Connect timeout*:  Trying next end point" << std::endl;
#endif
        // Try the next available end point.
        if (!start_connect(++endpoints_iterator)) {
            // Ran out of end points, An error occurred
            stop();
            std::stringstream ss;
            if (e)
                ss << "SslClient::handle_connect: Ran out of end points : connection error( " << e.message()
                   << " ) for request( " << outbound_request_ << " ) on " << host_ << ":" << port_;
            else
                ss << "SslClient::handle_connect: Ran out of end points : connection error for request( "
                   << outbound_request_ << " ) on " << host_ << ":" << port_;
            throw std::runtime_error(ss.str());
        }
    }
    else if (e) {

#ifdef DEBUG_CLIENT
        std::cout << "   SslClient::handle_connect Connect error: " << e.message() << " . Trying next end point"
                  << std::endl;
#endif

        // Some kind of error. We need to close the socket used in the previous connection attempt
        // before starting a new one.
        connection_.socket_ll().close();

        // Try the next end point.
        if (!start_connect(++endpoints_iterator)) {
            // Ran out of end points. An error occurred.
            stop();
            std::stringstream ss;
            ss << "SslClient::handle_connect: Ran out of end points: connection error( " << e.message()
               << " ) for request( " << outbound_request_ << " ) on " << host_ << ":" << port_;
            throw std::runtime_error(ss.str());
        }
    }
    else {
#ifdef DEBUG_CLIENT
        std::cout << "   SslClient::handle_connect **Successfully** established connection to the server: Sending Out "
                     "bound request = "
                  << outbound_request_ << std::endl;
#endif

        // **Successfully** established connection to the server
        start_handshake();
    }
}

void SslClient::start_handshake() {
#ifdef DEBUG_CLIENT
    std::cout << "   SslClient::start_handshake " << outbound_request_ << std::endl;
#endif
    // expires_from_now cancels any pending asynchronous waits, and returns the number of asynchronous waits that were
    // cancelled. If it returns 0 then you were too late and the wait handler has already been executed, or will soon be
    // executed. If it returns 1 then the wait handler was successfully cancelled. Set a deadline for the write
    // operation.
    deadline_.expires_from_now(boost::posix_time::seconds(timeout_));

    connection_.socket().async_handshake(boost::asio::ssl::stream_base::client,
                                         [this](const boost::system::error_code& e) { handle_handshake(e); });
}

void SslClient::handle_handshake(const boost::system::error_code& e) {
#ifdef DEBUG_CLIENT
    std::cout << "   SslClient::handle_handshake " << std::endl;
#endif
    if (!e) {
        start_write();
    }
    else {
        // An error occurred.
        stop();

        std::stringstream ss;
        ss << "SslClient::handle_handshake: error(" << e.message() << ") on " << host_ << ":" << port_
           << " possibly non-ssl server";
        inbound_response_.set_cmd(std::make_shared<ErrorCmd>(ss.str()));
    }
}

void SslClient::start_write() {
    // expires_from_now cancels any pending asynchronous waits, and returns the number of asynchronous waits that were
    // cancelled. If it returns 0 then you were too late and the wait handler has already been executed, or will soon be
    // executed. If it returns 1 then the wait handler was successfully cancelled.

    // Set a deadline for the write operation.
    deadline_.expires_from_now(boost::posix_time::seconds(timeout_));

    connection_.async_write(outbound_request_,
                            [this](const boost::system::error_code& error) { this->handle_write(error); });
}

void SslClient::handle_write(const boost::system::error_code& e) {
#ifdef DEBUG_CLIENT
    std::cout << "   SslClient::handle_write stopped_ = " << stopped_ << std::endl;
#endif
    if (stopped_)
        return;

    if (!e) {

#ifdef DEBUG_CLIENT
        std::cout << "   SslClient::handle_write OK: Check for server reply" << std::endl;
#endif
        // Check to see if the server was happy with our request.
        // If all is OK, the server may choose not to reply(cuts down on network traffic)
        // In which case handle_read will get an End of File error.
        start_read();
    }
    else {

        // An error occurred.
        stop();

        std::stringstream ss;
        ss << "SslClient::handle_write: error (" << e.message() << " ) for request( " << outbound_request_ << " ) on "
           << host_ << ":" << port_;
        throw std::runtime_error(ss.str());
    }

    // Nothing to do. The socket will be closed automatically when the last
    // reference to the connection object goes away.
}

void SslClient::start_read() {
    // expires_from_now cancels any pending asynchronous waits, and returns the number of asynchronous waits that were
    // cancelled. If it returns 0 then you were too late and the wait handler has already been executed, or will soon be
    // executed. If it returns 1 then the wait handler was successfully cancelled.

    // Set a deadline for the read operation.
    deadline_.expires_from_now(boost::posix_time::seconds(timeout_));

    connection_.async_read(inbound_response_,
                           [this](const boost::system::error_code& error) { this->handle_read(error); });
}

void SslClient::handle_read(const boost::system::error_code& e) {
#ifdef DEBUG_CLIENT
    std::cout << "   SslClient::handle_read stopped_ = " << stopped_ << std::endl;
#endif
    if (stopped_)
        return;

    // close socket, & cancel timer.
    stop();

    if (!e) {
#ifdef DEBUG_CLIENT
        std::cout << "   SslClient::handle_read OK \n";
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
        // A connection error occurred. EOF
        //   - In cases where ( to cut down network traffic),
        //     the server does a shutdown/closes the socket without replying we will get End of File error.
        //   - if we have a NONSSL client talking to SSL server, the SSL server will not reply, we will get End of File
        //   error.
        //
        // i.e. client requests a response from the server, and it does not reply(or replies with shutdown/close)
        // In both cases we will treat as an error

        if (e.value() == boost::asio::error::eof) {
#ifdef DEBUG_CLIENT
            std::cout << "   Client::handle_read: End of File (server did not reply or mixing ssl and non-ssl)"
                      << std::endl;
#endif
            inbound_response_.set_cmd(std::make_shared<StcCmd>(StcCmd::END_OF_FILE));
            return;
        }

        // If we get back Invalid argument it means server could *NOT* decode client message, and, the client
        // could not decode server reply
        //   - Could be boost version of client/server are incompatible
        //   - 4 series ecflow cannot talk to 5 series ecflow or vice versa
        // We treat this specially
        if (e.value() == boost::asio::error::invalid_argument) {
#ifdef DEBUG_CLIENT
            std::cout
                << "   Client::handle_read: Server replied with invalid argument (i.e could not decode client message) "
                << std::endl;
#endif
            inbound_response_.set_cmd(std::make_shared<StcCmd>(StcCmd::INVALID_ARGUMENT));
            return;
        }

        std::stringstream ss;
        ss << "Client::handle_read: connection error( " << e.message() << " ) for request( " << outbound_request_
           << " ) on " << host_ << ":" << port_;
        throw std::runtime_error(ss.str());
    }

    // Since we are not starting a new operation the io_context will run out of
    // work to do and the Client will exit.
}

void SslClient::stop() {
    stopped_ = true;
    connection_.socket_ll().close();
    deadline_.cancel();
}

/// Handle completion of a write operation.
/// Handle completion of a read operation.
bool SslClient::handle_server_response(ServerReply& server_reply, bool debug) const {
    if (debug)
        std::cout << "  SslClient::handle_server_response" << std::endl;
    server_reply.set_host_port(host_, port_); // client context, needed by some commands, i.e. SServerLoadCmd
    return inbound_response_.handle_server_response(server_reply, outbound_request_.get_cmd(), debug);
}

void SslClient::check_deadline() {
#ifdef DEBUG_CLIENT
    std::cout << "   SslClient::check_deadline stopped_=" << stopped_ << " expires("
              << to_simple_string(deadline_.expires_at()) << ") time now("
              << to_simple_string(boost::asio::deadline_timer::traits_type::now()) << ")" << std::endl;
#endif
    if (stopped_)
        return;

    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
#ifdef DEBUG_CLIENT
        std::cout << "   SslClient::check_deadline timed out" << std::endl;
#endif

        // The deadline has passed. The socket is closed so that any outstanding
        // asynchronous operations are cancelled.
        stop();

        std::stringstream ss;
        ss << "SslClient::check_deadline: timed out after " << timeout_ << " seconds for request( " << outbound_request_
           << " ) on " << host_ << ":" << port_;
        throw std::runtime_error(ss.str());
    }

    // Put the actor back to sleep.
    deadline_.async_wait([this](const boost::system::error_code&) { check_deadline(); });
}
