/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/stc/SClientHandleCmd.hpp"

#include <iostream>

#include "ecflow/core/Converter.hpp"

using namespace ecf;
using namespace std;

bool SClientHandleCmd::equals(ServerToClientCmd* rhs) const {
    return (dynamic_cast<SClientHandleCmd*>(rhs)) ? ServerToClientCmd::equals(rhs) : false;
}

std::string SClientHandleCmd::print() const {
    string os;
    os += "cmd:SClientHandleCmd [ ";
    os += ecf::convert_to<std::string>(handle_);
    os += " ]";
    return os;
}

bool SClientHandleCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const {
    if (debug) {
        cout << "  SClientHandleCmd::handle_server_response handle_ = " << handle_ << "\n";
    }
    server_reply.set_client_handle(handle_);
    return true;
}

std::ostream& operator<<(std::ostream& os, const SClientHandleCmd& c) {
    os << c.print();
    return os;
}
