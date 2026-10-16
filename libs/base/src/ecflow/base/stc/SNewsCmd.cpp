// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/stc/SNewsCmd.hpp"

#include <iostream>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/node/ClientSuiteMgr.hpp"
#include "ecflow/node/Defs.hpp"

using namespace ecf;

SNewsCmd::SNewsCmd(unsigned int client_handle,
                   unsigned int client_state_change_no,
                   unsigned int client_modify_change_no,
                   AbstractServer* as) {
    init(client_handle, client_state_change_no, client_modify_change_no, as);
}

namespace ecf {

NewsOutcome evaluate_news(unsigned int client_handle,
                          unsigned int client_state_change_no,
                          unsigned int client_modify_change_no,
                          const ClientSuiteMgr& client_suite_mgr) {
    /// This method assumes that all users see the same content !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // =====================================================================================
    // The code to determine changes here must also relate to SSyncCmd
    // ======================================================================================

    if (client_handle == 0) {

        // Here Ecf::modify_change_no() and  Ecf::state_change_no() represent the max change numbers over *all* the
        // suites

        /// *** The client_modify_change_no and client_state_change_no should always be trailing the server
        /// *** i.e the value should be less or equal to server. However if::
        /// ***   o/ Server **dies** we can get the case, where client numbers are greater than server numbers.
        /// ***   o/ Server changes number overflows, since it unsigned, and re-start's with 0
        /// *** When no handle are involved, we can get by with a full sync
        /// *** Note: whenever the server starts, the state and modify numbers start from zero
        if ((client_modify_change_no > Ecf::modify_change_no()) || (client_state_change_no > Ecf::state_change_no())) {

            return {ServerReply::DO_FULL_SYNC,
                    MESSAGE(" [server(" << Ecf::state_change_no() << "," << Ecf::modify_change_no()
                                        << ") : client no > server no ! :DO_FULL_SYNC]")};
        }

        if (client_modify_change_no < Ecf::modify_change_no()) {

            return {ServerReply::NEWS,
                    MESSAGE(" [server(" << Ecf::state_change_no() << "," << Ecf::modify_change_no()
                                        << ") : *Large* scale changes("
                                        << (Ecf::modify_change_no() - client_modify_change_no) << ") :NEWS]")};
        }

        if (client_state_change_no < Ecf::state_change_no()) {

            return {ServerReply::NEWS,
                    MESSAGE(" [server(" << Ecf::state_change_no() << "," << Ecf::modify_change_no()
                                        << ") : *Small* scale changes("
                                        << (Ecf::state_change_no() - client_state_change_no) << ") :NEWS]")};
        }

        return {ServerReply::NO_NEWS, " [:NO_NEWS]"};
    }

    // =============================================================================================
    // Handle used: Determine the max modify and state change no, for suites in our handle
    // =============================================================================================

    /// *** If we cannot find the handle, then it may be that the server died ?
    /// *** It is up to the client to do a full sync *including* re-registering suites
    if (!client_suite_mgr.valid_handle(client_handle)) {

        return {ServerReply::DO_FULL_SYNC,
                MESSAGE(" [server(" << Ecf::state_change_no() << "," << Ecf::modify_change_no()
                                    << ") : Cannot find handle(" << client_handle << ") :DO_FULL_SYNC]")};
    }

    /// *** The client_modify_change_no and client_state_change_no should always be trailing the server
    /// *** i.e the value should be less or equal to server. However if
    /// ***   o/ Server **dies** we can get the case, where client numbers are greater than server numbers.
    /// ***   o/ Server changes number overflows, since it unsigned, and re-start's with 0
    /// *** we can get the case, where client numbers are greater than server numbers and also the
    /// *** handle will not exist in the server,
    /// *** It is up to the client to do a full sync *including* re-registering suites
    /// *** Note: whenever the server starts, the state and modify numbers start from zero
    unsigned int max_client_handle_modify_change_no = 0;
    unsigned int max_client_handle_state_change_no  = 0;
    client_suite_mgr.max_change_no(
        client_handle, max_client_handle_state_change_no, max_client_handle_modify_change_no);

    if ((client_modify_change_no > max_client_handle_modify_change_no) ||
        (client_state_change_no > max_client_handle_state_change_no)) {

        return {ServerReply::DO_FULL_SYNC,
                MESSAGE(" [server handle(" << max_client_handle_state_change_no << ","
                                           << max_client_handle_modify_change_no << ")  server("
                                           << Ecf::state_change_no() << "," << Ecf::modify_change_no()
                                           << ") : client no > server no ! :DO_FULL_SYNC]")};
    }

    /// Changes where user adds a new handle/auto adds/removes require a full update, but only for changed handle
    if (client_suite_mgr.handle_changed(client_handle)) {

        return {ServerReply::NEWS,
                MESSAGE(" [server handle("
                        << max_client_handle_state_change_no << "," << max_client_handle_modify_change_no << ") server("
                        << Ecf::state_change_no() << "," << Ecf::modify_change_no()
                        << ") : *Large* scale changes (new handle or suites added or removed) :NEWS]")};
    }

    // The client handle represents a subset of the suites.
    if (client_modify_change_no < max_client_handle_modify_change_no) {

        return {ServerReply::NEWS,
                MESSAGE(" [server handle(" << max_client_handle_state_change_no << ","
                                           << max_client_handle_modify_change_no << ") server("
                                           << Ecf::state_change_no() << "," << Ecf::modify_change_no()
                                           << ") : *Large* scale changes :NEWS]")};
    }

    // This should also reflect changes made just to the defs(state/suspended) and also the server state
    if (client_state_change_no < max_client_handle_state_change_no) {

        return {ServerReply::NEWS,
                MESSAGE(" [server handle(" << max_client_handle_state_change_no << ","
                                           << max_client_handle_modify_change_no << ") server("
                                           << Ecf::state_change_no() << "," << Ecf::modify_change_no()
                                           << ") : *Small* scale changes :NEWS]")};
    }

    return {ServerReply::NO_NEWS, " [:NO_NEWS]"};
}

} // namespace ecf

/// Called in the server
void SNewsCmd::init(unsigned int client_handle,
                    unsigned int client_state_change_no,
                    unsigned int client_modify_change_no,
                    AbstractServer* as) {
    auto [news, annotation] = ecf::evaluate_news(
        client_handle, client_state_change_no, client_modify_change_no, as->defs()->client_suite_mgr());
    news_       = news;
    annotation_ = annotation;
}

/// Called in the client
bool SNewsCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr /*cts_cmd*/, bool debug) const {
    if (debug) {
        std::cout << "  SNewsCmd::handle_server_response news_ = " << news_ << "\n";
    }
    server_reply.set_news(news_);
    return true;
}

bool SNewsCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<SNewsCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (news_ != the_rhs->news()) {
        return false;
    }
    return ServerToClientCmd::equals(rhs);
}

std::string SNewsCmd::print() const {
    return MESSAGE("cmd:SNewsCmd [ " << news_ << " ] ");
}

std::ostream& operator<<(std::ostream& os, const SNewsCmd& c) {
    os << c.print();
    return os;
}
