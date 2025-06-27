/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/stc/GroupSTCCmd.hpp"

#include "ecflow/base/WhyCmd.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"

using namespace ecf;
using namespace std;
using namespace boost;

std::string GroupSTCCmd::print() const {
    return "cmd:GroupSTCCmd";
}

bool GroupSTCCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<GroupSTCCmd*>(rhs);
    if (!the_rhs)
        return false;

    const std::vector<STC_Cmd_ptr>& rhsCmdVec = the_rhs->cmdVec();
    if (cmdVec_.size() != rhsCmdVec.size())
        return false;

    for (size_t i = 0; i < cmdVec_.size(); i++) {
        if (!cmdVec_[i]->equals(rhsCmdVec[i].get())) {
            return false;
        }
    }

    return ServerToClientCmd::equals(rhs);
}

bool GroupSTCCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const {
    if (debug)
        std::cout << "  GroupSTCCmd::handle_server_response\n";

    bool ret_flag = true;
    for (STC_Cmd_ptr subCmd : cmdVec_) {
        if (!subCmd->handle_server_response(server_reply, cts_cmd, debug))
            ret_flag = false; // one of the commands failed
    }
    if (!server_reply.cli())
        return ret_flag;

    /// CLI called from the command line.
    /// This assumes the DefsCmd::handle_server_response() | SNodeCmd::handle_server_response has been called
    /// this will populate ServerReply with the defs/node returned from the server
    defs_ptr defs = server_reply.client_defs();
    node_ptr node = server_reply.client_node();

    if (defs.get() || node.get()) {
        if (debug)
            std::cout << "   GroupSTCCmd::handle_server_response *get* | *sync* | *sync_full* called\n";

        /// client --group="get; show"         # where get will call DefsCmd will return defs, from the server
        /// client --group="get; show state"   # where get will call DefsCmd will return defs, from the server
        /// client --group="get /s1; show state"   # where get will call DefsCmd will return defs, from the server
        /// client --group="sync_full; show"       # similar to get return defs, from the server
        /// client --group="sync 1 0 0; show"      # where sync will call SyncCmd will return defs, from the server
        ///                                        # will return those suites with handle 1

        // Print out the data that was received from server. as a part of get request.
        // The server cannot do a show, it MUST be done at the Client side
        // The show request is only valid if the out bound request to the server
        PrintStyle::Type_t style = cts_cmd->show_style();
        if (style != PrintStyle::NOTHING) {
            if (debug)
                std::cout << "   GroupSTCCmd::handle_server_response *show* was called " << PrintStyle::to_string(style)
                          << "\n";
            PrintStyle print_style(style);
            if (defs.get()) {

                /// Auto generate externs, before writing to standard out. This can be expensive since
                /// All the trigger references need to to be resolved. & AST need to be created first
                /// The old spirit based parsing is horrendously, slow. Can't use Spirit QI, till IBM support it
                if (!PrintStyle::is_persist_style(cts_cmd->show_style())) {
                    defs->auto_add_externs();
                }

                ecf::write_t(std::cout, *defs, cts_cmd->show_style());
            }
            else {
                if (node.get()) {
                    Suite* suite = node->isSuite();
                    if (suite)
                        std::cout << *suite << "\n";
                    Family* fam = node->isFamily();
                    if (fam)
                        std::cout << *fam << "\n";
                    Task* task = node->isTask();
                    if (task)
                        std::cout << *task << "\n";
                }
            }
        }
    }

    std::string nodePath;
    if (cts_cmd->why_cmd(nodePath) && defs.get()) {
        if (debug)
            std::cout << "  GroupSTCCmd::handle_server_response *why* was called\n";

        /// client --group="get; why"          # where get will call DefsCmd will return defs, from the server
        /// client --group="get; why <path>"   # where get will call DefsCmd will return defs, from the server
        WhyCmd cmd(defs, nodePath);
        std::cout << cmd.why() << "\n";
    }

    return ret_flag;
}

void GroupSTCCmd::addChild(STC_Cmd_ptr childCmd) {
    LOG_ASSERT(childCmd.get(), ""); // Dont add NULL children
    cmdVec_.push_back(childCmd);
}

// these two must be opposite of each other
bool GroupSTCCmd::ok() const {
    for (const auto& i : cmdVec_) {
        if (!i->ok())
            return false; // if child is ErrorCmd will return false
    }
    return true;
}

void GroupSTCCmd::cleanup() {
    /// After the command has run this function can be used to reclaim memory
    for (auto& cmd : cmdVec_) {
        cmd->cleanup();
    }
}

std::string GroupSTCCmd::error() const {
    std::string ret;
    for (const auto& i : cmdVec_) {
        std::string error_str = i->error();
        if (!error_str.empty()) {
            ret += error_str;
            ret += "\n";
        }
    }
    return ret;
}

std::ostream& operator<<(std::ostream& os, const GroupSTCCmd& c) {
    os << c.print();
    return os;
}
