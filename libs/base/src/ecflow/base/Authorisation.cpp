// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/Authorisation.hpp"

#include "ecflow/base/DenialLog.hpp"
#include "ecflow/base/RequestContext.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/server/AuthorisationService.hpp"

namespace ecf {

namespace {

// The option of a command, e.g. "--suspend", without its arguments
std::string option_of(const ClientToServerCmd& cmd) {
    const auto text = cmd.print_short();
    return text.substr(0, text.find_first_of(" ="));
}

} // namespace

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

std::string record_denial(const ClientToServerCmd& command, AbstractServer& server, const std::string& reason) {
    const auto& user       = command.identity().username().value();
    const auto option      = option_of(command);
    std::size_t suppressed = 0;
    bool overflow          = false;
    LogFlusher flusher; // the record is written at once, as the records of handled requests
    if (server.denial_log().record(user + " " + option, DenialLog::Clock::now(), suppressed, overflow)) {
        log(Log::ERR,
            DenialLog::format(user, command.hostname(), option, command.print_short(), reason, suppressed, overflow));
    }
    return "Permission denied: " + option;
}

} // namespace ecf
