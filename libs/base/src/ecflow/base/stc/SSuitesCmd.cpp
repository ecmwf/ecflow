/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/stc/SSuitesCmd.hpp"

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////

SSuitesCmd::SSuitesCmd(AbstractServer* as) {
    init(as);
}

void SSuitesCmd::init(AbstractServer* as) {
    // This command can be re-used hence clear existing data members
    suites_.clear();

    const std::vector<suite_ptr>& suiteVec = as->defs()->suiteVec();
    size_t suite_vec_size                  = suiteVec.size();
    suites_.reserve(suite_vec_size);
    for (size_t i = 0; i < suite_vec_size; i++) {
        suites_.push_back(suiteVec[i]->name());
    }
}

bool SSuitesCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<SSuitesCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    return ServerToClientCmd::equals(rhs);
}

std::string SSuitesCmd::print() const {
    return "cmd:SSuitesCmd ";
}

bool SSuitesCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const {
    if (debug) {
        std::cout << "  SSuitesCmd::handle_server_response\n";
    }

    if (server_reply.cli() && !cts_cmd->group_cmd()) {
        /// This Could be part of a group command, hence ONLY if NOT group command
        if (suites_.empty()) {
            cout << "No suites\n";
        }

        size_t max_suite_name_size = 0;
        size_t the_size            = suites_.size();
        for (size_t i = 0; i < the_size; i++) {
            max_suite_name_size = std::max(max_suite_name_size, suites_[i].size());
        }
        max_suite_name_size += 1;

        int newline_at = 4;
        for (size_t i = 0; i < the_size; i++) {
            cout << left << setw(max_suite_name_size) << suites_[i];
            if (i != 0 && (i % newline_at) == 0) {
                cout << "\n";
                newline_at += 5;
            }
        }
        cout << "\n";
    }
    else {
        server_reply.set_string_vec(suites_);
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const SSuitesCmd& c) {
    os << c.print();
    return os;
}
