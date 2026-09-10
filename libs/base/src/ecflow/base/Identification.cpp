/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/base/Identification.hpp"

#include "ecflow/base/cts/task/TaskCmd.hpp"
#include "ecflow/base/cts/user/UserCmd.hpp"

namespace ecf {

Identity identify(const Cmd_ptr& cmd) {
    if (auto user_cmd = dynamic_cast<UserCmd*>(cmd.get()); user_cmd != nullptr) {
        if (user_cmd->is_custom_user()) {
            return ecf::Identity::make_custom_user(user_cmd->user(), user_cmd->passwd());
        }
        return ecf::Identity::make_user(user_cmd->user(), user_cmd->passwd());
    }
    if (auto task_cmd = dynamic_cast<TaskCmd*>(cmd.get()); task_cmd != nullptr) {
        return ecf::Identity::make_task(
            task_cmd->process_or_remote_id(), task_cmd->jobs_password(), std::to_string(task_cmd->try_no()));
    }
    // Should not be reached!
    return Identity::make_none();
}

} // namespace ecf
