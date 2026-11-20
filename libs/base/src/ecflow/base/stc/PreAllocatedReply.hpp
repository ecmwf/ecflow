// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

#include "ecflow/base/Cmd.hpp"
#include "ecflow/core/Child.hpp"
#include "ecflow/core/ErrorCode.hpp"
#include "ecflow/node/NodeFwd.hpp"

class AbstractServer;

///
/// @brief Reuses server replies to reduce allocation while resetting per-reply state.
///
/// GroupSTCCmd::addChild snapshots error replies before reuse; other reply types remain shared.
///
class PreAllocatedReply {
public:
    PreAllocatedReply() = delete;

    static STC_Cmd_ptr ok_cmd();
    static STC_Cmd_ptr block_client_server_halted_cmd();
    static STC_Cmd_ptr block_client_on_home_server_cmd();
    static STC_Cmd_ptr block_client_zombie_cmd(ecf::Child::ZombieType zt);
    static STC_Cmd_ptr delete_all_cmd();

    static STC_Cmd_ptr defs_cmd(AbstractServer*, bool save_edit_history);
    static STC_Cmd_ptr node_cmd(AbstractServer*, node_ptr);
    static STC_Cmd_ptr stats_cmd(AbstractServer*);
    static STC_Cmd_ptr suites_cmd(AbstractServer*);
    static STC_Cmd_ptr zombie_get_cmd(AbstractServer*);

    ///
    /// @brief Reuses an unclassified error reply and resets any previous numeric code to zero.
    ///
    /// @param[in] error_msg Server diagnostic to transmit; must be non-empty.
    /// @return Shared reply object; copy it before storing multiple group children.
    /// @throws std::bad_alloc if storing the diagnostic fails.
    ///
    static STC_Cmd_ptr error_cmd(const std::string& error_msg);

    ///
    /// @brief Reuses a classified error reply, resetting its previous diagnostic and code.
    ///
    /// @param[in] error_msg Server diagnostic to transmit; must be non-empty.
    /// @param[in] code Stable numeric classification of the failure.
    /// @return Shared reply object; copy it before storing multiple group children.
    /// @throws std::bad_alloc if storing the diagnostic fails.
    ///
    static STC_Cmd_ptr error_cmd(const std::string& error_msg, ecf::ErrorCode code);

    ///
    /// @brief Reuses an error reply while preserving a raw, possibly unknown numeric code.
    ///
    /// @param[in] error_msg Server diagnostic to transmit; must be non-empty.
    /// @param[in] code Raw numeric classification of the failure.
    /// @return Shared reply object; copy it before storing multiple group children.
    /// @throws std::bad_alloc if storing the diagnostic fails.
    ///
    static STC_Cmd_ptr error_cmd(const std::string& error_msg, int code);
    static STC_Cmd_ptr client_handle_cmd(int handle);
    static STC_Cmd_ptr client_handle_suites_cmd(AbstractServer*);
    static STC_Cmd_ptr string_cmd(const std::string& any_string);
    static STC_Cmd_ptr string_vec_cmd(const std::vector<std::string>&);
    static STC_Cmd_ptr server_load_cmd(const std::string& any_string);
    static STC_Cmd_ptr news_cmd(unsigned int client_handle,
                                unsigned int client_state_change_no,
                                unsigned int client_modify_change_no,
                                AbstractServer* as);
    static STC_Cmd_ptr sync_cmd(unsigned int client_handle,
                                unsigned int client_state_change_no,
                                unsigned int client_modify_change_no,
                                AbstractServer* as);
    static STC_Cmd_ptr sync_clock_cmd(unsigned int client_handle,
                                      unsigned int client_state_change_no,
                                      unsigned int client_modify_change_no,
                                      AbstractServer* as);
    static STC_Cmd_ptr sync_full_cmd(unsigned int client_handle, AbstractServer* as);

private:
    static STC_Cmd_ptr stc_cmd_;
    static STC_Cmd_ptr defs_cmd_;
    static STC_Cmd_ptr node_cmd_;
    static STC_Cmd_ptr stats_cmd_;
    static STC_Cmd_ptr suites_cmd_;
    static STC_Cmd_ptr zombie_get_cmd_;
    static STC_Cmd_ptr error_cmd_;
    static STC_Cmd_ptr client_handle_cmd_;
    static STC_Cmd_ptr client_handle_suites_cmd_;
    static STC_Cmd_ptr string_cmd_;
    static STC_Cmd_ptr string_vec_cmd_;
    static STC_Cmd_ptr server_load_cmd_;
    static STC_Cmd_ptr news_cmd_;
    static STC_Cmd_ptr sync_cmd_;
    static STC_Cmd_ptr block_client_zombie_cmd_;
};
