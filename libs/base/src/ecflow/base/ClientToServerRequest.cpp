// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/ClientToServerRequest.hpp"

#include <stdexcept>

#include "ecflow/base/Authentication.hpp"
#include "ecflow/base/Authorisation.hpp"
#include "ecflow/base/RequestContext.hpp"
#include "ecflow/base/Stats.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Log.hpp"

STC_Cmd_ptr ClientToServerRequest::handleRequest(AbstractServer* as) const {
    if (cmd_.get()) {
        // Perform Authentication (i.e. user/task identity) control
        if (auto result = ecf::is_authentic(*cmd_, *as); !result.ok()) {
            return PreAllocatedReply::error_cmd(ecf::rejection_message(result.reason(), *cmd_));
        }

        // The context of this request, from the identity established by authentication
        const ecf::RequestContext context(cmd_->identity(), *as);

        // Perform Authorisation (i.e. access rules) control; a request not authorised is denied
        if (auto result = ecf::is_authorised_or_audited(*cmd_, context); !result.ok()) {
            ecf::LogFlusher flusher; // the message, logged by the reply, is written at once
            as->stats().record_denial(ecf::command_name(*cmd_));
            return PreAllocatedReply::denied_cmd(ecf::rejection_message(result.reason(), *cmd_));
        }
        return cmd_->handleRequest(as, context);
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
