// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace ecf::service::aviso {

///
/// @brief Names the incompatibility with Aviso v1, and the last ecFlow release that supports it.
///
/// The text is appended to every error caused by an Aviso v1 configuration, so that users of Aviso v1 learn which
/// ecFlow release to use instead.
///
inline constexpr std::string_view unsupported_v1 =
    "Aviso v1 is no longer supported; ecFlow 5.19.x is the last release that supports Aviso v1";

///
/// @brief Requests notifications for one Aviso attribute.
///
/// All values are fully resolved, i.e. ecFlow variables have already been substituted.
///
class AvisoSubscribe {
public:
    ///
    /// @brief Creates a subscription request.
    ///
    /// @param path     Unique identifier of the Aviso attribute (the node path, followed by the attribute name).
    /// @param listener Listener configuration, as JSON (event and request).
    /// @param url      Address of the Aviso server.
    /// @param revision Last notification already processed; 0 means that only new notifications are requested.
    /// @param auth     Path to the credentials file.
    ///
    AvisoSubscribe(std::string_view path,
                   std::string_view listener,
                   std::string_view url,
                   std::uint64_t revision,
                   std::string_view auth)
        : path_{path},
          listener_{listener},
          url_{url},
          revision_{revision},
          auth_{auth} {}

    ///
    /// @name Accessors
    /// @brief Return the values given at creation (see the constructor).
    ///
    /// @{
    [[nodiscard]] const std::string& path() const { return path_; }
    [[nodiscard]] const std::string& listener() const { return listener_; }
    [[nodiscard]] const std::string& url() const { return url_; }
    [[nodiscard]] std::uint64_t revision() const { return revision_; }
    [[nodiscard]] const std::string& auth() const { return auth_; }
    /// @}

    ///
    /// @brief Writes a one-line description of the request, for logging.
    ///
    /// @param[in,out] os      The stream to write to.
    /// @param[in]     request The request to describe.
    /// @return The stream.
    ///
    friend std::ostream& operator<<(std::ostream& os, const AvisoSubscribe& request);

private:
    std::string path_;
    std::string listener_;
    std::string url_;
    std::uint64_t revision_;
    std::string auth_;
};

///
/// @brief Holds one notification received for an Aviso attribute.
///
class AvisoNotification {
public:
    ///
    /// @brief Creates a notification.
    ///
    /// @param event_type      Event type of the notification (e.g. mars).
    /// @param sequence        Sequence number of the notification, strictly increasing within its event type.
    /// @param identifier_json Identifier of the notification, as JSON.
    /// @param payload_json    Payload of the notification, as JSON.
    ///
    AvisoNotification(std::string_view event_type,
                      std::uint64_t sequence,
                      std::string_view identifier_json,
                      std::string_view payload_json)
        : event_type_{event_type},
          sequence_{sequence},
          identifier_json_{identifier_json},
          payload_json_{payload_json} {}

    ///
    /// @name Accessors
    /// @brief Return the values given at creation (see the constructor).
    ///
    /// @{
    [[nodiscard]] const std::string& event_type() const { return event_type_; }
    [[nodiscard]] std::uint64_t sequence() const { return sequence_; }
    [[nodiscard]] const std::string& identifier_json() const { return identifier_json_; }
    [[nodiscard]] const std::string& payload_json() const { return payload_json_; }
    /// @}

    ///
    /// @brief Writes a one-line description of the notification, for logging.
    ///
    /// @param[in,out] os           The stream to write to.
    /// @param[in]     notification The notification to describe.
    /// @return The stream.
    ///
    friend std::ostream& operator<<(std::ostream& os, const AvisoNotification& notification);

private:
    std::string event_type_;
    std::uint64_t sequence_;
    std::string identifier_json_;
    std::string payload_json_;
};

///
/// @brief Holds an error that prevents an Aviso attribute from receiving notifications.
///
class AvisoError {
public:
    ///
    /// @brief Creates an error.
    ///
    /// @param reason User-facing description of the error.
    ///
    explicit AvisoError(std::string_view reason)
        : reason_{reason} {}

