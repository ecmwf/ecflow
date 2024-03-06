/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/stc/SStringCmd.hpp"

#include <iostream>

using namespace std;
using namespace boost;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SStringCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<SStringCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (str_ != the_rhs->get_string())
        return false;
    return ServerToClientCmd::equals(rhs);
}

std::string SStringCmd::print() const {
    return "cmd:SStringCmd ";
}

bool SStringCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const {
    if (debug)
        cout << "  SStringCmd::handle_server_response str.size()= " << str_.size() << "\n";
    if (server_reply.cli())
        // The following uses std::endl to ensure the output is flushed.
        // This is necessary when called from the Python API, otherwise the output cannot be captured systematically.
        std::cout << str_ << std::endl;
    else
        server_reply.set_string(str_);
    return true;
}

std::ostream& operator<<(std::ostream& os, const SStringCmd& c) {
    os << c.print();
    return os;
}
