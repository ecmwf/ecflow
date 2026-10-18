// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <optional>
#include <string>

#include "ecflow/base/ConnectionDiagnosis.hpp"
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
/// @brief Attempts a plain TCP connection to the endpoint, and classifies how the attempt ends.
///
/// This determines why a connection failed when the transport that observed the failure does not
/// say, as the HTTP library does for a refused connection and for one left unanswered alike.
///
/// @param[in] host    The server host name
/// @param[in] port    The server port
/// @param[in] timeout The time to wait for an answer to the connection attempt
/// @return ConnectionFailure::None when the connection was established;
///         ConnectionFailure::ConnectionUnanswered when no answer arrived within @p timeout;
///         the classification of the connect error otherwise
///
ConnectionFailure probe_connect(const std::string& host, const std::string& port, std::chrono::milliseconds timeout);

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
