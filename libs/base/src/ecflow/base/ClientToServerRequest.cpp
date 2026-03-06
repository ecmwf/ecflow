/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/ClientToServerRequest.hpp"

#include <stdexcept>

#include "ecflow/base/Authentication.hpp"
#include "ecflow/base/Authorisation.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"

namespace {

/**
 * @brief Builds the message returned to the client when a command is rejected.
 *
 * The originating user name is appended to the message to help identify who attempted the (rejected) connection.
 *
 * @param reason The reason for the rejection.
 * @param identity The identity of the user who attempted the command.
 * @return A string containing the rejection message.
 */
std::string make_rejection_message(const std::string& reason, const ecf::Identity& identity) {
    return std::string{"Command not accepted, due to: "} + reason + " [" + identity.username().value() + "]";
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
            return PreAllocatedReply::error_cmd(make_rejection_message(result.reason(), cmd_->identity()));
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
            return PreAllocatedReply::error_cmd(make_rejection_message(result.reason(), cmd_->identity()));
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
