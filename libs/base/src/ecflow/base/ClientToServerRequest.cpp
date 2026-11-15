// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/ClientToServerRequest.hpp"

#include <stdexcept>

#include "ecflow/base/Authentication.hpp"
#include "ecflow/base/Authorisation.hpp"
#include "ecflow/base/RequestContext.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Log.hpp"

namespace {

/// @brief Builds the message returned to the client, and written to the log, when a command is rejected.
///
/// The rejected command, the originating user name and the client host are appended to the message, as
/// `[--<command> :<user>@<host>]`, so that the record identifies who attempted the command, from where, and which
/// command was turned down.
///
/// @param[in] reason The reason for the rejection.
/// @param[in] cmd The command that was rejected.
/// @return A string containing the rejection message.
///
std::string make_rejection_message(const std::string& reason, const ClientToServerCmd& cmd) {
    std::string message = "Command not accepted, due to: ";
    message += reason;
    message += " [";
    message += cmd.print_short();
    message += " :";
    message += cmd.identity().username().value();
    message += '@';
    message += cmd.hostname();
    message += ']';
    return message;
}

// The option of a command, e.g. "--suspend", without its arguments
std::string option_of(const ClientToServerCmd& cmd) {
    const auto text = cmd.print_short();
    return text.substr(0, text.find_first_of(" ="));
}

// Records a denied request in the server log, at a bounded rate, and returns the reply denying it, whose message names
// neither nodes nor arguments
STC_Cmd_ptr deny(const ClientToServerCmd& cmd, AbstractServer& as, const std::string& reason) {
    const auto& user       = cmd.identity().username().value();
    const auto option      = option_of(cmd);
    std::size_t suppressed = 0;
    bool overflow          = false;
    ecf::LogFlusher flusher; // the record is written at once, as the records of handled requests
    if (as.denial_log().record(user + " " + option, ecf::DenialLog::Clock::now(), suppressed, overflow)) {
        ecf::log(ecf::Log::ERR,
                 ecf::DenialLog::format(user, cmd.hostname(), option, cmd.print_short(), reason, suppressed, overflow));
    }
    return PreAllocatedReply::denied_cmd("Permission denied: " + option);
}

} // namespace

STC_Cmd_ptr ClientToServerRequest::handleRequest(AbstractServer* as) const {
    if (cmd_.get()) {
        // Perform Authentication (i.e. user/task identity) control
        if (auto result = ecf::is_authentic(*cmd_, *as); !result.ok()) {
            return PreAllocatedReply::error_cmd(make_rejection_message(result.reason(), *cmd_));
        }

        // Perform Authorisation (i.e. access rules) control
        if (auto result = ecf::is_authorised(*cmd_, *as); result.ok()) {
            // The context of this request, from the identity established by authentication
            const ecf::RequestContext context(cmd_->identity(), *as);
            return cmd_->handleRequest(as, context);
        }
        else {
            // The command is not authorised: a denial
            return deny(*cmd_, *as, result.reason());
        }
    }

    /// means programming error somewhere
    throw std::runtime_error("ClientToServerRequest::handleRequest: Cannot send a NULL request to the server !");
}

std::ostream& ClientToServerRequest::print(std::ostream& os) const {
    if (cmd_.get()) {
        os << cmd_->print_short(); // avoid printing hundreds of paths in the command
        return os;
    }
    return os << "NULL request";
}

bool ClientToServerRequest::operator==(const ClientToServerRequest& rhs) const {
    if (!cmd_.get() && !rhs.cmd_.get()) {
        return true;
    }
    if (cmd_.get() && !rhs.cmd_.get()) {
        return false;
    }
    if (!cmd_.get() && rhs.cmd_.get()) {
        return false;
    }
    return (cmd_->equals(rhs.cmd_.get()));
}

std::ostream& operator<<(std::ostream& os, const ClientToServerRequest& d) {
    return d.print(os);
}
