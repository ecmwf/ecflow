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

#include "ecflow/base/ConnectionFailureMapping.hpp"
#include "ecflow/base/stc/StcCmd.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Message.hpp"

namespace ecf::http {

ConnectionFailure classify_httplib_error(httplib::Error error) {
    switch (error) {
        case httplib::Error::Success:
            return ecf::ConnectionFailure::None;
        case httplib::Error::Connection:
        case httplib::Error::BindIPAddress:
            return ecf::ConnectionFailure::ConnectionRefused;
        case httplib::Error::ConnectionTimeout:
        case httplib::Error::Canceled:
            return ecf::ConnectionFailure::Timeout;
        case httplib::Error::Read:
        case httplib::Error::Write:
            // The connection was established, and then failed while exchanging data. A peer that
            // is listening but does not answer with HTTP produces exactly this.
            return ecf::ConnectionFailure::ClosedWithoutReply;
        case httplib::Error::SSLConnection:
        case httplib::Error::SSLLoadingCerts:
            return ecf::ConnectionFailure::HandshakeFailed;
        case httplib::Error::SSLServerVerification:
            return ecf::ConnectionFailure::CertificateRejected;
        case httplib::Error::ExceedRedirectCount:
        case httplib::Error::UnsupportedMultipartBoundaryChars:
        case httplib::Error::Compression:
        case httplib::Error::Unknown:
            break;
    }
    return ecf::ConnectionFailure::Other;
}

} // namespace ecf::http

static std::string make_scheme_host_port(const std::string& scheme, const std::string& host, const std::string& port) {
    return scheme + "://" + host + ":" + port;
}

HttpClient::HttpClient(Cmd_ptr cmd_ptr,
                       const std::string& scheme,
                       const std::string& host,
                       const std::string& port,
                       int timeout,
                       ecf::ConnectionDiagnosis* diagnosis)
    : scheme_(scheme),
      host_(host),
      port_(port),
      base_url_(make_scheme_host_port(scheme, host, port)),
      client_(base_url_),
      headers_(),
      diagnosis_(diagnosis ? *diagnosis : owned_diagnosis_) {

    diagnosis_.clear();
    diagnosis_.client_protocol = (scheme_ == "https") ? ecf::Protocol::Https : ecf::Protocol::Http;
    diagnosis_.host            = host_;
    diagnosis_.port            = port_;

    client_.set_connection_timeout(std::chrono::seconds{timeout});
    client_.set_read_timeout(std::chrono::seconds{timeout});
    client_.set_write_timeout(std::chrono::seconds{timeout});

    // Disable cert verification
    if (scheme_ == "https") {
#ifdef ECF_OPENSSL
        client_.enable_server_certificate_verification(false);
#else
        // Without SSL support, the underlying HTTP library is unable to establish an HTTPS connection,
        // and reporting this immediately is preferable to attempting the request and failing obscurely.
        static const char* no_ssl_support = "HttpClient::HttpClient: Unable to use HTTPS, "
                                            "since this ecFlow was built without SSL support";
        record_failure(ecf::ConnectionFailure::HandshakeFailed, no_ssl_support);
        diagnosis_.protocol_mismatch_suspected = false;
        throw std::runtime_error(no_ssl_support);
#endif
    }

    if (!cmd_ptr.get()) {
        throw std::runtime_error("Client::Client: No request specified !");
    }

    outbound_request_.set_cmd(cmd_ptr);
}

void HttpClient::record_failure(ecf::ConnectionFailure failure, const std::string& detail) {
    diagnosis_.client_protocol = (scheme_ == "https") ? ecf::Protocol::Https : ecf::Protocol::Http;
    diagnosis_.failure         = failure;
    diagnosis_.host            = host_;
    diagnosis_.port            = port_;
    diagnosis_.detail          = detail;
    // The HTTP library reports a failure to connect separately from a failure to exchange data,
    // which is what allows a mismatch to be told apart from a server that is not running.
    diagnosis_.protocol_mismatch_suspected = ecf::suggests_protocol_mismatch(failure);
}

void HttpClient::run() {
    std::string outbound;
    ecf::save_as_string(outbound, outbound_request_);

    client_.set_default_headers(headers_);

    if (auto result = client_.Post("/v1/ecflow", outbound, "application/json"); result) {
        status_ = static_cast<ecf::http::Status>(result->status);
        reason_ = "";
        if (status_ == ecf::http::Status::OK) {
            auto response = result.value();
            ecf::restore_from_string(response.body, inbound_response_);
        }
        else {
            // The peer answered with a well-formed HTTP response, so it does speak HTTP; the
            // request itself was refused.
            reason_ = MESSAGE(status_);
            record_failure(ecf::ConnectionFailure::RejectedRequest, reason_);
            diagnosis_.peer_protocol = diagnosis_.client_protocol;
        }
    }
    else {
        status_ = ecf::http::Status::Unknown;
        reason_ = httplib::to_string(result.error());
        record_failure(ecf::http::classify_httplib_error(result.error()), reason_);
    }
}

bool HttpClient::handle_server_response(ServerReply& server_reply, bool debug) const {
    if (debug) {
        std::cout << "  Client::handle_server_response" << std::endl;
    }
    server_reply.set_host_port(host_, port_); // client context, needed by some commands, i.e., SServerLoadCmd
    server_reply.set_diagnosis(diagnosis_);

    if (status_ == ecf::http::Status::OK) {
        return inbound_response_.handle_server_response(server_reply, outbound_request_.get_cmd(), debug);
    }

    throw std::runtime_error(
        MESSAGE("HttpClient::handle_server_response: Error: " << status_ << (reason_.empty() ? "" : " : ") << reason_));
}