    ///
    /// @name Accessors
    /// @brief Return the values given at creation (see the constructor).
    ///
    /// @{
    [[nodiscard]] const std::string& reason() const { return reason_; }
    /// @}

    ///
    /// @brief Writes a one-line description of the error, for logging.
    ///
    /// @param[in,out] os    The stream to write to.
    /// @param[in]     error The error to describe.
    /// @return The stream.
    ///
    friend std::ostream& operator<<(std::ostream& os, const AvisoError& error);

private:
    std::string reason_;
};

///
/// @brief Signals an attempt to (re)create the watch of an Aviso attribute, clearing any previous error.
///
/// The signal precedes any notification or error caused by the attempt, including the failure of the attempt itself.
///
class AvisoWatchStarted {
public:
    ///
    /// @brief Writes a one-line description of the signal, for logging.
    ///
    /// @param[in,out] os      The stream to write to.
    /// @param[in]     started The signal to describe.
    /// @return The stream.
    ///
    friend std::ostream& operator<<(std::ostream& os, const AvisoWatchStarted& started);
};

///
/// @brief Represents one outcome delivered to an Aviso attribute: a notification, an error, or a (re)started watch.
///
using AvisoResponse = std::variant<AvisoNotification, AvisoError, AvisoWatchStarted>;

///
/// @brief Writes a one-line description of the response, for logging.
///
/// @param[in,out] os       The stream to write to.
/// @param[in]     response The response to describe.
/// @return The stream.
///
std::ostream& operator<<(std::ostream& os, const AvisoResponse& response);

///
/// @brief Holds the watch configuration derived from the listener of an Aviso attribute.
///
struct Listener
{
    std::string event;       ///< The event type to watch (e.g. mars).
    std::string filter_json; ///< The filter, as JSON; empty when the listener has no request.
};

///
/// @brief Derives the watch configuration from the listener of an Aviso attribute.
///
/// The listener is a JSON object, with the mandatory string `event` and the optional object `request`. Each entry
/// of the request becomes a filter constraint: scalars are used as they are, and arrays become `{"in": [...]}`.
///
/// @param listener The listener, as JSON, without the surrounding single quotes.
/// @return The watch configuration.
/// @throws std::runtime_error if the listener is not valid.
///
Listener parse_listener(const std::string& listener);

///
/// @brief Holds HTTP Basic credentials.
///
struct BasicAuth
{
    std::string username; ///< The user name.
    std::string password; ///< The password.
};

///
/// @brief Holds a bearer token.
///
struct BearerAuth
{
    std::string token; ///< The token (the key of an ECMWF API credentials file).
};

///
/// @brief Holds the credentials used to contact the Aviso server, of either kind.
///
using Auth = std::variant<BasicAuth, BearerAuth>;

///
/// @brief Loads the credentials used to contact the Aviso server.
///
/// The credentials file is a JSON object, holding either `email` and `key` (the format of the ECMWF API credentials
/// file, `$HOME/.ecmwfapirc`), whose key is used as a bearer token while the email is ignored, or `username` and
/// `password` (HTTP Basic authentication); the key is preferred when both are present.
///
/// @param path The path to the credentials file, as given by the attribute option --auth.
/// @return The credentials.
/// @throws std::runtime_error if no path is given, the file cannot be loaded, or holds no usable credentials.
///
Auth load_auth(const std::string& path);

///
/// @brief Describes an error in a single line, suitable for the reason of an Aviso attribute.
///
/// The description names the incompatibility with Aviso v1, since an error is also what a v1 server causes. Single
/// quotes are replaced by back quotes, and line breaks by spaces, as the reason is stored within single quotes.
///
/// @param kind        The kind of error (e.g. transport, http).
/// @param http_status The HTTP status, or 0 when not applicable.
/// @param message     The error message.
/// @param request_id  The identifier of the failed request, when known.
/// @return The description.
///
std::string describe_error(std::string_view kind,
                           std::uint16_t http_status,
                           std::string_view message,
                           const std::optional<std::string>& request_id = std::nullopt);

} // namespace ecf::service::aviso
