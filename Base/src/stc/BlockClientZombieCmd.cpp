/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Cmd
// Author      : Avi
// Revision    : $Revision: #7 $
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "BlockClientZombieCmd.hpp"

#include <iostream>

std::string BlockClientZombieCmd::print() const {
    switch (zombie_type_) {
        case ecf::Child::USER:
            return "cmd:BlockClientZombieCmd: user";
        case ecf::Child::PATH:
            return "cmd:BlockClientZombieCmd: path";
        case ecf::Child::ECF:
            return "cmd:BlockClientZombieCmd: ecf";
        case ecf::Child::ECF_PID:
            return "cmd:BlockClientZombieCmd: ecf_pid";
        case ecf::Child::ECF_PID_PASSWD:
            return "cmd:BlockClientZombieCmd: ecf_pid_passwd";
        case ecf::Child::ECF_PASSWD:
            return "cmd:BlockClientZombieCmd: ecf_passwd";
        case ecf::Child::NOT_SET:
            return "cmd:BlockClientZombieCmd: not_set";
    }
    assert(false); // unknown command
    return "cmd:Unknown??";
}

// Client context
bool BlockClientZombieCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const {
    if (debug)
        std::cout << "  BlockClientZombieCmd::handle_server_response BLOCK_CLIENT_ZOMBIE "
                  << ecf::Child::to_string(zombie_type_) << "\n";
    server_reply.set_block_client_zombie_detected(); // requires further work, by ClientInvoker
    return false;                                    // false means fall through and try again, i.e BLOCK client
}

bool BlockClientZombieCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<BlockClientZombieCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (zombie_type_ != the_rhs->zombie_type())
        return false;
    return ServerToClientCmd::equals(rhs);
}

std::ostream& operator<<(std::ostream& os, const BlockClientZombieCmd& c) {
    os << c.print();
    return os;
}
