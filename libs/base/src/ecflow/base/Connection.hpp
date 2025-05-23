/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_Connection_HPP
#define ecflow_base_Connection_HPP

///
/// \brief Serves as the connection between client server
///
/// \note If you change this file then ssl_connection.hpp may also need changing
///

#if defined(HPUX)
    #include <sys/select.h> // hp-ux uses pselect
#endif

#include <iomanip>
#include <memory>
#include <sstream>

#include <boost/asio.hpp>

#include "ecflow/core/Serialization.hpp"

// #define DEBUG_CONNECTION 1
#ifdef DEBUG_CONNECTION
    #include "ecflow/core/Ecf.hpp"
#endif

/// The connection class provides serialisation primitives on top of a socket.
/**
 * Each message sent using this class consists of:
 * @li An 8-byte header containing the length of the serialized data in
 * hexadecimal.
 * @li The serialized data.
 */
class connection {
public:
    ~connection();

    explicit connection(boost::asio::io_context& io);
    boost::asio::ip::tcp::socket& socket() { return socket_; }
    boost::asio::ip::tcp::socket& socket_ll() { return socket_; }

    /// Asynchronously write a data structure to the socket.
    template <typename T, typename Handler>
    void async_write(const T& t, Handler handler) {

#ifdef DEBUG_CONNECTION
        auto location = Ecf::server() ? "SERVER" : "CLIENT";
        std::cout << location << ": Connection::async_write\n"
                  << "   Serialise the data first so we know how large it is\n";
#endif
        // Serialise the data first so we know how large it is.
        try {
            ecf::save_as_string(outbound_data_, t);
        }
        catch (const std::exception& ae) {
            // Unable to decode data. Something went wrong, inform the caller.
            log_archive_error("Connection::async_write, exception ", ae, outbound_data_);
            boost::asio::post(socket_.get_executor(), [handler]() { handler(boost::asio::error::invalid_argument); });
            return;
        }

#ifdef DEBUG_CONNECTION
        std::cout << "   Format the header:\n";
#endif
        // Format the header.
        std::ostringstream header_stream;
        header_stream << std::setw(header_length) << std::hex << outbound_data_.size();
        if (!header_stream || header_stream.str().size() != header_length) {
            // Something went wrong, inform the caller.
            log_error("Connection::async_write, could not format header");
            boost::asio::post(socket_.get_executor(), [handler]() { handler(boost::asio::error::invalid_argument); });
            return;
        }
        outbound_header_ = header_stream.str();

#ifdef DEBUG_CONNECTION
        std::cout << "   Write the HEADER and serialised DATA to the socket\n"
                  << "   outbound_header_.size(" << outbound_header_.size() << ")\n"
                  << "   outbound_header_:'" << outbound_header_ << "' # this is the size in hex\n"
                  << "   outbound_data_.size(" << outbound_data_.size() << ")\n"
                  << "   hdr+data:'" << outbound_header_ << outbound_data_ << "'\n";
#endif
        // Write the serialized data to the socket. We use "gather-write" to send
        // both the header and the data in a single write operation.
        std::vector<boost::asio::const_buffer> buffers;
        buffers.reserve(2);
        buffers.emplace_back(boost::asio::buffer(outbound_header_));
        buffers.emplace_back(boost::asio::buffer(outbound_data_));
        boost::asio::async_write(
            socket_, buffers, [handler](const boost::system::error_code& error, std::size_t bytes_transferred) {
                handler(error);
            });

#ifdef DEBUG_CONNECTION
        std::cout << "   END\n";
#endif
    }

    /// Asynchronously read a data structure from the socket.
    template <typename T, typename Handler>
    void async_read(T& t, Handler handler) {

#ifdef DEBUG_CONNECTION
        auto location = Ecf::server() ? "SERVER" : "CLIENT";
        std::cout << location << ": Connection::async_read\n";
#endif

        // Issue a read operation to read exactly the number of bytes in a header
        boost::asio::async_read(
            socket_,
            boost::asio::buffer(inbound_header_),
            [this, &t, handler](const boost::system::error_code& error, std::size_t bytes_transferred) {
                this->handle_read_header(error, t, handler);
            });
    }

private:
    /// Handle a completed read of a message header.
    template <typename T, typename Handler>
    void handle_read_header(const boost::system::error_code& e, T& t, Handler handler) {
#ifdef DEBUG_CONNECTION
        auto location = Ecf::server() ? "SERVER" : "CLIENT";
        std::cout << location << ": Connection::handle_read_header\n"
                  << "   header:'" << std::string(inbound_header_, header_length)
                  << "'  # this size of payload in hex\n";
#endif
        if (e) {
            handler(e);
        }
        else {
            // Determine the length of the serialized data.
            std::istringstream is(std::string(inbound_header_, header_length));
            std::size_t inbound_data_size = 0;
            if (!(is >> std::hex >> inbound_data_size)) {

                // Header doesn't seem to be valid. Inform the caller.
                std::string err =
                    "Connection::handle_read_header: invalid header : " + std::string(inbound_header_, header_length);
                log_error(err.c_str());
                handler(boost::asio::error::invalid_argument);
                return;
            }

            // Start an asynchronous call to receive the data.
            inbound_data_.resize(inbound_data_size);
            boost::asio::async_read(
                socket_,
                boost::asio::buffer(inbound_data_),
                [this, &t, handler](const boost::system::error_code& error, std::size_t bytes_transferred) {
                    this->handle_read_data(error, t, handler);
                });
        }
    }

    /// Handle a completed read of message data.
    template <typename T, typename Handler>
    void handle_read_data(const boost::system::error_code& e, T& t, Handler handler) {
#ifdef DEBUG_CONNECTION
        auto location = Ecf::server() ? "SERVER" : "CLIENT";
        std::cout << location << ": Connection::handle_read_data\n";
#endif

        if (e) {
            handler(e);
        }
        else {
            // Extract the data structure from the data just received.
            std::string archive_data(&inbound_data_[0], inbound_data_.size());
            try {
#ifdef DEBUG_CONNECTION
                std::cout << "   inbound_data_.size(" << inbound_data_.size() << ") typeid=" << typeid(t).name() << "\n"
                          << "   '" << archive_data << "'\n";
#endif
                ecf::restore_from_string(archive_data, t);
            }
            catch (std::exception& e) {
                log_archive_error("Connection::handle_read_data, Unable to decode data :", e, archive_data);
                handler(boost::asio::error::invalid_argument);
                return;
            }

            // Inform caller that data has been received ok.
            handler(e);
        }
    }

private:
    static void log_error(const char* msg);
    static void log_archive_error(const char* msg, const std::exception& ae, const std::string& data);

private:
    boost::asio::ip::tcp::socket socket_; /// The underlying socket.
    std::string outbound_header_;         /// Holds an out-bound header.
    std::string outbound_data_;           /// Holds the out-bound data.
    enum { header_length = 8 };           /// The size of a fixed length header.
    char inbound_header_[header_length];  /// Holds an in-bound header.
    std::vector<char> inbound_data_;      /// Holds the in-bound data.
};

typedef std::shared_ptr<connection> connection_ptr;

#endif /* ecflow_base_Connection_HPP */
