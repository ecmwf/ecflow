/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_ServerProtocolProbe_HPP
#define ecflow_base_ServerProtocolProbe_HPP

#include <chrono>
#include <optional>
#include <string>

#include "ecflow/base/ServerProtocol.hpp"

namespace ecf {

///
/// @brief Determines which protocol a server endpoint appears to speak.
///
/// The probe performs up to four short, independent attempts against the endpoint, and reports the
/// first protocol that the endpoint answers coherently. Each attempt is cheap and requires neither
/// valid credentials nor a certificate: the TLS attempt does not verify the peer certificate, so a
/// certificate mismatch still establishes that the peer speaks TLS.
///
/// The probe is intended for diagnosing a request that has already failed, and is never part of
/// the normal path. A failed attempt is likely to leave an error entry in the log of the probed
/// server, which is the unavoidable cost of asking a server what it speaks.
///
/// @param[in] host      The server host name
/// @param[in] port      The server port
/// @param[in] timeout   The per-attempt timeout
/// @param[in] try_first The protocol to attempt first; when set, and when the endpoint does speak
///                      it, the probe returns after a single attempt
/// @return The protocol determined; an empty optional when no attempt succeeded, which includes
///         the case of nothing listening on the endpoint
///
std::optional<Protocol> probe_server_protocol(const std::string& host,
                                              const std::string& port,
                                              std::chrono::milliseconds timeout = std::chrono::seconds{3},
                                              std::optional<Protocol> try_first = std::nullopt);

///
/// @brief Reports whether the endpoint answers an ecFlow request over plain TCP/IP.
///
/// @param[in] host    The server host name
/// @param[in] port    The server port
/// @param[in] timeout The attempt timeout
/// @return true when the endpoint answered with a decodable ecFlow reply; false otherwise
///
bool probes_as_plain(const std::string& host, const std::string& port, std::chrono::milliseconds timeout);

///
/// @brief Reports whether the endpoint completes a TLS handshake.
///
/// The peer certificate is deliberately not verified: the question asked is whether the endpoint
/// speaks TLS at all, not whether it presents a certificate this client trusts.
///
/// @param[in] host    The server host name
/// @param[in] port    The server port
/// @param[in] timeout The attempt timeout
/// @return true when the handshake completed; false otherwise, and always when this ecFlow was
///         built without SSL support
///
bool probes_as_tls(const std::string& host, const std::string& port, std::chrono::milliseconds timeout);

///
/// @brief Reports whether the endpoint answers an HTTP request over the given scheme.
///
/// Any well-formed HTTP status line counts as an answer, including an error status: the question
/// asked is whether the endpoint speaks HTTP, not whether it accepts the request.
///
/// @param[in] scheme  Either "http" or "https"
/// @param[in] host    The server host name
/// @param[in] port    The server port
/// @param[in] timeout The attempt timeout
/// @return true when the endpoint answered with an HTTP response; false otherwise
///
bool probes_as_http(const std::string& scheme,
                    const std::string& host,
                    const std::string& port,
                    std::chrono::milliseconds timeout);

} // namespace ecf

#endif /* ecflow_base_ServerProtocolProbe_HPP */
