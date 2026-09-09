/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/base/stc/SClientHandleCmd.hpp"

#include <iostream>

#include "ecflow/core/Converter.hpp"

using namespace ecf;

bool SClientHandleCmd::equals(ServerToClientCmd* rhs) const {
    return (dynamic_cast<SClientHandleCmd*>(rhs)) ? ServerToClientCmd::equals(rhs) : false;
}

std::string SClientHandleCmd::print() const {
    std::string os;
    os += "cmd:SClientHandleCmd [ ";
    os += ecf::convert_to<std::string>(handle_);
    os += " ]";
    return os;
}

bool SClientHandleCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const {
    if (debug) {
        std::cout << "  SClientHandleCmd::handle_server_response handle_ = " << handle_ << "\n";
    }
    server_reply.set_client_handle(handle_);
    return true;
}

std::ostream& operator<<(std::ostream& os, const SClientHandleCmd& c) {
    os << c.print();
    return os;
}
