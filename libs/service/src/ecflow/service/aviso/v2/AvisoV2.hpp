// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace ecf::service::aviso::v2 {

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

} // namespace ecf::service::aviso::v2
