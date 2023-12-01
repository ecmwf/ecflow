/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "EditHistoryMgr.hpp"

#include "AbstractServer.hpp"
#include "ClientToServerCmd.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/node/Defs.hpp"

using namespace std;
using namespace boost;
using namespace ecf;

EditHistoryMgr::EditHistoryMgr(const ClientToServerCmd* c, AbstractServer* a)
    : cts_cmd_(c),
      as_(a),
      state_change_no_(Ecf::state_change_no()),
      modify_change_no_(Ecf::modify_change_no()) {
    assert(cts_cmd_->edit_history_nodes_.empty());
    assert(cts_cmd_->edit_history_node_paths_.empty());
}

EditHistoryMgr::~EditHistoryMgr() {
    // check if state changed
    if (state_change_no_ != Ecf::state_change_no() || modify_change_no_ != Ecf::modify_change_no()) {

        // Ignore child commands for edit history, where only interested in user commands
        if (!cts_cmd_->task_cmd()) {

            // *ONLY* record edit history to commands that change the data model
            // Otherwise we will end up making a data model change for read only commands
            // If there has been a change in defs state then the command must return true from isWrite
            if (cts_cmd_->isWrite()) {
                cts_cmd_->add_edit_history(as_->defs().get());
            }
            else {
                // Read only command, that is making data model changes, oops ?
                // Can happen with check pt command, when ecf_home can't be written to, (exceptional)
                // i.e set late flag( when saving takes more the 30 seconds) *OR* Flag::CHECKPT_ERROR | Flag::LOG_ERROR
                //       even though its read only command. In which case is_mutable() should return true.
                if (!cts_cmd_->is_mutable()) {
                    std::string ss;
                    cts_cmd_->print(ss);
                    cout << "cmd " << ss << " should return true from isWrite() ******************\n";
                    cout << "Read only command is making data changes to defs ?????\n";
                    cout << "Ecf::state_change_no() " << Ecf::state_change_no() << " Ecf::modify_change_no() "
                         << Ecf::modify_change_no() << "\n";
                    cout << "state_change_no_       " << state_change_no_ << " modify_change_no_       "
                         << modify_change_no_ << "\n";
                    cout.flush();
                }
            }
        }
    }
}
