// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/stc/PreAllocatedReply.hpp"

#include <chrono>
#include <cstdint>
#include <utility>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/RequestContext.hpp"
#include "ecflow/base/Stats.hpp"
#include "ecflow/base/stc/BlockClientZombieCmd.hpp"
#include "ecflow/base/stc/DefsCmd.hpp"
#include "ecflow/base/stc/ErrorCmd.hpp"
#include "ecflow/base/stc/GroupSTCCmd.hpp"
#include "ecflow/base/stc/SClientHandleCmd.hpp"
#include "ecflow/base/stc/SClientHandleSuitesCmd.hpp"
#include "ecflow/base/stc/SNewsCmd.hpp"
#include "ecflow/base/stc/SNodeCmd.hpp"
#include "ecflow/base/stc/SServerLoadCmd.hpp"
#include "ecflow/base/stc/SStatsCmd.hpp"
#include "ecflow/base/stc/SStringCmd.hpp"
#include "ecflow/base/stc/SStringVecCmd.hpp"
#include "ecflow/base/stc/SSuitesCmd.hpp"
#include "ecflow/base/stc/SSyncCmd.hpp"
#include "ecflow/base/stc/StcCmd.hpp"
#include "ecflow/base/stc/ZombieGetCmd.hpp"

STC_Cmd_ptr PreAllocatedReply::stc_cmd_                  = std::make_shared<StcCmd>();
STC_Cmd_ptr PreAllocatedReply::defs_cmd_                 = std::make_shared<DefsCmd>();
STC_Cmd_ptr PreAllocatedReply::node_cmd_                 = std::make_shared<SNodeCmd>();
STC_Cmd_ptr PreAllocatedReply::stats_cmd_                = std::make_shared<SStatsCmd>();
STC_Cmd_ptr PreAllocatedReply::suites_cmd_               = std::make_shared<SSuitesCmd>();
STC_Cmd_ptr PreAllocatedReply::zombie_get_cmd_           = std::make_shared<ZombieGetCmd>();
STC_Cmd_ptr PreAllocatedReply::error_cmd_                = std::make_shared<ErrorCmd>();
STC_Cmd_ptr PreAllocatedReply::client_handle_cmd_        = std::make_shared<SClientHandleCmd>();
STC_Cmd_ptr PreAllocatedReply::client_handle_suites_cmd_ = std::make_shared<SClientHandleSuitesCmd>();
STC_Cmd_ptr PreAllocatedReply::string_cmd_               = std::make_shared<SStringCmd>();
STC_Cmd_ptr PreAllocatedReply::string_vec_cmd_           = std::make_shared<SStringVecCmd>();
STC_Cmd_ptr PreAllocatedReply::server_load_cmd_          = std::make_shared<SServerLoadCmd>();
STC_Cmd_ptr PreAllocatedReply::news_cmd_                 = std::make_shared<SNewsCmd>();
STC_Cmd_ptr PreAllocatedReply::sync_cmd_                 = std::make_shared<SSyncCmd>();
STC_Cmd_ptr PreAllocatedReply::block_client_zombie_cmd_  = std::make_shared<BlockClientZombieCmd>();

namespace {

// The nodes the requester can read, to filter a reply; null when Permissions do not apply, so that the reply is built
// as before (R5.4)
std::shared_ptr<const ecf::Visibility> filter_of(const ecf::RequestContext& context) {
    auto visibility = context.visibility();
    return visibility->applies() ? visibility : nullptr;
}

///
/// @brief Builds a reply that depends on the nodes its requester can read, counting its cost when filtered (R12.5).
///
/// @param[in] context The context of the request
/// @param[in] build Builds the reply, given the visibility of the requester (nullptr when nothing is filtered)
///
template <typename BUILD>
void build_filtered(const ecf::RequestContext& context, BUILD build) {
    const auto start      = std::chrono::steady_clock::now();
    const auto visibility = filter_of(context);
    build(visibility.get());
    if (visibility) {
        const auto elapsed = std::chrono::steady_clock::now() - start;
        context.server().stats().record_filtered_reply(
            static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()));
    }
}

} // namespace

