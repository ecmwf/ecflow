/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/base/stc/BlockClientZombieCmd.hpp"

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
    if (debug) {
        std::cout << "  BlockClientZombieCmd::handle_server_response BLOCK_CLIENT_ZOMBIE "
                  << ecf::Child::to_string(zombie_type_) << "\n";
    }
    server_reply.set_block_client_zombie_detected(); // requires further work, by ClientInvoker
    return false;                                    // false means fall through and try again, i.e BLOCK client
}

bool BlockClientZombieCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<BlockClientZombieCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (zombie_type_ != the_rhs->zombie_type()) {
        return false;
    }
    return ServerToClientCmd::equals(rhs);
}

std::ostream& operator<<(std::ostream& os, const BlockClientZombieCmd& c) {
    os << c.print();
    return os;
}
