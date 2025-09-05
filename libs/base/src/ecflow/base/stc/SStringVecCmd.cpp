/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/stc/SStringVecCmd.hpp"

#include <iostream>

using namespace std;
using namespace boost;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SStringVecCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<SStringVecCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (vec_ != the_rhs->get_string_vec()) {
        return false;
    }
    return ServerToClientCmd::equals(rhs);
}

std::string SStringVecCmd::print() const {
    return "cmd:SStringVecCmd ";
}

bool SStringVecCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const {
    if (debug) {
        cout << "  SStringVecCmd::handle_server_response str.size()= " << vec_.size() << "\n";
    }
    if (server_reply.cli()) {
        for (const auto& i : vec_) {
            std::cout << i << "\n";
        }
    }
    else {
        server_reply.set_string_vec(vec_);
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const SStringVecCmd& c) {
    os << c.print();
    return os;
}
