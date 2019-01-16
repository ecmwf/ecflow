//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #24 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include "PreAllocatedReply.hpp"

#include "StcCmd.hpp"
#include "DefsCmd.hpp"
#include "SNodeCmd.hpp"
#include "SStringCmd.hpp"
#include "SStringVecCmd.hpp"
#include "SServerLoadCmd.hpp"
#include "GroupSTCCmd.hpp"
#include "ErrorCmd.hpp"
#include "SNewsCmd.hpp"
#include "SSyncCmd.hpp"
#include "SStatsCmd.hpp"
#include "SSuitesCmd.hpp"
#include "SClientHandleCmd.hpp"
#include "SClientHandleSuitesCmd.hpp"
#include "ZombieGetCmd.hpp"
#include "BlockClientZombieCmd.hpp"

STC_Cmd_ptr PreAllocatedReply::stc_cmd_                  = STC_Cmd_ptr( new StcCmd() );
STC_Cmd_ptr PreAllocatedReply::defs_cmd_                 = STC_Cmd_ptr( new DefsCmd() );
STC_Cmd_ptr PreAllocatedReply::node_cmd_                 = STC_Cmd_ptr( new SNodeCmd() );
STC_Cmd_ptr PreAllocatedReply::stats_cmd_                = STC_Cmd_ptr( new SStatsCmd() );
STC_Cmd_ptr PreAllocatedReply::suites_cmd_               = STC_Cmd_ptr( new SSuitesCmd() );
STC_Cmd_ptr PreAllocatedReply::zombie_get_cmd_           = STC_Cmd_ptr( new ZombieGetCmd() );
STC_Cmd_ptr PreAllocatedReply::error_cmd_                = STC_Cmd_ptr( new ErrorCmd() );
STC_Cmd_ptr PreAllocatedReply::client_handle_cmd_        = STC_Cmd_ptr( new SClientHandleCmd() );
STC_Cmd_ptr PreAllocatedReply::client_handle_suites_cmd_ = STC_Cmd_ptr( new SClientHandleSuitesCmd() );
STC_Cmd_ptr PreAllocatedReply::string_cmd_               = STC_Cmd_ptr( new SStringCmd() );
STC_Cmd_ptr PreAllocatedReply::string_vec_cmd_           = STC_Cmd_ptr( new SStringVecCmd() );
STC_Cmd_ptr PreAllocatedReply::server_load_cmd_          = STC_Cmd_ptr( new SServerLoadCmd() );
STC_Cmd_ptr PreAllocatedReply::news_cmd_                 = STC_Cmd_ptr( new SNewsCmd() );
STC_Cmd_ptr PreAllocatedReply::sync_cmd_                 = STC_Cmd_ptr( new SSyncCmd() );
STC_Cmd_ptr PreAllocatedReply::block_client_zombie_cmd_  = STC_Cmd_ptr( new BlockClientZombieCmd() );


