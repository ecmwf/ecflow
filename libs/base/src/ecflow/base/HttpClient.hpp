
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

/*
 * The following enforces the HTTP client to run on a single thread.
 * It acts as a flag to the HttpLibrary.hpp file.
 */
#define ECF_HTTP_CUSTOM_THREAD_POOL_COUNT 1

#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/Connection.hpp"
#include "ecflow/base/ConnectionDiagnosis.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"
#include "ecflow/core/HttpLibrary.hpp"

namespace ecf::http {

enum class Status {
    Unknown                       = -1,
    Continue                      = 100, // Informational 1xx
    SwitchingProtocol             = 101,
    Processing                    = 102,
    EarlyHints                    = 103,
    OK                            = 200, // Success 2xx
    Created                       = 201,
    Accepted                      = 202,
    NonAuthoritativeInformation   = 203,
    NoContent                     = 204,
    ResetContent                  = 205,
    PartialContent                = 206,
    MultiStatus                   = 207,
    AlreadyReported               = 208,
    IMUsed                        = 226,
    MultipleChoice                = 300, // Redirection 3xx
    MovedPermanently              = 301,
    Found                         = 302,
    SeeOther                      = 303,
    NotModified                   = 304,
    UseProxy                      = 305,
    unused                        = 306,
    TemporaryRedirect             = 307,
    PermanentRedirect             = 308,
    BadRequest                    = 400, // Client Error 4xx
    Unauthorized                  = 401,
    PaymentRequired               = 402,
    Forbidden                     = 403,
    NotFound                      = 404,
    MethodNotAllowed              = 405,
    NotAcceptable                 = 406,
    ProxyAuthenticationRequired   = 407,
    RequestTimeout                = 408,
    Conflict                      = 409,
    Gone                          = 410,
    LengthRequired                = 411,
    PreconditionFailed            = 412,
    PayloadTooLarge               = 413,
    URITooLong                    = 414,
    UnsupportedMediaType          = 415,
    RangeNotSatisfiable           = 416,
    ExpectationFailed             = 417,
    ImATeapot                     = 418,
    MisdirectedRequest            = 421,
    UnprocessableEntity           = 422,
    Locked                        = 423,
    FailedDependency              = 424,
    TooEarly                      = 425,
    UpgradeRequired               = 426,
    PreconditionRequired          = 428,
    TooManyRequests               = 429,
    RequestHeaderFieldsTooLarge   = 431,
    UnavailableForLegalReasons    = 451,
    InternalServerError           = 500, // Server Error 5xx
    NotImplemented                = 501,
    BadGateway                    = 502,
    ServiceUnavailable            = 503,
    GatewayTimeout                = 504,
    HTTPVersionNotSupported       = 505,
    VariantAlsoNegotiates         = 506,
    InsufficientStorage           = 507,
    LoopDetected                  = 508,
    NotExtended                   = 510,
    NetworkAuthenticationRequired = 511
};

inline std::string to_http_reason(Status s) {
    if (s == Status::Unknown) {
        return "Could not contact server";
    }
    return httplib::detail::status_message(static_cast<std::underlying_type_t<Status>>(s));
}

inline std::ostream& operator<<(std::ostream& o, Status s) {
    o << to_http_reason(s) << " (" << static_cast<std::underlying_type_t<Status>>(s) << ")";
    return o;
}

///
/// @brief Classifies an error reported by the underlying HTTP library.
///
/// The HTTP library reports every transport-level problem through a single error enumeration.
/// Mapping it here is what separates "no server is listening" from "something is listening, but it
/// is not speaking HTTP", which the library itself does not distinguish in its error message.
///
/// @param[in] error The error reported by the HTTP library; must not be httplib::Error::Success
/// @return The matching failure class
///
ecf::ConnectionFailure classify_httplib_error(httplib::Error error);

} // namespace ecf::http

///
/// \brief  This class acts as an HTTP client
///

class HttpClient {
public:
    /// Constructor starts the asynchronous connect operation.
    ///
    /// @brief Prepares the HTTP request to the given endpoint.
    ///
    /// @param[in] cmd_ptr   The request to send; must not be null
    /// @param[in] scheme    Either "http" or "https"
    /// @param[in] host      The server host name
    /// @param[in] port      The server port
    /// @param[in] timeout   The connect, read and write timeout, in seconds
    /// @param[out] diagnosis Storage for the diagnosis of the exchange, which outlives this client;
    ///                      when null, the diagnosis is kept internally and lost on destruction
    /// @throws std::runtime_error if @p cmd_ptr is null, or if HTTPS is requested by an ecFlow
    ///         built without SSL support
    ///
    HttpClient(Cmd_ptr cmd_ptr,
               const std::string& scheme,
               const std::string& host,
               const std::string& port,
               int timeout                         = 120,
               ecf::ConnectionDiagnosis* diagnosis = nullptr);

    std::string url() const { return base_url_; }

    void run();

    void set_x_realm(const std::string& realm) { headers_.emplace("X-Realm", realm); }
    void set_x_username(const std::string& username) { headers_.emplace("X-Username", username); }
    void set_x_roles(const std::string& roles) { headers_.emplace("X-Roles", roles); }
    void set_x_secret(const std::string& secret) { headers_.emplace("X-Secret", secret); }

    void set_authorisation_basic(const std::string& username, const std::string& password) {
        client_.set_basic_auth(username, password);
    }
    void set_authorisation_bearer(const std::string& token) { client_.set_bearer_token_auth(token); }

    /// Client side, get the server response, handles reply from server
    /// Returns true if all is ok, else false if further client action is required
    /// will throw std::runtime_error for errors
    bool handle_server_response(ServerReply&, bool debug) const;

    ///
    /// @brief Provides the diagnosis of the exchange performed by this client.
    ///
    /// @return The diagnosis of the exchange
    ///
    const ecf::ConnectionDiagnosis& diagnosis() const { return diagnosis_; }

private:
    /// Records a failure, together with the endpoint and the originating message.
    void record_failure(ecf::ConnectionFailure failure, const std::string& detail);

    std::string scheme_; /// the scheme to use
    std::string host_;   /// the servers name
    std::string port_;   /// the port on the server
    std::string base_url_;
    httplib::Client client_;
    httplib::Headers headers_;

    httplib::Response response_;
    ecf::http::Status status_ = ecf::http::Status::Unknown;
    std::string reason_       = "";

    ClientToServerRequest outbound_request_;  /// The request we will send to the server
    ServerToClientResponse inbound_response_; /// The response we get back from the server

    ecf::ConnectionDiagnosis owned_diagnosis_; /// Used when the caller provides no storage
    ecf::ConnectionDiagnosis& diagnosis_;      /// The diagnosis of the exchange, populated on failure
};

#endif /* ecflow_base_HttpClient_HPP */
