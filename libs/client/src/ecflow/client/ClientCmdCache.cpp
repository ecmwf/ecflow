/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/client/ClientCmdCache.hpp"

std::shared_ptr<CSyncCmd> ClientCmdCache::csync_cmd(CSyncCmd::Api api,
                                                    unsigned int client_handle,
                                                    unsigned int client_state_change_no,
                                                    unsigned int client_modify_change_no) const {
    if (!csync_cmd_) {
        csync_cmd_ = std::make_shared<CSyncCmd>();
    }
    // csync_cmd_->init(api,client_handle,client_state_change_no,client_modify_change_no);
    return csync_cmd_;
}

std::shared_ptr<CSyncCmd> ClientCmdCache::csync_cmd(unsigned int client_handle) const {
    if (!csync_cmd_) {
        csync_cmd_ = std::make_shared<CSyncCmd>();
    }
    // csync_cmd_->init(client_handle);
    return csync_cmd_;
}
