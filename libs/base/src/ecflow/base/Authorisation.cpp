// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/Authorisation.hpp"

#include <algorithm>

#include "ecflow/base/RequestContext.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/core/Identity.hpp"
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

authorisation_t is_authorised(const ClientToServerCmd& command, const RequestContext& context) noexcept {
    return fail_closed([&]() {
        if (authorisation_mode(context.server()) == AuthorisationMode::Unrestricted) {
            // Without a server attribute and a white list, every request is allowed
            return authorisation_t::success(true);
        }
        return command.authorise(context);
    });
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
