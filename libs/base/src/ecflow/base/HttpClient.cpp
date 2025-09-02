/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/HttpClient.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "ecflow/base/stc/StcCmd.hpp"
#include "ecflow/core/Converter.hpp"

static std::string make_scheme_host_port(const std::string& scheme, const std::string& host, const std::string& port) {
    return scheme + "://" + host + ":" + port;
}

HttpClient::HttpClient(Cmd_ptr cmd_ptr,
                       const std::string& scheme,
                       const std::string& host,
                       const std::string& port,
                       int timeout)
    : scheme_(scheme),
      host_(host),
      port_(port),
      base_url_(make_scheme_host_port(scheme, host, port)),
      client_(base_url_),
      headers_() {

    client_.set_connection_timeout(std::chrono::seconds{timeout});
    client_.set_read_timeout(std::chrono::seconds{timeout});
    client_.set_write_timeout(std::chrono::seconds{timeout});

    // Disable cert verification
    if (scheme_ == "https") {
        client_.enable_server_certificate_verification(false);
    }

    if (!cmd_ptr.get()) {
        throw std::runtime_error("Client::Client: No request specified !");
    }

    outbound_request_.set_cmd(cmd_ptr);
}

void HttpClient::run() {
    std::string outbound;
    ecf::save_as_string(outbound, outbound_request_);

    client_.set_default_headers(headers_);

    auto result = client_.Post("/v1/ecflow", outbound, "application/json");
    if (result) {
        auto response = result.value();
        status_       = httplib::Error::Success;
        ecf::restore_from_string(response.body, inbound_response_);
    }
    else {
        status_ = result.error();
        reason_ = httplib::to_string(status_);
    }
}

bool HttpClient::handle_server_response(ServerReply& server_reply, bool debug) const {
    if (debug) {
        std::cout << "  Client::handle_server_response" << std::endl;
    }
    server_reply.set_host_port(host_, port_); // client context, needed by some commands, ie. SServerLoadCmd

    if (status_ == httplib::Error::Success) {
        return inbound_response_.handle_server_response(server_reply, outbound_request_.get_cmd(), debug);
    }
    else {
        std::stringstream ss;
        ss << "HttpClient::handle_server_response: Error: " << status_ << " " << reason_;
        throw std::runtime_error(ss.str());
    }
}
