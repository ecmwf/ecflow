/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_EditHistoryMgr_HPP
#define ecflow_base_cts_EditHistoryMgr_HPP

class ClientToServerCmd;
class AbstractServer;

///
/// \brief This class manages the edit history, for the commands.
/// It determines if there was a state change, if there was, it adds edit
/// history to the stored nodes.
/// Additionally we check that if there was an edit then the command must
/// return ClientToServerCmd::isWrite() true.
///

class EditHistoryMgr {
public:
    EditHistoryMgr()                      = delete;
    EditHistoryMgr(const EditHistoryMgr&) = delete;
    EditHistoryMgr(const ClientToServerCmd*, AbstractServer*);
    ~EditHistoryMgr();

    EditHistoryMgr& operator=(const EditHistoryMgr&) = delete;

private:
    const ClientToServerCmd* cts_cmd_;
    AbstractServer* as_;
    mutable unsigned int state_change_no_;  // detect state change in defs
    mutable unsigned int modify_change_no_; // detect state change in defs
};

#endif /* ecflow_base_cts_EditHistoryMgr_HPP */
