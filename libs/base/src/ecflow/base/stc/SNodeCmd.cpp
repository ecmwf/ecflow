/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/stc/SNodeCmd.hpp"

#include <iostream>
#include <stdexcept>

#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"

using namespace std;
using namespace boost;

//=====================================================================================
// This command returns the requested node back to the client
// Note: In the case where defs has not been loaded, it can be NULL

SNodeCmd::SNodeCmd(AbstractServer* as, node_ptr node) {
    init(as, node);
}

void SNodeCmd::init(AbstractServer* as, node_ptr node) {
    the_node_str_.clear();
    if (node.get()) {
        the_node_str_ = ecf::as_string(node, PrintStyle::NET);
    }
}

node_ptr SNodeCmd::get_node_ptr(std::string& error_msg) const {
    return Node::create(the_node_str_, error_msg);
}

bool SNodeCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<SNodeCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (!ServerToClientCmd::equals(rhs))
        return false;
    return true;
}

std::string SNodeCmd::print() const {
    std::string os;
    os += "cmd:SNodeCmd [ ";
    std::string error_msg;
    node_ptr node = get_node_ptr(error_msg);
    if (node.get())
        os += node->absNodePath();
    else
        os += "node == NULL";
    os += " ]";
    return os;
}

// Called in client
bool SNodeCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const {
    if (debug)
        std::cout << "  SNodeCmd::handle_server_response\n";

    std::string error_msg;
    node_ptr node = get_node_ptr(error_msg);
    if (!node.get()) {
        std::string ss;
        ss += "SNodeCmd::handle_server_response: Error Node could not be retrieved from server. Request ";
        ss += cts_cmd->print_short();
        ss += " failed.\n";
        ss += error_msg;
        throw std::runtime_error(ss);
    }

    if (server_reply.cli() && !cts_cmd->group_cmd()) {
        /// This Could be part of a group command, hence ONLY show Node if NOT group command

        auto style = cts_cmd->show_style();

        Suite* suite = node->isSuite();
        if (suite) {
            if (!PrintStyle::is_persist_style(style)) {
                /// Auto generate externs, before writing to standard out. This can be expensive since
                /// All the trigger references need to to be resolved. & AST need to be created first
                /// The old spirit based parsing, horrendously, slow. Can't use Spirit QI, till IBM pull support it
                ///
                /// We need a fabricate a defs to show the externs, used by the suite
                Defs defs;
                defs.addSuite(std::dynamic_pointer_cast<Suite>(node));
                defs.auto_add_externs();
                std::cout << ecf::as_string(defs, style);
                return true;
            }

            // with defs_state MIGRATE on --load we will recover the state.
            if (PrintStyle::is_persist_style(style)) {
                std::cout << "defs_state " << PrintStyle::to_string(style) << "\n"; // see ECFLOW-1233
            }
            std::cout << ecf::as_string(*suite, style) << "\n";
            return true;
        }
        cout << ecf::as_string(node, style) << "\n";
    }
    else {
        server_reply.set_client_node(node);
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const SNodeCmd& c) {
    os << c.print();
    return os;
}