STC_Cmd_ptr PreAllocatedReply::ok_cmd() {
    auto* cmd = dynamic_cast<StcCmd*>(stc_cmd_.get());
    cmd->init(StcCmd::OK);
    return stc_cmd_;
}
STC_Cmd_ptr PreAllocatedReply::block_client_server_halted_cmd() {
    auto* cmd = dynamic_cast<StcCmd*>(stc_cmd_.get());
    cmd->init(StcCmd::BLOCK_CLIENT_SERVER_HALTED);
    return stc_cmd_;
}
STC_Cmd_ptr PreAllocatedReply::block_client_on_home_server_cmd() {
    auto* cmd = dynamic_cast<StcCmd*>(stc_cmd_.get());
    cmd->init(StcCmd::BLOCK_CLIENT_ON_HOME_SERVER);
    return stc_cmd_;
}
STC_Cmd_ptr PreAllocatedReply::delete_all_cmd() {
    auto* cmd = dynamic_cast<StcCmd*>(stc_cmd_.get());
    cmd->init(StcCmd::DELETE_ALL);
    return stc_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::block_client_zombie_cmd(ecf::Child::ZombieType zt) {
    auto* cmd = dynamic_cast<BlockClientZombieCmd*>(block_client_zombie_cmd_.get());
    cmd->init(zt);
    return block_client_zombie_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::defs_cmd(const ecf::RequestContext& context, bool save_edit_history) {
    AbstractServer* as = &context.server();
    as->reply_built("defs_cmd", context);
    auto* cmd = dynamic_cast<DefsCmd*>(defs_cmd_.get());
    build_filtered(context, [&](const ecf::Visibility* visibility) { cmd->init(as, save_edit_history, visibility); });
    return defs_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::node_cmd(const ecf::RequestContext& context, node_ptr node) {
    AbstractServer* as = &context.server();
    as->reply_built("node_cmd", context);
    auto* cmd = dynamic_cast<SNodeCmd*>(node_cmd_.get());
    build_filtered(context, [&](const ecf::Visibility* visibility) { cmd->init(as, node, visibility); });
    return node_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::stats_cmd(const ecf::RequestContext& context) {
    AbstractServer* as = &context.server();
    as->reply_built("stats_cmd", context);
    auto* cmd             = dynamic_cast<SStatsCmd*>(stats_cmd_.get());
    const auto visibility = filter_of(context);
    cmd->init(as, visibility.get());
    return stats_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::suites_cmd(const ecf::RequestContext& context) {
    AbstractServer* as = &context.server();
    as->reply_built("suites_cmd", context);
    auto* cmd             = dynamic_cast<SSuitesCmd*>(suites_cmd_.get());
    const auto visibility = filter_of(context);
    cmd->init(as, visibility.get());
    return suites_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::zombie_get_cmd(const ecf::RequestContext& context) {
    AbstractServer* as = &context.server();
    as->reply_built("zombie_get_cmd", context);
    auto* cmd             = dynamic_cast<ZombieGetCmd*>(zombie_get_cmd_.get());
    const auto visibility = filter_of(context);
    cmd->init(as, visibility.get());
    return zombie_get_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::error_cmd(const std::string& error_msg) {
    auto* cmd = dynamic_cast<ErrorCmd*>(error_cmd_.get());
    cmd->init(error_msg);
    return error_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::denied_cmd(const std::string& error_msg) {
    auto* cmd = dynamic_cast<ErrorCmd*>(error_cmd_.get());
    cmd->init(error_msg, ErrorCmd::Kind::PermissionDenied);
    return error_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::client_handle_cmd(int handle) {
    auto* cmd = dynamic_cast<SClientHandleCmd*>(client_handle_cmd_.get());
    cmd->init(handle);
    return client_handle_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::client_handle_suites_cmd(const ecf::RequestContext& context) {
    AbstractServer* as = &context.server();
    as->reply_built("client_handle_suites_cmd", context);
    auto* cmd             = dynamic_cast<SClientHandleSuitesCmd*>(client_handle_suites_cmd_.get());
    const auto visibility = filter_of(context);
    cmd->init(as, visibility.get(), context.identity().username().value());
    return client_handle_suites_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::string_cmd(const std::string& any_string) {
    auto* cmd = dynamic_cast<SStringCmd*>(string_cmd_.get());
    cmd->init(any_string);
    return string_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::string_vec_cmd(const std::vector<std::string>& vec) {
    auto* cmd = dynamic_cast<SStringVecCmd*>(string_vec_cmd_.get());
    cmd->init(vec);
    return string_vec_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::server_load_cmd(const std::string& log_file_path) {
    auto* cmd = dynamic_cast<SServerLoadCmd*>(server_load_cmd_.get());
    cmd->init(log_file_path);
    return server_load_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::news_cmd(unsigned int client_handle,
                                        unsigned int client_state_change_no,
                                        unsigned int client_modify_change_no,
                                        const ecf::RequestContext& context) {
    AbstractServer* as = &context.server();
    as->reply_built("news_cmd", context);
    auto* cmd = dynamic_cast<SNewsCmd*>(news_cmd_.get());
    cmd->init(client_handle, client_state_change_no, client_modify_change_no, as);
    return news_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::sync_cmd(unsigned int client_handle,
                                        unsigned int client_state_change_no,
                                        unsigned int client_modify_change_no,
                                        const ecf::RequestContext& context) {
    AbstractServer* as = &context.server();
    as->reply_built("sync_cmd", context);
    auto* cmd = dynamic_cast<SSyncCmd*>(sync_cmd_.get());
    build_filtered(context, [&](const ecf::Visibility* visibility) {
        cmd->init(client_handle,
                  client_state_change_no,
                  client_modify_change_no,
                  false /*full sync*/,
                  false /*sync suite clock*/,
                  as,
                  visibility);
    });
    return sync_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::sync_clock_cmd(unsigned int client_handle,
                                              unsigned int client_state_change_no,
                                              unsigned int client_modify_change_no,
                                              const ecf::RequestContext& context) {
    AbstractServer* as = &context.server();
    as->reply_built("sync_clock_cmd", context);
    auto* cmd = dynamic_cast<SSyncCmd*>(sync_cmd_.get());
    build_filtered(context, [&](const ecf::Visibility* visibility) {
        cmd->init(client_handle,
                  client_state_change_no,
                  client_modify_change_no,
                  false /*full sync*/,
                  true /*sync suite clock*/,
                  as,
                  visibility);
    });
    return sync_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::sync_full_cmd(unsigned int client_handle, const ecf::RequestContext& context) {
    AbstractServer* as = &context.server();
    as->reply_built("sync_full_cmd", context);
    auto* cmd = dynamic_cast<SSyncCmd*>(sync_cmd_.get()); // can reuse the same command
    build_filtered(context, [&](const ecf::Visibility* visibility) {
        cmd->init(client_handle, 0, 0, true /*full sync*/, false /*sync suite clock*/, as, visibility);
    });
    return sync_cmd_;
}

// ==============================================================================
// Serialisation export

CEREAL_REGISTER_TYPE(DefsCmd)
CEREAL_REGISTER_TYPE(SNodeCmd)
CEREAL_REGISTER_TYPE(SStringCmd)
CEREAL_REGISTER_TYPE(SStringVecCmd)
CEREAL_REGISTER_TYPE(SServerLoadCmd)
CEREAL_REGISTER_TYPE(GroupSTCCmd)
CEREAL_REGISTER_TYPE(ErrorCmd)
CEREAL_REGISTER_TYPE(StcCmd)
CEREAL_REGISTER_TYPE(SSyncCmd)
CEREAL_REGISTER_TYPE(SNewsCmd)
CEREAL_REGISTER_TYPE(SStatsCmd)
CEREAL_REGISTER_TYPE(SSuitesCmd)
CEREAL_REGISTER_TYPE(SClientHandleCmd)
CEREAL_REGISTER_TYPE(SClientHandleSuitesCmd)
CEREAL_REGISTER_TYPE(ZombieGetCmd)
CEREAL_REGISTER_TYPE(BlockClientZombieCmd)
