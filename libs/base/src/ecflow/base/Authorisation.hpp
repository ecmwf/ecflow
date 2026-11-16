// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <string>

#include "ecflow/attribute/Permissions.hpp"
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
/// @brief Decides whether a user request is executed, applying the audit-only mode (R12.3).
///
/// Evaluates the request as is_authorised does. In audit-only mode, when the server uses Permissions attributes, a
/// request they deny is executed nevertheless: the denial is written to the server log as a warning (see
/// audit_message), and success is returned. The exceptions, refused as in Permissions mode:
///
/// - a denial for want of the role r (see is_role_denial), so that no content of a node is returned to a requester
///   who cannot read it; the evaluation stops at the first requirement not met, and every command checks its
///   requirements of r before the others, so a request lacking r at any target is refused; a client handle command
///   checks r at its suites together with the owner of the handle, and names both when both fail;
/// - an error while evaluating the permissions, or while recording the would-be denial (R11.7).
///
/// The white-list mode is never audited.
///
/// @param[in] command The request
/// @param[in] context The context of the request
/// @return Success when the request is allowed, or denied but audited; otherwise a failure holding the reason. Never
///         throws.
///
authorisation_t is_authorised_or_audited(const ClientToServerCmd& command, const RequestContext& context) noexcept;

///
/// @brief Returns the reason of a denial for want of a role at a target, in Permissions mode.
///
/// @param[in] role The role missing
/// @param[in] target The path at which the role is missing: "/" for the server, or a node
/// @return The reason, e.g. `Authorisation (user) failed, due to: Insufficient permissions (role x required at /s/t)`.
///
std::string role_denial(Role role, const std::string& target);

///
/// @brief Returns the reason of a denial because a client handle belongs to another user.
///
/// @param[in] unreadable The suite at which the requester also lacks the role r, as "/<suite>"; empty when r is not
///            missing
/// @return The reason, e.g. `Authorisation (user) failed, due to: the handle belongs to another user`, or, when r is
///         also missing, `Authorisation (user) failed, due to: the handle belongs to another user; Insufficient
///         permissions (role r required at /u)`.
///
std::string handle_owner_denial(const std::string& unreadable = {});

///
/// @brief Tells whether the reason of a denial is the want of a given role, as built by role_denial, or by
///        handle_owner_denial when r is also missing.
///
/// @param[in] reason The reason of the denial
/// @param[in] role The role
/// @return True when the reason names the role as missing.
///
bool is_role_denial(const std::string& reason, Role role);

///
/// @brief Returns the log record of a request executed in audit-only mode although the attributes deny it.
///
/// The record names the reason, the request and its requester, as rejection_message does, e.g.
/// `Audit only, executed although denied: Authorisation (user) failed, due to: Insufficient permissions (role x
/// required at /s/t) [--suspend /s/t :bob@host]`.
///
/// @param[in] reason Why the attributes deny the request
/// @param[in] command The request
/// @return The record, on one line.
///
std::string audit_message(const std::string& reason, const ClientToServerCmd& command);

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
