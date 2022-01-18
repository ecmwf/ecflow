/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : ClientCmdCache
// Author      : Avi
// Revision    : $Revision$
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "ClientCmdCache.hpp"

ClientCmdCache::ClientCmdCache(){}
ClientCmdCache::~ClientCmdCache(){}

std::shared_ptr<CSyncCmd> ClientCmdCache::csync_cmd(CSyncCmd::Api api, unsigned int client_handle,unsigned int client_state_change_no, unsigned int client_modify_change_no) const
{
   if (!csync_cmd_) csync_cmd_ = std::make_shared<CSyncCmd>();
   //csync_cmd_->init(api,client_handle,client_state_change_no,client_modify_change_no);
   return csync_cmd_;
}

std::shared_ptr<CSyncCmd> ClientCmdCache::csync_cmd(unsigned int client_handle) const
{
   if (!csync_cmd_) csync_cmd_ = std::make_shared<CSyncCmd>();
   //csync_cmd_->init(client_handle);
   return csync_cmd_;
}

