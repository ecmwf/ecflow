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
/// @brief The EditHistoryMgr class manages the history of changes performed by the commands.
///
/// The goal is to determine if there was an actual state change performed by the command,
/// and if there was adds them to the history associated to the nodes.
/// The history only considers changes performed by user commands.
///
/// This also performs validation of the changes, and ensures that only commands that
/// return ClientToServerCmd::isWrite() true actually make changes to the node tree.
///
/// This class is used in the following sequence of steps:
///   1) An object of EditHistoryMgr is created before the command is handled and retrieves before state/modify counters
///   2) The server handles the command and the command makes changes to the node tree (if any)
///        n.b. this must update the list of edit_history_nodes/edit_history_node_paths in the comamnd itself
///   3) The dtor of EditHistoryMgr retrieves after state/modify counters
///      and if there is a change in either of the counters, then adds the edit history to the nodes in the command
///
class EditHistoryMgr {
public:
    ///
    /// @brief Construct an EditHistoryMgr object.
    ///
    /// @param cmd The command to track the history for.
    /// @param server The server handling the request
    ///
    EditHistoryMgr(const ClientToServerCmd& cmd, const AbstractServer& server);

    EditHistoryMgr()                                 = delete;
    EditHistoryMgr(const EditHistoryMgr&)            = delete;
    EditHistoryMgr& operator=(const EditHistoryMgr&) = delete;
    EditHistoryMgr(EditHistoryMgr&&)                 = delete;
    EditHistoryMgr& operator=(EditHistoryMgr&&)      = delete;

    ~EditHistoryMgr();

private:
    const ClientToServerCmd& cts_cmd_;
    const AbstractServer& as_;

    mutable unsigned int state_change_no_;
    mutable unsigned int modify_change_no_;
};

#endif /* ecflow_base_cts_EditHistoryMgr_HPP */
