/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/base/stc/ZombieGetCmd.hpp"

#include <iostream>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/core/Converter.hpp"

ZombieGetCmd::ZombieGetCmd(AbstractServer* as) {
    init(as);
}

// called in the server
void ZombieGetCmd::init(AbstractServer* as) {
    zombies_.clear();
    as->zombie_ctrl().get(zombies_);
}

bool ZombieGetCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<ZombieGetCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    return ServerToClientCmd::equals(rhs);
}

std::string ZombieGetCmd::print() const {
    std::string os;
    os += "cmd:ZombieGetCmd [ ";
    os += ecf::convert_to<std::string>(zombies_.size());
    os += " ]";
    return os;
}

// Called in client
bool ZombieGetCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const {
    if (debug) {
        std::cout << "  ZombieGetCmd::handle_server_response zombies.size() = " << zombies_.size() << "\n";
    }

    if (server_reply.cli()) {
        std::cout << Zombie::pretty_print(zombies_);
    }
    else {
        if (debug) {
            std::cout << Zombie::pretty_print(zombies_);
        }
        server_reply.set_zombies(zombies_);
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const ZombieGetCmd& c) {
    os << c.print();
    return os;
}
