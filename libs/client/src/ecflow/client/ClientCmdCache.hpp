/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_client_ClientCmdCache_HPP
#define ecflow_client_ClientCmdCache_HPP

#include "ecflow/base/cts/user/CSyncCmd.hpp"

class ClientCmdCache {
public:
    ClientCmdCache() = default;

    // Disable copy (and move) semantics
    ClientCmdCache(const ClientCmdCache&)                  = delete;
    const ClientCmdCache& operator=(const ClientCmdCache&) = delete;
    ClientCmdCache(ClientCmdCache&&)                       = delete;
    ClientCmdCache& operator=(ClientCmdCache&&)            = delete;

    ~ClientCmdCache() = default;

    std::shared_ptr<CSyncCmd> csync_cmd(CSyncCmd::Api,
                                        unsigned int client_handle,
                                        unsigned int client_state_change_no,
                                        unsigned int client_modify_change_no) const;
    std::shared_ptr<CSyncCmd> csync_cmd(unsigned int client_handle) const;

private:
    mutable std::shared_ptr<CSyncCmd> csync_cmd_;
};

#endif /* ecflow_client_ClientCmdCache_HPP */
