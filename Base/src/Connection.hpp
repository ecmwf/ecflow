#ifndef CONNECTION_HPP_
#define CONNECTION_HPP_
//============================================================================
// Name        : Connection.cpp
// Author      : Avi
// Revision    : $Revision: #26 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Serves as the connection between client server
//============================================================================

#if defined(HPUX)
#include <sys/select.h> // hp-ux uses pselect
#endif

#include <iomanip>
#include <sstream>
#include <memory>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>

#include "Log.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "Serialization.hpp"

#ifdef ECF_OPENSSL
#include <boost/asio/ssl.hpp>
typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
#endif

#ifdef DEBUG
//#define DEBUG_CONNECTION 1
//#define DEBUG_CONNECTION_MEMORY 1
#endif

/// The connection class provides serialisation primitives on top of a socket.
/// ISSUES
/// ======
/// When the test are run with different configuration
/// TEXT,BINARY, PORTABLE_BINARY there was _no_ discernible difference in the
/// run times? I would have expected client->server communication via binary
/// to be faster. However the definition file we are using are relatively small!
///
///
/**
 * Each message sent using this class consists of:
 * @li An 8-byte header containing the length of the serialized data in
 * hexadecimal.
 * @li The serialized data.
 */
class connection {
public:
	/// Allow tentative support for new client to talk to old server
	/// by changing the boost serialisation archive version, hence tentative
	~connection();

#ifdef ECF_OPENSSL
	connection(boost::asio::io_service& io_service,boost::asio::ssl::context& context);
	bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx);

	/// Get the underlying socket. Used for making a connection or for accepting
	/// an incoming connection.
	ssl_socket::lowest_layer_type& socket_ll() { return socket_.lowest_layer();}
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& socket() { return socket_;}
#else
	explicit connection(boost::asio::io_service& io_service);
	boost::asio::ip::tcp::socket& socket_ll() { return socket_; }
	boost::asio::ip::tcp::socket& socket() { return socket_; }
#endif

	/// Asynchronously write a data structure to the socket.
	template<typename T, typename Handler>
	void async_write(const T& t, Handler handler) {

#ifdef DEBUG_CONNECTION
		std::cout << "Connection::async_write, Serialise the data first so we know how large it is\n";
#endif
		// Serialise the data first so we know how large it is.
		try {
			ecf::save_as_string(outbound_data_,t);
		} catch (const std::exception& ae ) {
			// Unable to decode data. Something went wrong, inform the caller.
			log_archive_error("Connection::async_write, exception ",ae);
			boost::system::error_code error(boost::asio::error::invalid_argument);
			socket_.get_io_service().post(boost::bind(handler, error));
			return;
		}

#ifdef DEBUG_CONNECTION
		std::cout << "Connection::async_write Format the header\n";
		std::cout << " " << outbound_data_ << "\n";
#endif
		// Format the header.
		std::ostringstream header_stream;
		header_stream << std::setw(header_length) << std::hex << outbound_data_.size();
		if (!header_stream || header_stream.str().size() != header_length) {
			// Something went wrong, inform the caller.
			log_error("Connection::async_write, could not format header");
			boost::system::error_code error(boost::asio::error::invalid_argument);
			socket_.get_io_service().post(boost::bind(handler, error));
			return;
		}
		outbound_header_ = header_stream.str();


#ifdef DEBUG_CONNECTION_MEMORY
		if (Ecf::server()) std::cout << "server::";
		else               std::cout << "client::";
		std::cout << "async_write outbound_header_.size(" << outbound_header_.size() << ") outbound_data_.size(" << outbound_data_.size() << ")\n";
#endif

#ifdef DEBUG_CONNECTION
		std::cout << "Connection::async_write Write the serialized data to the socket. \n";
#endif
		// Write the serialized data to the socket. We use "gather-write" to send
		// both the header and the data in a single write operation.
		std::vector<boost::asio::const_buffer> buffers; buffers.reserve(2);
		buffers.emplace_back(boost::asio::buffer(outbound_header_));
		buffers.emplace_back(boost::asio::buffer(outbound_data_));
		boost::asio::async_write(socket_, buffers, handler);

#ifdef DEBUG_CONNECTION
		std::cout << "Connection::async_write END \n";
#endif
	}

	/// Asynchronously read a data structure from the socket.
	template<typename T, typename Handler>
	void async_read(T& t, Handler handler) {

#ifdef DEBUG_CONNECTION
		std::cout << "Connection::async_read\n";
#endif

		// Issue a read operation to read exactly the number of bytes in a header.
		void (connection::*f)(const boost::system::error_code&, T&,boost::tuple<Handler>)
				= &connection::handle_read_header<T, Handler>;

		boost::asio::async_read(socket_, boost::asio::buffer(inbound_header_),
				boost::bind(f, this, boost::asio::placeholders::error,
						boost::ref(t), boost::make_tuple(handler)));
	}

