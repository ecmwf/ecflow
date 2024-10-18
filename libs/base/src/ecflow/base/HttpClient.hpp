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

#define CPPHTTPLIB_THREAD_POOL_COUNT 1
#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#define CPPHTTPLIB_ZLIB_SUPPORT 1

#include <httplib.h>

#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/Connection.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"

///
/// \brief  This class acts as an HTTP client
///

class HttpClient {
public:
    /// Constructor starts the asynchronous connect operation.
    HttpClient(Cmd_ptr cmd_ptr,
               const std::string& scheme,
               const std::string& host,
               const std::string& port,
               int timeout = 60);

    void run();

    /// Client side, get the server response, handles reply from server
    /// Returns true if all is ok, else false if further client action is required
    /// will throw std::runtime_error for errors
    bool handle_server_response(ServerReply&, bool debug) const;

private:
    std::string scheme_; /// the scheme to use
    std::string host_; /// the servers name
    std::string port_; /// the port on the server
    std::string base_url_;
    httplib::Client client_;

    httplib::Response response_;
    httplib::Error status_ = httplib::Error::Success;
    std::string reason_    = "";

    ClientToServerRequest outbound_request_;  /// The request we will send to the server
    ServerToClientResponse inbound_response_; /// The response we get back from the server
};

#endif /* ecflow_base_HttpClient_HPP */
