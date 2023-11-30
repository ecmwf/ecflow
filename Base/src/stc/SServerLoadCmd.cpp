/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "SServerLoadCmd.hpp"

#include <iostream>

#include "Gnuplot.hpp"

using namespace ecf;
using namespace std;
using namespace boost;

bool SServerLoadCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<SServerLoadCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (log_file_path_ != the_rhs->log_file_path())
        return false;
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
    if (debug)
        cout << "  SServerLoadCmd::handle_server_response log_file_path = " << log_file_path() << "\n";
    Gnuplot gnuplot(log_file_path(), server_reply.host(), server_reply.port());
    gnuplot.show_server_load();
    return true;
}

std::ostream& operator<<(std::ostream& os, const SServerLoadCmd& c) {
    os << c.print();
    return os;
}
