/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/EditHistoryMgr.hpp"

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/node/Defs.hpp"

EditHistoryMgr::EditHistoryMgr(const ClientToServerCmd& cmd, const AbstractServer& server)
    : cts_cmd_(cmd),
      as_(server),
      state_change_no_(Ecf::state_change_no()),
      modify_change_no_(Ecf::modify_change_no()) {
    assert(cts_cmd_.edit_history_nodes_.empty());
    assert(cts_cmd_.edit_history_node_paths_.empty());
}

EditHistoryMgr::~EditHistoryMgr() {
    // Detect a state/modify change
    if (state_change_no_ != Ecf::state_change_no() || modify_change_no_ != Ecf::modify_change_no()) {

        // Ignore child commands for edit history -- only user commands are considered
        if (!cts_cmd_.task_cmd()) {

            // The goal is to *ONLY* record edit history to commands that already change the data model
            // Otherwise, all read-only commands would actually become write commands!
            if (cts_cmd_.isWrite()) {
                cts_cmd_.add_edit_history(as_.defs().get());
            }
            else {
                // Read-only commands should not produce any data model changes!
                //
                // Exception to the rule:
                //   This can happen in very exceptional cases (e.g. when the ECF_HOME_home cannot be written to,
                //   the checkpt command eventually sets a late or error flag because technically saving is late or was
                //   not successful).
                //   These commands are identified as being "mutable", which is gives them a "get out of free" card.
                //
                if (!cts_cmd_.is_mutable()) {
                    std::string ss;
                    cts_cmd_.print(ss);
                    std::cout << "cmd " << ss << " should return true from isWrite() ******************\n";
                    std::cout << "Read only command is making data changes to defs ?????\n";
                    std::cout << "Ecf::state_change_no() " << Ecf::state_change_no() << " Ecf::modify_change_no() "
                              << Ecf::modify_change_no() << "\n";
                    std::cout << "state_change_no_       " << state_change_no_ << " modify_change_no_       "
                              << modify_change_no_ << "\n";
                    std::cout.flush();
                }
            }
        }
    }
}
