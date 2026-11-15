// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

#include "ecflow/base/Cmd.hpp"
#include "ecflow/core/Child.hpp"
#include "ecflow/node/NodeFwd.hpp"

class AbstractServer;

namespace ecf {
class RequestContext;
}

// class PreAllocatedReply:
// This class pre allocates the replies back to the client
// This will help to reduce memory fragmentation.
// Since the commands are re-used those commands with state,
// should be cleared first
class PreAllocatedReply {
public:
    PreAllocatedReply() = delete;

    static STC_Cmd_ptr ok_cmd();
    static STC_Cmd_ptr block_client_server_halted_cmd();
    static STC_Cmd_ptr block_client_on_home_server_cmd();
    static STC_Cmd_ptr block_client_zombie_cmd(ecf::Child::ZombieType zt);
    static STC_Cmd_ptr delete_all_cmd();

    ///
    /// @brief Returns the reply holding the definitions of the server (--get, --get_state, --migrate without a path).
    ///
    /// The reply is built for the requester of the context, which the builder reads but does not keep.
    ///
    /// @param[in] save_edit_history True to include the edit history (--migrate)
    /// @param[in] context The context of the request being handled; valid during the call only
    /// @return The reply, shared with other requests: it is valid until the next reply of the same kind is built.
    ///
    static STC_Cmd_ptr defs_cmd(const ecf::RequestContext& context, bool save_edit_history);

    ///
    /// @brief Returns the reply holding one node of the server (--get, --get_state, --migrate with a path).
    ///
    /// The reply is built for the requester of the context, which the builder reads but does not keep.
    ///
    /// @param[in] node The node returned, with its descendants
    /// @param[in] context The context of the request being handled; valid during the call only
    /// @return The reply, shared with other requests: it is valid until the next reply of the same kind is built.
    ///
    static STC_Cmd_ptr node_cmd(const ecf::RequestContext& context, node_ptr node);

    ///
    /// @brief Returns the reply holding the statistics of the server, as a structure (--stats_server).
    ///
    /// The reply is built for the requester of the context, which the builder reads but does not keep.
    ///
    /// @param[in] context The context of the request being handled; valid during the call only
    /// @return The reply, shared with other requests: it is valid until the next reply of the same kind is built.
    ///
    static STC_Cmd_ptr stats_cmd(const ecf::RequestContext& context);

    ///
    /// @brief Returns the reply holding the names of the suites (--suites).
    ///
    /// The reply is built for the requester of the context, which the builder reads but does not keep.
    ///
    /// @param[in] context The context of the request being handled; valid during the call only
    /// @return The reply, shared with other requests: it is valid until the next reply of the same kind is built.
    ///
    static STC_Cmd_ptr suites_cmd(const ecf::RequestContext& context);

    ///
    /// @brief Returns the reply holding the zombies (--zombie_get).
    ///
    /// The reply is built for the requester of the context, which the builder reads but does not keep.
    ///
    /// @param[in] context The context of the request being handled; valid during the call only
    /// @return The reply, shared with other requests: it is valid until the next reply of the same kind is built.
    ///
    static STC_Cmd_ptr zombie_get_cmd(const ecf::RequestContext& context);
    static STC_Cmd_ptr error_cmd(const std::string& error_msg);

    ///
    /// @brief Returns the reply denying a request, an error reply of the kind PermissionDenied.
    ///
    /// @param[in] error_msg The message, naming the reason and the request (see ecf::rejection_message)
    /// @return The reply, shared with other requests: it is valid until the next error reply is built.
    ///
    static STC_Cmd_ptr denied_cmd(const std::string& error_msg);
    static STC_Cmd_ptr client_handle_cmd(int handle);

    ///
    /// @brief Returns the reply holding the client handles and their suites (--ch_suites).
    ///
    /// The reply is built for the requester of the context, which the builder reads but does not keep.
    ///
    /// @param[in] context The context of the request being handled; valid during the call only
    /// @return The reply, shared with other requests: it is valid until the next reply of the same kind is built.
    ///
    static STC_Cmd_ptr client_handle_suites_cmd(const ecf::RequestContext& context);
    static STC_Cmd_ptr string_cmd(const std::string& any_string);
    static STC_Cmd_ptr string_vec_cmd(const std::vector<std::string>&);
    static STC_Cmd_ptr server_load_cmd(const std::string& any_string);

    ///
    /// @brief Returns the reply holding whether the server changed since the given change numbers (--news).
    ///
    /// The reply is built for the requester of the context, which the builder reads but does not keep.
    ///
    /// @param[in] client_handle The client handle of the requester, or 0 for none
    /// @param[in] client_state_change_no The state change number held by the client
    /// @param[in] client_modify_change_no The modify change number held by the client
    /// @param[in] context The context of the request being handled; valid during the call only
    /// @return The reply, shared with other requests: it is valid until the next reply of the same kind is built.
    ///
    static STC_Cmd_ptr news_cmd(unsigned int client_handle,
                                unsigned int client_state_change_no,
                                unsigned int client_modify_change_no,
                                const ecf::RequestContext& context);

    ///
    /// @brief Returns the reply holding the changes since the given change numbers, or the full definitions (--sync).
    ///
    /// The reply is built for the requester of the context, which the builder reads but does not keep.
    ///
    /// @param[in] client_handle The client handle of the requester, or 0 for none
    /// @param[in] client_state_change_no The state change number held by the client
    /// @param[in] client_modify_change_no The modify change number held by the client
    /// @param[in] context The context of the request being handled; valid during the call only
    /// @return The reply, shared with other requests: it is valid until the next reply of the same kind is built.
    ///
    static STC_Cmd_ptr sync_cmd(unsigned int client_handle,
                                unsigned int client_state_change_no,
                                unsigned int client_modify_change_no,
                                const ecf::RequestContext& context);

    ///
    /// @brief Returns the reply holding the changes since the given change numbers, with the suite clocks
    /// (--sync_clock).
    ///
    /// The reply is built for the requester of the context, which the builder reads but does not keep.
    ///
    /// @param[in] client_handle The client handle of the requester, or 0 for none
    /// @param[in] client_state_change_no The state change number held by the client
    /// @param[in] client_modify_change_no The modify change number held by the client
    /// @param[in] context The context of the request being handled; valid during the call only
    /// @return The reply, shared with other requests: it is valid until the next reply of the same kind is built.
    ///
    static STC_Cmd_ptr sync_clock_cmd(unsigned int client_handle,
                                      unsigned int client_state_change_no,
                                      unsigned int client_modify_change_no,
                                      const ecf::RequestContext& context);

    ///
    /// @brief Returns the reply holding the full definitions of the handle (--sync_full).
    ///
    /// The reply is built for the requester of the context, which the builder reads but does not keep.
    ///
    /// @param[in] client_handle The client handle of the requester, or 0 for none
    /// @param[in] context The context of the request being handled; valid during the call only
    /// @return The reply, shared with other requests: it is valid until the next reply of the same kind is built.
    ///
    static STC_Cmd_ptr sync_full_cmd(unsigned int client_handle, const ecf::RequestContext& context);

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
