/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/ConnectionFailureMapping.hpp"

#include <boost/asio/error.hpp>

#ifdef ECF_OPENSSL
    #include <boost/asio/ssl/error.hpp>
    #include <openssl/err.h>
    #include <openssl/ssl.h>
#endif

namespace ecf {

ConnectionFailure classify_connect_error(const boost::system::error_code& error) {
    if (!error) {
        return ConnectionFailure::None;
    }

    if (error == boost::asio::error::host_not_found || error == boost::asio::error::host_not_found_try_again ||
        error == boost::asio::error::service_not_found || error == boost::asio::error::no_data ||
        error == boost::asio::error::no_recovery) {
        return ConnectionFailure::HostResolution;
    }

    if (error == boost::asio::error::connection_refused || error == boost::asio::error::host_unreachable ||
        error == boost::asio::error::network_unreachable || error == boost::asio::error::network_down ||
        error == boost::asio::error::address_family_not_supported) {
        return ConnectionFailure::ConnectionRefused;
    }

    if (error == boost::asio::error::timed_out) {
        return ConnectionFailure::Timeout;
    }

    return ConnectionFailure::Other;
}

ConnectionFailure classify_read_error(const boost::system::error_code& error) {
    if (!error) {
        return ConnectionFailure::None;
    }

    // A peer that closes the connection rather than reply is reported either as a clean end of
    // file, or as a reset, or as a broken pipe, depending on how abruptly it closed.
    if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset ||
        error == boost::asio::error::broken_pipe || error == boost::asio::error::connection_aborted) {
        return ConnectionFailure::ClosedWithoutReply;
    }

    // Raised by Connection::handle_read_header() when the reply does not begin with a decodable
    // ecFlow frame header, and by Boost.Asio itself for a malformed payload.
    if (error == boost::asio::error::invalid_argument || error == boost::asio::error::message_size) {
        return ConnectionFailure::UndecodableReply;
    }

    if (error == boost::asio::error::timed_out || error == boost::asio::error::operation_aborted) {
        return ConnectionFailure::Timeout;
    }

    return ConnectionFailure::Other;
}

bool suggests_protocol_mismatch(ConnectionFailure failure) {
    switch (failure) {
        case ConnectionFailure::ClosedWithoutReply:
        case ConnectionFailure::UndecodableReply:
        case ConnectionFailure::HandshakeFailed:
            return true;
        case ConnectionFailure::None:
        case ConnectionFailure::HostResolution:
        case ConnectionFailure::ConnectionRefused:
        case ConnectionFailure::Timeout:
        case ConnectionFailure::CertificateRejected:
        case ConnectionFailure::RejectedRequest:
        case ConnectionFailure::Other:
            break;
    }
    return false;
}

#ifdef ECF_OPENSSL

ConnectionFailure classify_handshake_error(const boost::system::error_code& error) {
    if (!error) {
        return ConnectionFailure::None;
    }

    // OpenSSL reports a rejected certificate as an error in the SSL library, carrying one of the
    // verification reasons below. Everything else -- a plaintext peer, a closed connection, an
    // unsupported protocol version -- means the peer is not completing a TLS handshake at all.
    if (error.category() == boost::asio::error::get_ssl_category()) {
        const int reason = ERR_GET_REASON(error.value());
        switch (reason) {
            case SSL_R_CERTIFICATE_VERIFY_FAILED:
            case SSL_R_SSLV3_ALERT_BAD_CERTIFICATE:
            case SSL_R_SSLV3_ALERT_UNSUPPORTED_CERTIFICATE:
            case SSL_R_SSLV3_ALERT_CERTIFICATE_REVOKED:
            case SSL_R_SSLV3_ALERT_CERTIFICATE_EXPIRED:
            case SSL_R_SSLV3_ALERT_CERTIFICATE_UNKNOWN:
            case SSL_R_TLSV1_ALERT_UNKNOWN_CA:
                return ConnectionFailure::CertificateRejected;
            default:
                break;
        }
    }

    return ConnectionFailure::HandshakeFailed;
}

#endif

} // namespace ecf
