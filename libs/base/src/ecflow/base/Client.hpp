/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_Client_HPP
#define ecflow_base_Client_HPP

#include <boost/asio.hpp>

#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/Connection.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"

///
/// \brief  This class acts as the client part. ( in client/server architecture)
///
/// \note The plug command can move a node to another server hence the server
/// itself will NEED to ACT as a client. This is why client lives in Base and
/// not the Client project
///

class Client {
public:
    /// Constructor starts the asynchronous connect operation.
    Client(boost::asio::io_context& io,
           Cmd_ptr cmd_ptr,
           const std::string& host,
           const std::string& port,
           int timout = 0);
    ~Client();

    /// Client side, get the server response, handles reply from server
    /// Returns true if all is ok, else false if further client action is required
    /// will throw std::runtime_error for errors
    bool handle_server_response(ServerReply&, bool debug) const;

private:
    void start(boost::asio::ip::tcp::resolver::iterator);
    void stop();
    void check_deadline();

    bool start_connect(boost::asio::ip::tcp::resolver::iterator);
    void start_write();
    void start_read();

    /// Handle completion of a connect operation.
    void handle_connect(const boost::system::error_code& e, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

    /// Handle completion of a read operation.
    void handle_read(const boost::system::error_code& e);

    /// Handle completion of a write operation
    void handle_write(const boost::system::error_code& e);

private:
    bool stopped_;
    std::string host_;                        /// the servers name
    std::string port_;                        /// the port on the server
    connection connection_;                   /// The connection to the server.
    ClientToServerRequest outbound_request_;  /// The request we will send to the server
    ServerToClientResponse inbound_response_; /// The response we get back from the server

    boost::asio::deadline_timer deadline_;

    //    connect        : timeout_ second
    //    send request   : timeout_ second
    //    receive reply  : timeout_ second
    // Default value of 0 means take the timeout from the command
    int timeout_;
};

#endif /* ecflow_base_Client_HPP */
