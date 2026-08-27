/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_ConnectionFailureMapping_HPP
#define ecflow_base_ConnectionFailureMapping_HPP

#include <boost/system/error_code.hpp>

#include "ecflow/base/ConnectionDiagnosis.hpp"

namespace ecf {

///
/// @brief Classifies a Boost.Asio error observed while connecting to a server.
///
/// @param[in] error The error reported by the connect operation; must not be a success code
/// @return The matching failure class
///
ConnectionFailure classify_connect_error(const boost::system::error_code& error);

///
/// @brief Classifies a Boost.Asio error observed while reading a reply from a server.
///
/// @param[in] error The error reported by the read operation; must not be a success code
/// @return The matching failure class
///
ConnectionFailure classify_read_error(const boost::system::error_code& error);

///
/// @brief Reports whether a read failure is consistent with the peer speaking another protocol.
///
/// The connection having been accepted before the failure is what separates a mismatch from a
/// server that is simply not running: a peer that accepts and then closes, or that answers with
/// something undecodable, is listening but is not speaking the expected protocol.
///
/// @param[in] failure The failure class observed while reading a reply
/// @return true when a protocol mismatch is a plausible cause; false otherwise
///
bool suggests_protocol_mismatch(ConnectionFailure failure);

#ifdef ECF_OPENSSL

///
/// @brief Classifies a Boost.Asio error observed during a TLS handshake.
///
/// Certificate verification failures are separated from every other handshake failure, because the
/// two call for opposite corrective actions: a rejected certificate means the peer does speak TLS,
/// whereas any other handshake failure suggests that it does not.
///
/// @param[in] error The error reported by the handshake operation; must not be a success code
/// @return ConnectionFailure::CertificateRejected on a verification failure;
///         ConnectionFailure::HandshakeFailed otherwise
///
ConnectionFailure classify_handshake_error(const boost::system::error_code& error);

#endif

} // namespace ecf

#endif /* ecflow_base_ConnectionFailureMapping_HPP */
