/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_HttpClient_HPP
#define ecflow_base_HttpClient_HPP

#include <httplib.h>

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

class HttpClient {
public:
    /// Constructor starts the asynchronous connect operation.
    HttpClient(Cmd_ptr cmd_ptr, const std::string& host, const std::string& port, int timout = 0);

    void run();

    /// Client side, get the server response, handles reply from server
    /// Returns true if all is ok, else false if further client action is required
    /// will throw std::runtime_error for errors
    bool handle_server_response(ServerReply&, bool debug) const;

private:
    bool stopped_;
    std::string host_; /// the servers name
    std::string port_; /// the port on the server
    httplib::Client client_;
    ClientToServerRequest outbound_request_;  /// The request we will send to the server
    ServerToClientResponse inbound_response_; /// The response we get back from the server
};

#endif /* ecflow_base_HttpClient_HPP */
