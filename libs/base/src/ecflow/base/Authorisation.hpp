// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <string>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/core/Result.hpp"

class ClientToServerCmd;

namespace ecf {

class RequestContext;

using authorisation_t = Result<bool>;

///
/// @brief Identifies how the server authorises user requests.
///
enum class AuthorisationMode {
    Unrestricted, ///< No server attribute and no white list: every request is allowed
    WhiteList,    ///< No server attribute, and a white list file loaded: the white list applies
    Permissions   ///< A server Permissions attribute: the attributes apply, and the white list is not used
};

///
/// @brief Returns the mode in which the server authorises user requests.
///
/// The server attribute selects the Permissions mode; otherwise a loaded white list selects the white-list mode, and
/// the server is unrestricted when there is neither.
///
/// @param[in] server The server
/// @return The mode.
///
AuthorisationMode authorisation_mode(const AbstractServer& server);

///
/// @brief Runs an evaluation of permissions, turning any error into a refusal.
///
/// @param[in] evaluate The evaluation
/// @return The result of the evaluation, or a refusal when it throws, whatever it throws. Never throws.
///
authorisation_t fail_closed(const std::function<authorisation_t()>& evaluate) noexcept;

///
/// @brief Decides whether a user request is allowed.
///
/// Task requests are not subject to the authorisation of users. An error while evaluating the permissions refuses the
/// request.
///
/// @param[in] command The request
/// @param[in] context The context of the request
/// @return Success when the request is allowed; otherwise a failure holding the reason. Never throws.
///
authorisation_t is_authorised(const ClientToServerCmd& command, const RequestContext& context) noexcept;

///
/// @brief Returns the message of a rejected request, as returned to the client and written to the server log.
///
/// The message names the reason, the request and its requester (D5.29), e.g.
/// `Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions (role x required at
/// /s/t) [--suspend /s/t :bob@host]`.
///
/// @param[in] reason Why the request was rejected
/// @param[in] command The request rejected
/// @return The message, on one line: line breaks are replaced by spaces.
///
std::string rejection_message(const std::string& reason, const ClientToServerCmd& command);

} // namespace ecf