STC_Cmd_ptr PreAllocatedReply::ok_cmd()
{
	auto* cmd = dynamic_cast<StcCmd*>(stc_cmd_.get());
	cmd->init(StcCmd::OK);
	return stc_cmd_;
}
STC_Cmd_ptr PreAllocatedReply::block_client_server_halted_cmd()
{
	auto* cmd = dynamic_cast<StcCmd*>(stc_cmd_.get());
	cmd->init(StcCmd::BLOCK_CLIENT_SERVER_HALTED);
	return stc_cmd_;
}
STC_Cmd_ptr PreAllocatedReply::block_client_on_home_server_cmd()
{
	auto* cmd = dynamic_cast<StcCmd*>(stc_cmd_.get());
	cmd->init(StcCmd::BLOCK_CLIENT_ON_HOME_SERVER);
	return stc_cmd_;
}
STC_Cmd_ptr PreAllocatedReply::delete_all_cmd()
{
   auto* cmd = dynamic_cast<StcCmd*>(stc_cmd_.get());
   cmd->init(StcCmd::DELETE_ALL);
   return stc_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::block_client_zombie_cmd(ecf::Child::ZombieType zt)
{
   auto* cmd = dynamic_cast<BlockClientZombieCmd*>(block_client_zombie_cmd_.get());
	cmd->init(zt);
	return block_client_zombie_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::defs_cmd(AbstractServer* as, bool save_edit_history)
{
	auto* cmd = dynamic_cast<DefsCmd*>(defs_cmd_.get());
	cmd->init(as,save_edit_history);
	return defs_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::node_cmd(AbstractServer* as,node_ptr node)
{
   auto* cmd = dynamic_cast<SNodeCmd*>(node_cmd_.get());
   cmd->init(as,node);
   return node_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::stats_cmd(AbstractServer* as)
{
	auto* cmd = dynamic_cast<SStatsCmd*>(stats_cmd_.get());
	cmd->init(as);
	return stats_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::suites_cmd(AbstractServer* as)
{
	auto* cmd = dynamic_cast<SSuitesCmd*>(suites_cmd_.get());
	cmd->init(as);
	return suites_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::zombie_get_cmd(AbstractServer* as)
{
	auto* cmd = dynamic_cast<ZombieGetCmd*>(zombie_get_cmd_.get());
	cmd->init(as);
	return zombie_get_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::error_cmd(const std::string& error_msg)
{
	auto* cmd = dynamic_cast<ErrorCmd*>(error_cmd_.get());
	cmd->init(error_msg);
	return error_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::client_handle_cmd(int handle)
{
	auto* cmd = dynamic_cast<SClientHandleCmd*>(client_handle_cmd_.get());
	cmd->init(handle);
	return client_handle_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::client_handle_suites_cmd(AbstractServer* as)
{
   auto* cmd = dynamic_cast<SClientHandleSuitesCmd*>(client_handle_suites_cmd_.get());
   cmd->init(as);
   return client_handle_suites_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::string_cmd(const std::string& any_string)
{
   auto* cmd = dynamic_cast<SStringCmd*>(string_cmd_.get());
	cmd->init(any_string);
	return string_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::string_vec_cmd(const std::vector<std::string>& vec)
{
   auto* cmd = dynamic_cast<SStringVecCmd*>(string_vec_cmd_.get());
   cmd->init(vec);
   return string_vec_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::server_load_cmd(const std::string& log_file_path)
{
   auto* cmd = dynamic_cast<SServerLoadCmd*>(server_load_cmd_.get());
   cmd->init(log_file_path);
   return server_load_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::news_cmd(unsigned int client_handle,
										unsigned int client_state_change_no,
										unsigned int client_modify_change_no,
										AbstractServer* as)
{
	auto* cmd = dynamic_cast<SNewsCmd*>(news_cmd_.get());
	cmd->init(client_handle,client_state_change_no,client_modify_change_no,as);
	return news_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::sync_cmd(unsigned int client_handle,
										unsigned int client_state_change_no,
										unsigned int client_modify_change_no,
										AbstractServer* as)
{
	auto* cmd = dynamic_cast<SSyncCmd*>(sync_cmd_.get());
	cmd->init(client_handle,client_state_change_no,client_modify_change_no,false/*full sync*/,false/*sync suite clock*/,as);
	return sync_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::sync_clock_cmd(unsigned int client_handle,
                              unsigned int client_state_change_no,
                              unsigned int client_modify_change_no,
                              AbstractServer* as)
{
   auto* cmd = dynamic_cast<SSyncCmd*>(sync_cmd_.get());
   cmd->init(client_handle,client_state_change_no,client_modify_change_no,false/*full sync*/,true/*sync suite clock*/,as);
   return sync_cmd_;
}

STC_Cmd_ptr PreAllocatedReply::sync_full_cmd(unsigned int client_handle,AbstractServer* as)
{
   auto* cmd = dynamic_cast<SSyncCmd*>(sync_cmd_.get()); // can reuse the same command
   cmd->init(client_handle,0,0,true/*full sync*/,false/*sync suite clock*/,as);
   return sync_cmd_;
}

// ==============================================================================
// Serialisation export

CEREAL_REGISTER_TYPE(DefsCmd);
CEREAL_REGISTER_TYPE(SNodeCmd);
CEREAL_REGISTER_TYPE(SStringCmd);
CEREAL_REGISTER_TYPE(SStringVecCmd);
CEREAL_REGISTER_TYPE(SServerLoadCmd);
CEREAL_REGISTER_TYPE(GroupSTCCmd);
CEREAL_REGISTER_TYPE(ErrorCmd);
CEREAL_REGISTER_TYPE(StcCmd);
CEREAL_REGISTER_TYPE(SSyncCmd);
CEREAL_REGISTER_TYPE(SNewsCmd);
CEREAL_REGISTER_TYPE(SStatsCmd);
CEREAL_REGISTER_TYPE(SSuitesCmd);
CEREAL_REGISTER_TYPE(SClientHandleCmd);
CEREAL_REGISTER_TYPE(SClientHandleSuitesCmd);
CEREAL_REGISTER_TYPE(ZombieGetCmd);
CEREAL_REGISTER_TYPE(BlockClientZombieCmd);
