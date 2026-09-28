// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/ClientToServerRequest.hpp"

#include <stdexcept>

#include "ecflow/base/Authentication.hpp"
#include "ecflow/base/Authorisation.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"

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

} // namespace

STC_Cmd_ptr ClientToServerRequest::handleRequest(AbstractServer* as) const {
    if (cmd_.get()) {
        // Perform Authentication (i.e. user/task identity) control
        std::cout << "*** [DBG] ClientToServerRequest::handleRequest: Command received, processing request..."
                  << std::endl;

        if (auto result = ecf::is_authentic(*cmd_, *as); !result.ok()) {
            std::cout
                << "*** [DBG] ClientToServerRequest::handleRequest: Command failed, due to authentication failure..."
                << std::endl;
            std::cout << "*** [DBG] ClientToServerRequest::handleRequest: Command failed, with username ["
                      << cmd_->identity().username().value() << "]" << std::endl;
            return PreAllocatedReply::error_cmd(make_rejection_message(result.reason(), *cmd_));
        }

        std::cout << "*** [DBG] ClientToServerRequest::handleRequest: Command accepted: authentication..." << std::endl;

        // Perform Authorisation (i.e. access rules) control
        if (auto result = ecf::is_authorised(*cmd_, *as); result.ok()) {
            std::cout << "*** [DBG] ClientToServerRequest::handleRequest: Command accepted: authorization..."
                      << std::endl;
            std::cout << "*** [DBG] ClientToServerRequest::handleRequest: Command accepted, with command ["
                      << cmd_->print_short() << "]" << std::endl;
            std::cout << "*** [DBG] ClientToServerRequest::handleRequest: Command accepted, with username ["
                      << cmd_->identity().username().value() << "]" << std::endl;
            return cmd_->handleRequest(as);
        }
        else {
            std::cout
                << "*** [DBG] ClientToServerRequest::handleRequest: Command failed, due to authorisation failure..."
                << std::endl;
            std::cout << "*** [DBG] ClientToServerRequest::handleRequest: Command failed, with username ["
                      << cmd_->identity().username().value() << "]" << std::endl;
            // The command is not accepted, return an error
            return PreAllocatedReply::error_cmd(make_rejection_message(result.reason(), *cmd_));
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
