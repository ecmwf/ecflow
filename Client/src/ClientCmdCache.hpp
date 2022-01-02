#ifndef CLIENT_CMD_CACHE_HPP_
#define CLIENT_CMD_CACHE_HPP_

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

#include "ClientToServerCmd.hpp"

class ClientCmdCache {
public:
   ClientCmdCache(const ClientCmdCache &) = delete;
   const ClientCmdCache& operator=(const ClientCmdCache&) = delete;

   ClientCmdCache();
   ~ClientCmdCache();

    std::shared_ptr<CSyncCmd> csync_cmd(CSyncCmd::Api, unsigned int client_handle,unsigned int client_state_change_no, unsigned int client_modify_change_no) const;
    std::shared_ptr<CSyncCmd> csync_cmd(unsigned int client_handle) const;

private:
    mutable std::shared_ptr<CSyncCmd> csync_cmd_;
};

#endif
