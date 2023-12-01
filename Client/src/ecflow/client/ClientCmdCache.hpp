/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_client_ClientCmdCache_HPP
#define ecflow_client_ClientCmdCache_HPP

#include "ecflow/base/cts/ClientToServerCmd.hpp"

class ClientCmdCache {
public:
    ClientCmdCache(const ClientCmdCache&)                  = delete;
    const ClientCmdCache& operator=(const ClientCmdCache&) = delete;

    ClientCmdCache();
    ~ClientCmdCache();

    std::shared_ptr<CSyncCmd> csync_cmd(CSyncCmd::Api,
                                        unsigned int client_handle,
                                        unsigned int client_state_change_no,
                                        unsigned int client_modify_change_no) const;
    std::shared_ptr<CSyncCmd> csync_cmd(unsigned int client_handle) const;

private:
    mutable std::shared_ptr<CSyncCmd> csync_cmd_;
};

#endif /* ecflow_client_ClientCmdCache_HPP */
