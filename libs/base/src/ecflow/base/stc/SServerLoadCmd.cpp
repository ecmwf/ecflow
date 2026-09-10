/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/base/stc/SServerLoadCmd.hpp"

#include <iostream>

#include "ecflow/base/Gnuplot.hpp"

using namespace ecf;

bool SServerLoadCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<SServerLoadCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (log_file_path_ != the_rhs->log_file_path()) {
        return false;
    }
    return ServerToClientCmd::equals(rhs);
}

std::string SServerLoadCmd::print() const {
    std::string os;
    os += "cmd:SServerLoadCmd [ ";
    os += log_file_path_;
    os += " ]";
    return os;
}

bool SServerLoadCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const {
    if (debug) {
        std::cout << "  SServerLoadCmd::handle_server_response log_file_path = " << log_file_path() << "\n";
    }
    Gnuplot gnuplot(log_file_path(), server_reply.host(), server_reply.port());
    gnuplot.show_server_load();
    return true;
}

std::ostream& operator<<(std::ostream& os, const SServerLoadCmd& c) {
    os << c.print();
    return os;
}
