/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_ConnectionDiagnosis_HPP
#define ecflow_base_ConnectionDiagnosis_HPP

#include <optional>
#include <ostream>
#include <string>

#include "ecflow/base/ServerProtocol.hpp"

namespace ecf {

///
/// @brief The class of failure observed while communicating with a server.
///
/// The value is assigned by the transport that observed the failure, at the point where the
/// underlying error is still available. Each value describes *what was observed*, and never
/// *what the cause is presumed to be*: the presumed cause is recorded separately, in
/// ConnectionDiagnosis::peer_protocol and ConnectionDiagnosis::protocol_mismatch_suspected.
///
enum class ConnectionFailure {
    None,                ///< No failure was observed
    HostResolution,      ///< The host name could not be resolved
    ConnectionRefused,   ///< No listener accepted the connection
    Timeout,             ///< The peer accepted the connection but did not reply in time
    ClosedWithoutReply,  ///< The peer accepted the connection and then closed it without replying
    HandshakeFailed,     ///< The TLS handshake did not complete
    CertificateRejected, ///< The TLS handshake failed while verifying the peer certificate
    UndecodableReply,    ///< A reply was received that the transport cannot decode
    RejectedRequest,     ///< The peer answered, refusing the request
    Other                ///< A failure that none of the other values describes
};

namespace detail {

template <>
struct EnumTraits<ConnectionFailure>
{
    static constexpr std::array<std::pair<ConnectionFailure, const char*>, 10> map = {
        {std::make_pair(ConnectionFailure::None, "NONE"),
         std::make_pair(ConnectionFailure::HostResolution, "HOST_RESOLUTION"),
         std::make_pair(ConnectionFailure::ConnectionRefused, "CONNECTION_REFUSED"),
         std::make_pair(ConnectionFailure::Timeout, "TIMEOUT"),
         std::make_pair(ConnectionFailure::ClosedWithoutReply, "CLOSED_WITHOUT_REPLY"),
         std::make_pair(ConnectionFailure::HandshakeFailed, "HANDSHAKE_FAILED"),
         std::make_pair(ConnectionFailure::CertificateRejected, "CERTIFICATE_REJECTED"),
         std::make_pair(ConnectionFailure::UndecodableReply, "UNDECODABLE_REPLY"),
         std::make_pair(ConnectionFailure::RejectedRequest, "REJECTED_REQUEST"),
         std::make_pair(ConnectionFailure::Other, "OTHER")}};

    static constexpr size_t size = map.size();
    static_assert(size == 10, "ConnectionFailure enum size mismatch");
};

} // namespace detail

///
/// @brief Structured description of a failed exchange between a client and a server.
///
/// A ConnectionDiagnosis is populated by the transport that observed the failure, and carried
/// unchanged to the caller through ServerReply. It exists so that callers, and in particular the
/// ecFlow UI, can classify a failure without parsing the free-text error message.
///
/// A default-constructed instance describes "no failure"; ok() reports that state.
///
/// @invariant peer_protocol holds a value only when the protocol spoken by the peer has been
///            positively determined. An absent value means "not determined", never "plain".
///
struct ConnectionDiagnosis
{
    /// The protocol of the transport that actually ran, which is not necessarily the configured one
    Protocol client_protocol{Protocol::Plain};

    /// The class of failure observed
    ConnectionFailure failure{ConnectionFailure::None};

    /// The protocol the peer speaks, set only when positively determined
    std::optional<Protocol> peer_protocol{};

    /// Set when the observed failure is consistent with the peer speaking a different protocol
    bool protocol_mismatch_suspected{false};

    /// The host the request was addressed to
    std::string host;

    /// The port the request was addressed to
    std::string port;

    /// The verbatim low-level error message, retained for logging
    std::string detail;

    ///
    /// @brief Reports whether no failure has been recorded.
    ///
    /// @return true when no failure was observed; false otherwise
    ///
    [[nodiscard]] bool ok() const { return failure == ConnectionFailure::None; }

    ///
    /// @brief Reports whether a protocol mismatch is the established or the suspected cause.
    ///
    /// A mismatch is established when the peer protocol has been determined and differs from the
    /// protocol the client used; it is merely suspected when the observed failure is consistent
    /// with a mismatch but the peer protocol remains unknown.
    ///
    /// @return true when the failure is attributed to a protocol mismatch; false otherwise
    ///
    [[nodiscard]] bool is_protocol_mismatch() const {
        if (peer_protocol) {
            return peer_protocol.value() != client_protocol;
        }
        return protocol_mismatch_suspected;
    }

    ///
    /// @brief Restores the default state, describing "no failure".
    ///
    void clear() { *this = ConnectionDiagnosis{}; }
};

///
/// @brief Writes the designation of the given failure class.
///
/// @param[in,out] o      The stream to write to
/// @param[in] failure    The failure class to write
/// @return The stream given as @p o
///
inline std::ostream& operator<<(std::ostream& o, ConnectionFailure failure) {
    o << Enumerate<ConnectionFailure>::as_string(failure);
    return o;
}

///
/// @brief Renders a human-readable explanation of a failed exchange.
///
/// The wording is produced here, rather than by each caller, so that the command line client, the
/// Python bindings and the ecFlow UI all report a failure in the same terms.
///
/// @param[in] diagnosis The diagnosis to explain
/// @return A single-paragraph explanation; an empty string when @p diagnosis records no failure
///
std::string explain(const ConnectionDiagnosis& diagnosis);

} // namespace ecf

#endif /* ecflow_base_ConnectionDiagnosis_HPP */
