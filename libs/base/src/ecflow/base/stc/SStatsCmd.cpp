// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/stc/SStatsCmd.hpp"

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Visibility.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////

SStatsCmd::SStatsCmd(AbstractServer* as) {
    init(as);
}

void SStatsCmd::init(AbstractServer* as, const ecf::Visibility* visibility) {
    as->stats().update_for_serialisation();
    stats_               = as->stats();
    stats_.no_of_suites_ = 0;
    for (const auto& suite : as->defs()->suites()) {
        if (!visibility || !visibility->stops_at(*suite)) {
            ++stats_.no_of_suites_;
        }
    }
}

bool SStatsCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<SStatsCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    return ServerToClientCmd::equals(rhs);
}

std::string SStatsCmd::print() const {
    return "cmd:SStatsCmd ";
}

bool SStatsCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr /*cts_cmd*/, bool debug) const {
    if (debug) {
        std::cout << "  SStatsCmd::handle_server_response\n";
    }
    if (server_reply.cli()) {
        stats_.show();
    }
    else {
        server_reply.set_stats(stats_);
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const SStatsCmd& c) {
    os << c.print();
    return os;
}
