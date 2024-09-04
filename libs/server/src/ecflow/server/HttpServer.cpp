/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/server/HttpServer.hpp"

#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/server/ServerEnvironment.hpp"

inline void log_error(char const* where, boost::beast::error_code ec) {
    using namespace ecf;
    LOG(Log::ERR, where << ": " << ec.message());
}

#if defined(BOOST_VERSION) && BOOST_VERSION > 108100

// Return a response for the given request.
//
// The concrete type of the response message (which depends on the
// request), is type-erased in message_generator.
template <class Body, class Allocator>
boost::beast::http::message_generator
handle_request(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>&& request,
               BaseServer* server) {
    // Returns a bad request response
    auto const bad_request = [&request](boost::beast::string_view why) {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::bad_request,
                                                                          request.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(request.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if (request.method() != boost::beast::http::verb::post) {
        return bad_request("Unknown HTTP-method");
    }

    // Buffers to hold request/response
    ClientToServerRequest inbound_request;
    ServerToClientResponse outbound_response;

    // 1) unserialize request body into inbound_request
    ecf::restore_from_string(request.body(), inbound_request);

    // 2) handle request, as per TcpBaseServer::handle_request()
    {
        // See what kind of message we got from the client
        //        if (serverEnv_.debug()) {
        //            std::cout << "   HTTPServer::handle_request  : client request " << inbound_request << std::endl;
        //        }

        try {
            // Service the in bound request, handling the request will populate the outbound_response_
            // Note:: Handle request will first authenticate
            outbound_response.set_cmd(inbound_request.handleRequest(server));
        }
        catch (std::exception& e) {
            outbound_response.set_cmd(PreAllocatedReply::error_cmd(e.what()));
        }

        // Do any necessary clean up after inbound_request_ has run. i.e like re-claiming memory
        inbound_request.cleanup();
    }

    // 3) serialize outbound_response into response body
    std::string outbound;
    ecf::save_as_string(outbound, outbound_response);

    outbound_response.cleanup();

    // 4) ship response
    boost::beast::http::string_body::value_type body = outbound;

    if (inbound_request.terminateRequest()) {
        //        if (serverEnv_.debug()) {
        //            std::cout << "   <-- HttpServer::handle_terminate_request  exiting server via terminate() port "
        //                      << serverEnv_.port() << std::endl;
        //        }
        // TODO: Terminate the server!
    }

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to POST request
    boost::beast::http::response<boost::beast::http::string_body> response{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(boost::beast::http::status::ok, request.version())};
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, "application/json");
    response.content_length(size);
    response.keep_alive(request.keep_alive());
    return response;
}

// Handles an HTTP server connection
class HttpSession : public std::enable_shared_from_this<HttpSession> {
    boost::beast::tcp_stream stream_;
    boost::beast::flat_buffer buffer_;
    boost::beast::http::request<boost::beast::http::string_body> request_;
    BaseServer* server_;

public:
    // Take ownership of the stream
    HttpSession(boost::asio::ip::tcp::socket&& socket, BaseServer* server)
        : stream_(std::move(socket)),
          server_{server} {}

    // Start the asynchronous operation
    void run() {
        auto self = shared_from_this();
        boost::asio::dispatch(stream_.get_executor(), [self]() { self->do_read(); });
    }

    void do_read() {
        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        request_ = {};

        // Set the timeout.
        stream_.expires_after(std::chrono::seconds(30));

        // Read a request
        auto self = shared_from_this();
        boost::beast::http::async_read(
            stream_, buffer_, request_, [self](boost::beast::error_code ec, std::size_t bytes_transferred) {
                self->on_read(ec, bytes_transferred);
            });
    }

    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if (ec == boost::beast::http::error::end_of_stream) {
            return do_close();
        }

        if (ec) {
            return log_error("HttpSession::on_read", ec);
        }

        // Send the response
        send_response(handle_request(std::move(request_), server_));
    }

    void send_response(boost::beast::http::message_generator&& msg) {
        bool keep_alive = msg.keep_alive();

        // Write the response
        auto self = shared_from_this();
        boost::beast::async_write(
            stream_, std::move(msg), [self, keep_alive](boost::beast::error_code ec, std::size_t bytes_transferred) {
                self->on_write(keep_alive, ec, bytes_transferred);
            });
    }

    void on_write(bool keep_alive, boost::beast::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) {
            return log_error("HttpSession::on_write", ec);
        }

        if (!keep_alive) {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // Read another request
        do_read();
    }

    void do_close() {
        // Send a TCP shutdown
        boost::beast::error_code ec;
        stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
};

HttpListener::HttpListener(BaseServer* server, boost::asio::io_context& io, ServerEnvironment& env)
    : server_{server},
      io_{io},
      acceptor_{io} {
    boost::beast::error_code ec;

    boost::asio::ip::tcp::endpoint endpoint(env.tcp_protocol(), env.port());

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        log_error("HttpListener::open", ec);
        return;
    }

    // Allow address reuse
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
        log_error("HttpListener::set_option", ec);
        return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
        log_error("HttpListener::bind", ec);
        return;
    }

    // Start listening for connections
    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
        log_error("HttpListener::listen", ec);
        return;
    }

    do_accept();
}

void HttpListener::do_accept() {
    // The new connection gets its own strand
    acceptor_.async_accept(io_, [this](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
        on_accept(ec, std::move(socket), this->server_);
    });
}

void HttpListener::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket, BaseServer* server) {
    if (ec) {
        log_error("HttpListener::on_accept", ec);
        return; // To avoid infinite loop
    }
    else {
        // Create the session and run it
        std::make_shared<HttpSession>(std::move(socket), server)->run();
    }

    // Accept another connection
    do_accept();
}

