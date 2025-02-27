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
#include "ecflow/base/Identification.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/ecflow_version.h"
#include "ecflow/server/ServerEnvironment.hpp"

inline void log_error(char const* where, boost::beast::error_code ec) {
    using namespace ecf;
    LOG(Log::ERR, where << ": " << ec.message());
}

#define LOG_LEVEL(LEVEL, WHERE, MESSAGE)                                      \
    {                                                                         \
        using namespace ecf;                                                  \
        std::cout << LEVEL << " (" << WHERE << "): " << MESSAGE << std::endl; \
    }

#define LOG_ERROR(WHERE, MESSAGE) LOG_LEVEL("ERR", WHERE, MESSAGE)
#define LOG_DEBUG(WHERE, MESSAGE) LOG_LEVEL("DBG", WHERE, MESSAGE)

static const std::string CONTENT_TYPE = "application/json";

template <class Body, class Allocator>
void handle_request(const boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>& request,
                    boost::beast::http::response<boost::beast::http::string_body>& response,
                    bool& is_terminate,
                    BaseServer* server) {
    using response_t = boost::beast::http::response<boost::beast::http::string_body>;

    // Returns a bad request response

    auto const bad_request = [&request](boost::beast::string_view why) {
        response_t res{boost::beast::http::status::bad_request, request.version()};
        res.set(boost::beast::http::field::server, ECFLOW_VERSION);
        res.set(boost::beast::http::field::content_type, CONTENT_TYPE);
        res.keep_alive(request.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Handle only POST requests
    if (request.method() != boost::beast::http::verb::post) {
        response = bad_request("Unknown HTTP-method");
    }

    ClientToServerRequest inbound_request;
    ServerToClientResponse outbound_response;

    // 1) Retrieve inbound_request from request body
    ecf::restore_from_string(request.body(), inbound_request);

    {
        ecf::Identity identity = ecf::Identity::make_none();

        // If possible, the Identify is retrieved from request header fields
        bool identity_headers = false;
        bool identity_secure  = false;
        std::string username;
        std::string password;
        {
            auto found = std::find_if(std::begin(request), std::end(request), [](auto& field) {
                return field.name_string() == "X-Auth-Username";
            });
            if (found != std::end(request)) {
                identity_headers = true;
                username         = found->value();
            }
        }
        {
            auto found = std::find_if(std::begin(request), std::end(request), [](auto& field) {
                return field.name_string() == "X-Auth-Password";
            });
            if (found != std::end(request)) {
                identity_headers = true;
                password         = found->value();
            }
        }
        {
            auto found = std::find_if(std::begin(request), std::end(request), [](auto& field) {
                return field.name_string() == "Authorization";
            });
            if (found != std::end(request)) {
                identity_secure = true;
            }
        }

        if (identity_secure && identity_headers) {
            LOG_DEBUG("HttpServer::handle_request", "Identity extracted from HTTP(s) request header (Authorization)");
            identity = ecf::Identity::make_secure_user(username);
        }
        else if (identity_headers) {
            LOG_DEBUG("HttpServer::handle_request", "Identity extracted from HTTP(s) request header (X-Auth-Username)");
            identity = ecf::Identity::make_user(username, password);
        }
        else {
            LOG_DEBUG("HttpServer::handle_request", "Identity extracted from inbound Command");
            auto cmd = inbound_request.get_cmd();
            identity = ecf::identify(cmd);
        }

        LOG_DEBUG("HttpServer::handle_request", "Identity extracted is " << identity.as_string());

        inbound_request.get_cmd()->set_identity(std::move(identity));
    }

    // 2) Handle request, as per TcpBaseServer::handle_request()
    {
        // Tag termination request, to be handled by the caller
        is_terminate = inbound_request.terminateRequest();

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

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to POST request
    response = response_t{std::piecewise_construct,
                          std::make_tuple(std::move(body)),
                          std::make_tuple(boost::beast::http::status::ok, request.version())};
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, CONTENT_TYPE);
    response.content_length(size);
    response.keep_alive(request.keep_alive());
}

// Handles an HTTP server connection
class HttpSession : public std::enable_shared_from_this<HttpSession> {
    boost::asio::ip::tcp::socket socket_;
    // Buffers for incoming and outgoing data
    boost::beast::flat_buffer buffer_{};
    boost::beast::http::request<boost::beast::http::string_body> request_{};
    boost::beast::http::response<boost::beast::http::string_body> response_{};
    bool is_terminate_{false};
    // The Http server that owns this session
    HttpServer* owner_;

    friend class HttpServer;

public:
    // Take ownership of the stream
    HttpSession(boost::asio::ip::tcp::socket&& socket, HttpServer* owner) : socket_(std::move(socket)), owner_{owner} {}

    void run() {
        // Clear the incoming and outgoing data buffers used buy the session
        buffer_   = {};
        request_  = {};
        response_ = {};

        boost::asio::dispatch(socket_.get_executor(), [self = shared_from_this()]() { self->do_read(); });
    }

    void do_read() {
        boost::beast::http::async_read(
            socket_,
            buffer_,
            request_,
            [self = shared_from_this()](boost::beast::error_code ec, std::size_t bytes_transferred) {
                self->on_read(ec, bytes_transferred);
            });
    }

    void on_read(boost::beast::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) {

        // Handle connection that has been closed
        if (ec == boost::beast::http::error::end_of_stream) {
            return do_close();
        }

        // Handle eventual read error
        if (ec) {
            return log_error("HttpSession::on_read", ec);
        }

        // Handle actual request
        handle_request(request_, response_, is_terminate_, owner_->server());

        // Send the response
        send_response();
    }

    void send_response() {
        // Write the response
        boost::beast::http::async_write(
            socket_,
            response_,
            [self = shared_from_this()](boost::beast::error_code ec, std::size_t bytes_transferred) {
                self->on_write(ec, bytes_transferred);
            });
    }

    void on_write(boost::beast::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) {
        // Handle eventual write error
        if (ec) {
            return log_error("HttpSession::on_write", ec);
        }

        // Finish off the connection
        do_close();
    }

    void do_close() {
        // Send a TCP shutdown
        boost::beast::error_code ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully

        // Handle terminate request (in case the request was a termination request, the owning server must terminate)
        owner_->handle_terminate(is_terminate_);
    }
};

HttpServer::HttpServer(BaseServer* server, boost::asio::io_context& io, ServerEnvironment& env)
    : server_{server},
      io_{io},
      acceptor_{io} {
    boost::beast::error_code ec;

    boost::asio::ip::tcp::endpoint endpoint(env.tcp_protocol(), env.port());

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        log_error("HttpServer::open", ec);
        return;
    }

    // Allow address reuse
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
        log_error("HttpServer::set_option", ec);
        return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
        log_error("HttpServer::bind", ec);
        return;
    }

    // Start listening for connections
    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
        log_error("HttpServer::listen", ec);
        return;
    }

    do_accept();
}

void HttpServer::handle_terminate(bool terminate) {
    if (terminate) {
        if (server_->debug()) {
            std::cout << "   <-- HttpServer exiting server via terminate()" << std::endl;
        }

        boost::asio::post(io_, [this]() {
            server_->handle_terminate();
            acceptor_.close();
            io_.stop();
        });
    }
}

void HttpServer::do_accept() {
    acceptor_.async_accept(io_, [this](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
        on_accept(ec, std::move(socket), this->server_);
    });
}

void HttpServer::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket, BaseServer* server) {
    if (ec) {
        log_error("HttpServer::on_accept", ec);
        // Terminate the server
        return;
    }
    else {
        // Create a new session (and thus, implicitly, a new strand) to process the connection
        std::make_shared<HttpSession>(std::move(socket), this)->run();
    }

    // Accept another connection
    do_accept();
}
