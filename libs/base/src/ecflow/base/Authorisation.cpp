// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/Authorisation.hpp"

#include <algorithm>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/RequestContext.hpp"
#include "ecflow/base/Stats.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/base/stc/DefsCache.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/server/AuthorisationService.hpp"

namespace ecf {

AuthorisationMode authorisation_mode(const AbstractServer& server) {
    if (const auto defs = server.defs(); defs && defs->server_state().permissions()) {
        return AuthorisationMode::Permissions;
    }
    if (server.authorisation().uses_whitelist()) {
        return AuthorisationMode::WhiteList;
    }
    return AuthorisationMode::Unrestricted;
}

authorisation_t fail_closed(const std::function<authorisation_t()>& evaluate) noexcept {
    static const std::string reason = "Authorisation (user) failed, due to: an error while evaluating permissions";
    try {
        return evaluate();
    }
    catch (const std::exception& e) {
        return authorisation_t::failure(reason + " (" + e.what() + ")");
    }
    catch (...) {
        return authorisation_t::failure(reason);
    }
}

namespace {

///
/// @brief Evaluates a user request, telling an evaluated decision from an error while evaluating.
///
/// @param[in] command The request
/// @param[in] context The context of the request
/// @param[out] evaluated Set to true when the decision was evaluated, false when an error refused the request
/// @return The decision; a refusal when the evaluation throws. Never throws.
///
authorisation_t evaluate(const ClientToServerCmd& command, const RequestContext& context, bool& evaluated) noexcept {
    evaluated = false;
    return fail_closed([&]() {
        auto result = authorisation_mode(context.server()) == AuthorisationMode::Unrestricted
                          ? authorisation_t::success(true) // without a server attribute and a white list, all allowed
                          : command.authorise(context);
        evaluated   = true;
        return result;
    });
}

} // namespace

authorisation_t is_authorised(const ClientToServerCmd& command, const RequestContext& context) noexcept {
    bool evaluated = false;
    return evaluate(command, context, evaluated);
}

authorisation_t is_authorised_or_audited(const ClientToServerCmd& command, const RequestContext& context) noexcept {
    bool evaluated = false;
    auto result    = evaluate(command, context, evaluated);
    if (result.ok() || !evaluated) {
        return result; // an error while evaluating refuses the request, in every mode (R11.7)
    }
    try {
        if (!context.server().audit_only() || authorisation_mode(context.server()) != AuthorisationMode::Permissions) {
            return result;
        }
        if (is_role_denial(result.reason(), Role::Read)) {
            return result; // reads are enforced: no content of a node the requester cannot read is returned
        }
        // Audit-only mode (R12.3): the would-be denial is logged, counted, and the request executed
        if (!ecf::log_flushed(Log::WAR, audit_message(result.reason(), command))) {
            return result; // a would-be denial that cannot be recorded is enforced
        }
        context.server().stats().record_audited(command_name(command));
    }
    catch (...) {
        return result; // a would-be denial that cannot be recorded is enforced
    }
    return authorisation_t::success("Authorisation (user) not enforced: audit only");
}

namespace {

/// The beginning of the reason of every denial
const std::string denial_prefix = "Authorisation (user) failed, due to: ";

/// The beginning of the cause of a denial for want of a role, before the letter of the role
const std::string insufficient = "Insufficient permissions (role ";

/// The cause of a denial because a client handle belongs to another user
const std::string handle_owner = "the handle belongs to another user";

bool starts_with(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

} // namespace

std::string role_denial(Role role, const std::string& target) {
    return denial_prefix + insufficient + letter_of(role) + " required at " + target + ")";
}

std::string handle_owner_denial(const std::string& unreadable) {
    std::string reason = denial_prefix + handle_owner;
    if (!unreadable.empty()) {
        reason += "; " + insufficient + letter_of(Role::Read) + " required at " + unreadable + ")";
    }
    return reason;
}

bool is_role_denial(const std::string& reason, Role role) {
    // The role is named before the target, the only part of the reason chosen by the requester
    const std::string cause = insufficient + letter_of(role) + " required at ";
    return starts_with(reason, denial_prefix + cause) ||
           starts_with(reason, denial_prefix + handle_owner + "; " + cause);
}

void update_authorisation_statistics(AbstractServer& server) {
    auto& stats = server.stats();
    switch (authorisation_mode(server)) {
        case AuthorisationMode::Permissions:
            stats.authorisation_mode_ = server.audit_only() ? "permissions (audit only)" : "permissions";
            break;
        case AuthorisationMode::WhiteList:
            stats.authorisation_mode_ = "whitelist";
            break;
        case AuthorisationMode::Unrestricted:
            stats.authorisation_mode_ = "unrestricted";
            break;
    }
    stats.readable_sets_ = static_cast<unsigned int>(DefsCache::entries());
}

std::string command_name(const ClientToServerCmd& command) {
    return std::string("--") + command.theArg();
}

std::string audit_message(const std::string& reason, const ClientToServerCmd& command) {
    static const std::string refused = "Command not accepted, due to: ";
    std::string message              = rejection_message(reason, command);
    return "Audit only, executed although denied: " + message.substr(refused.size());
}

std::string rejection_message(const std::string& reason, const ClientToServerCmd& command) {
    std::string message = "Command not accepted, due to: ";
    message += reason;
    message += " [";
    message += command.print_short();
    message += " :";
    // A name outside the user name model (refused by the authentication) is shown in its printable form
    const auto& user = command.identity().username().value();
    std::string error;
    message += is_valid_user_name(user, error) ? user : printable_user_name(user);
    message += '@';
    message += command.hostname();
    message += ']';
    // One line: line breaks in the arguments of the request would otherwise split the record in the server log
    std::replace(message.begin(), message.end(), '\n', ' ');
    std::replace(message.begin(), message.end(), '\r', ' ');
    // Other control characters and invalid bytes (e.g. a user name outside the model, printed by the request) never
    // reach the reply or the log
    return printable_text(message);
}

} // namespace ecf