#elif defined(BOOST_VERSION) && BOOST_VERSION >= 106600 && BOOST_VERSION <= 108100

// Return a response for the given request.
//
// The concrete type of the response message (which depends on the
// request), is type-erased in message_generator.
template <class Body, class Allocator, class Send>
void handle_request(const boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>& request,
                    BaseServer* server,
                    Send&& send) {
    using response_t = boost::beast::http::response<boost::beast::http::string_body>;

    // Returns a bad request response
    auto const bad_request = [&request](boost::beast::string_view why) {
        response_t res{boost::beast::http::status::bad_request, request.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(request.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Handle only POST requests
    if (request.method() != boost::beast::http::verb::post) {
        send(bad_request("Unknown HTTP-method"));
    }

    // Buffers to hold request/response
    ClientToServerRequest inbound_request;
    ServerToClientResponse outbound_response;

    // 1) Retrieve inbound_request from request body
    ecf::restore_from_string(request.body(), inbound_request);

    // 2) Handle request, as per TcpBaseServer::handle_request()
    {
        try {
            // Service the inbound request, handling the request will populate the outbound_response_
            // Note:: Handle request will first authenticate
            outbound_response.set_cmd(inbound_request.handleRequest(server));
        }
        catch (std::exception& e) {
            outbound_response.set_cmd(PreAllocatedReply::error_cmd(e.what()));
        }

        // Clean up after inbound_request_ has run. i.e like re-claiming memory
        inbound_request.cleanup();
    }

    // 3) Serialize outbound_response into response body
    std::string outbound;
    ecf::save_as_string(outbound, outbound_response);

    outbound_response.cleanup();

    // 4) Ship response
    boost::beast::http::string_body::value_type body = outbound;

    if (inbound_request.terminateRequest()) {
        //        if (serverEnv_.debug()) {
        //            std::cout << "   <-- HttpServer::handle_terminate_request  exiting server via terminate() port "
        //                      << serverEnv_.port() << std::endl;
        //        }
        // TODO: Terminate the server!
    }

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to POST request
    response_t response{std::piecewise_construct,
                        std::make_tuple(std::move(body)),
                        std::make_tuple(boost::beast::http::status::ok, request.version())};
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, "application/json");
    response.content_length(size);
    response.keep_alive(request.keep_alive());

    send(std::move(response));
}

// Handles an HTTP server connection
class HttpSession : public std::enable_shared_from_this<HttpSession> {
    boost::asio::ip::tcp::socket socket_;
    boost::beast::flat_buffer buffer_;
    boost::beast::http::request<boost::beast::http::string_body> request_;
    BaseServer* server_;

public:
    // Take ownership of the stream
    HttpSession(boost::asio::ip::tcp::socket&& socket, BaseServer* server)
        : socket_(std::move(socket)),
          server_{server} {}

    // Start the asynchronous operation
    void run() {
        boost::asio::dispatch(socket_.get_executor(), [this]() { do_read(); });
    }

    void do_read() {
        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        request_ = {};

        std::cout << "::0:: do_read" << std::endl;

        // Read a request
        boost::beast::http::async_read(
            socket_, buffer_, request_, [this](boost::beast::error_code ec, std::size_t bytes_transferred) {
                std::cout << "::0:: calling on_read (" << ec << ", " << bytes_transferred << ")" << std::endl;
                on_read(ec, bytes_transferred);
            });
    }

    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if (ec == boost::beast::http::error::end_of_stream) {
            return do_close();
        }

        if (ec) {
            return log_error("HttpSession::on_read", ec);
        }

        // Send the response
        handle_request(request_, server_, [this](boost::beast::http::response<boost::beast::http::string_body>&& msg) {
            send_response(std::move(msg));
        });
    }

    void send_response(boost::beast::http::response<boost::beast::http::string_body>&& response) {
        bool keep_alive = response.keep_alive();

        // Write the response
        boost::beast::http::async_write(
            socket_, response, [this, keep_alive](boost::beast::error_code ec, std::size_t bytes_transferred) {
                on_write(keep_alive, ec, bytes_transferred);
            });
    }

    void on_write(bool keep_alive, boost::beast::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) {

        if (ec) {
            return log_error("HttpSession::on_write", ec);
        }

        if (!keep_alive) {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // Read another request
        // do_read();

        do_close();
    }

    void do_close() {
        // Send a TCP shutdown
        boost::beast::error_code ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
};

HttpListener::HttpListener(BaseServer* server, boost::asio::io_context& io, ServerEnvironment& env)
    : server_{server},
      io_{io},
      acceptor_{io} {
    boost::beast::error_code ec;

    boost::asio::ip::tcp::endpoint endpoint(env.tcp_protocol(), env.port());

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        log_error("HttpListener::open", ec);
        return;
    }

    // Allow address reuse
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
        log_error("HttpListener::set_option", ec);
        return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
        log_error("HttpListener::bind", ec);
        return;
    }

    // Start listening for connections
    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
        log_error("HttpListener::listen", ec);
        return;
    }

    do_accept();
}

void HttpListener::do_accept() {
    // The new connection gets its own strand
    acceptor_.async_accept(io_, [this](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
        on_accept(ec, std::move(socket), this->server_);
    });
}

void HttpListener::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket, BaseServer* server) {
    if (ec) {
        log_error("HttpListener::on_accept", ec);
        return; // To avoid infinite loop
    }
    else {
        // Create the session and run it
        std::make_shared<HttpSession>(std::move(socket), server)->run();
    }

    // Accept another connection
    do_accept();
}

#else

    #error "Unsupported boost version"

#endif
