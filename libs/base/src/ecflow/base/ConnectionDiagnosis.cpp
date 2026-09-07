/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/ConnectionDiagnosis.hpp"

#include <sstream>

namespace ecf {

namespace {

// The endpoint, rendered as "<host>:<port>", or as "the server" when neither is known.
std::string endpoint_of(const ConnectionDiagnosis& diagnosis) {
    if (diagnosis.host.empty() && diagnosis.port.empty()) {
        return "the server";
    }
    std::ostringstream os;
    os << diagnosis.host;
    if (!diagnosis.port.empty()) {
        os << ":" << diagnosis.port;
    }
    return os.str();
}

// What was observed, phrased so that it reads after the endpoint has been introduced.
std::string observation_of(ConnectionFailure failure) {
    switch (failure) {
        case ConnectionFailure::HostResolution:
            return "The host name could not be resolved.";
        case ConnectionFailure::ConnectionRefused:
            return "The connection was refused, which means that nothing is listening on that port.";
        case ConnectionFailure::Timeout:
            return "The connection was accepted, but no reply arrived before the timeout elapsed.";
        case ConnectionFailure::ClosedWithoutReply:
            return "The connection was accepted and then closed without a reply.";
        case ConnectionFailure::HandshakeFailed:
            return "The TLS handshake did not complete.";
        case ConnectionFailure::CertificateRejected:
            return "The TLS handshake failed while verifying the server certificate.";
        case ConnectionFailure::UndecodableReply:
            return "A reply arrived that the client is unable to decode.";
        case ConnectionFailure::RejectedRequest:
            return "The server answered, refusing the request.";
        case ConnectionFailure::Other:
            return "The exchange failed for an unrecognised reason.";
        case ConnectionFailure::None:
            break;
    }
    return "";
}

// The action the user is expected to take, given the failure and what is known about the peer.
std::string advice_of(const ConnectionDiagnosis& diagnosis) {
    if (diagnosis.failure == ConnectionFailure::CertificateRejected) {
        return "Check that the client and the server use the same self-signed certificate.";
    }
    if (diagnosis.is_protocol_mismatch()) {
        if (diagnosis.peer_protocol) {
            std::ostringstream os;
            os << "Configure the client to use " << to_ui_designation(diagnosis.peer_protocol.value())
               << ", or restart the server so that it uses " << to_ui_designation(diagnosis.client_protocol) << ".";
            return os.str();
        }
        return "Check that the client and the server are configured for the same protocol.";
    }
    if (diagnosis.failure == ConnectionFailure::ConnectionRefused ||
        diagnosis.failure == ConnectionFailure::HostResolution) {
        return "Check that the server is running, and that the host and port are correct.";
    }
    return "";
}

} // namespace

std::string explain(const ConnectionDiagnosis& diagnosis) {
    if (diagnosis.ok()) {
        return "";
    }

    std::ostringstream os;

    const char* headline = "Cannot communicate with the server";
    if (diagnosis.is_protocol_mismatch()) {
        headline = "Protocol mismatch";
    }
    else if (diagnosis.failure == ConnectionFailure::RejectedRequest) {
        headline = "Request refused";
    }

    os << headline << ": the client is using " << to_ui_designation(diagnosis.client_protocol) << " to reach "
       << endpoint_of(diagnosis);

    if (diagnosis.peer_protocol) {
        os << ", which appears to be using " << to_ui_designation(diagnosis.peer_protocol.value());
    }
    os << ".";

    if (auto observation = observation_of(diagnosis.failure); !observation.empty()) {
        os << " " << observation;
    }
    if (auto advice = advice_of(diagnosis); !advice.empty()) {
        os << " " << advice;
    }
    if (!diagnosis.detail.empty()) {
        os << " Detail: " << diagnosis.detail;
    }

    return os.str();
}

} // namespace ecf
