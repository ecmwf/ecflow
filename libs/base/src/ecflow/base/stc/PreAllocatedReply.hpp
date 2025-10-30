/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_stc_PreAllocatedReply_HPP
#define ecflow_base_stc_PreAllocatedReply_HPP

#include <string>

#include "ecflow/base/Cmd.hpp"
#include "ecflow/core/Child.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/node/NodeFwd.hpp"

class AbstractServer;

// class PreAllocatedReply:
// This class pre allocates the replies back to the client
// This will help to reduce memory fragmentation.
// Since the commands are re-used those commands with state,
// should be cleared first
class PreAllocatedReply {
public:
    PreAllocatedReply()                                    = delete;
    PreAllocatedReply(const PreAllocatedReply&)            = delete;
    PreAllocatedReply& operator=(const PreAllocatedReply&) = delete;

    static STC_Cmd_ptr ok_cmd();
    static STC_Cmd_ptr block_client_server_halted_cmd();
    static STC_Cmd_ptr block_client_on_home_server_cmd();
    static STC_Cmd_ptr block_client_zombie_cmd(ecf::Child::ZombieType zt);
    static STC_Cmd_ptr delete_all_cmd();

    static STC_Cmd_ptr defs_cmd(const ecf::Identity& identity, AbstractServer*, bool save_edit_history);
    static STC_Cmd_ptr node_cmd(const ecf::Identity& identity, AbstractServer*, node_ptr);
    static STC_Cmd_ptr stats_cmd(AbstractServer*);
    static STC_Cmd_ptr suites_cmd(AbstractServer*);
    static STC_Cmd_ptr zombie_get_cmd(AbstractServer*);
    static STC_Cmd_ptr error_cmd(const std::string& error_msg);
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
                                const ecf::Identity& identity,
                                AbstractServer* as);
    static STC_Cmd_ptr sync_clock_cmd(unsigned int client_handle,
                                      unsigned int client_state_change_no,
                                      unsigned int client_modify_change_no,
                                      const ecf::Identity& identity,
                                      AbstractServer* as);
    static STC_Cmd_ptr sync_full_cmd(unsigned int client_handle, const ecf::Identity& identity, AbstractServer* as);

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

#endif /* ecflow_base_stc_PreAllocatedReply_HPP */