private:
	/// Handle a completed read of a message header. The handler is passed using
	/// a tuple since boost::bind seems to have trouble binding a function object
	/// created using boost::bind as a parameter.
	template<typename T, typename Handler>
	void handle_read_header(const boost::system::error_code& e, T& t,boost::tuple<Handler> handler)
	{
		if (e) {
			boost::get<0>(handler)(e);
		} else {
			// Determine the length of the serialized data.
			std::istringstream is(std::string(inbound_header_, header_length));
			std::size_t inbound_data_size = 0;
			if (!(is >> std::hex >> inbound_data_size)) {

				// Header doesn't seem to be valid. Inform the caller.
				boost::system::error_code error(boost::asio::error::invalid_argument);
				boost::get<0>(handler)(error);
				return;
			}

			// Start an asynchronous call to receive the data.
			inbound_data_.resize(inbound_data_size);
			void (connection::*f)(const boost::system::error_code&, T&,boost::tuple<Handler>)
					= &connection::handle_read_data<T, Handler>;

			boost::asio::async_read(socket_,
					boost::asio::buffer(inbound_data_), boost::bind(f, this,
							boost::asio::placeholders::error, boost::ref(t),
							handler));
		}
	}

	/// Handle a completed read of message data.
	template<typename T, typename Handler>
	void handle_read_data(const boost::system::error_code& e, T& t, boost::tuple<Handler> handler)
	{
		if (e) {
			boost::get<0>(handler)(e);
		} else {
			// Extract the data structure from the data just received.
			try {
				std::string archive_data(&inbound_data_[0], inbound_data_.size());

#ifdef DEBUG_CONNECTION_MEMORY
				if (Ecf::server()) std::cout << "server::";
				else               std::cout << "client::";
				std::cout << "handle_read_data inbound_data_.size(" << inbound_data_.size() << ") typeid(" << typeid(t).name() << ")\n";
				std::cout << archive_data << "\n";
#endif
				ecf::restore_from_string(archive_data,t);
			}
			catch (std::exception& e) {
				log_archive_error("Connection::handle_read_data, Unable to decode data :",e);
				boost::system::error_code error( boost::asio::error::invalid_argument);
				boost::get<0>(handler)(error);
				return;
			}

			// Inform caller that data has been received ok.
			boost::get<0>(handler)(e);
		}
	}

private:

	static void log_error(const char* msg);
	static void log_archive_error(const char* msg,const std::exception& ae);

private:
#ifdef ECF_OPENSSL
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
#else
	boost::asio::ip::tcp::socket socket_;/// The underlying socket.
#endif
	std::string outbound_header_;        /// Holds an out-bound header.
	std::string outbound_data_;          /// Holds the out-bound data.
	enum { header_length = 8 };          /// The size of a fixed length header.
	char inbound_header_[header_length]; /// Holds an in-bound header.
	std::vector<char> inbound_data_; 	 /// Holds the in-bound data.
};

typedef std::shared_ptr<connection> connection_ptr;

#endif /* CONNECTION_HPP_ */
