// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/os/OwnerEligibility.hpp"

#include <algorithm>

namespace ecf::os {

Result<OsIdentity> OwnerEligibility::check(const std::string& user, const std::optional<std::string>& group) const {
    auto account = db_.find_user(user);
    if (!account) {
        return Result<OsIdentity>::failure("The owner '" + user + "' is an unknown user");
    }
    if (account->uid == 0) {
        return Result<OsIdentity>::failure("The owner '" + user +
                                           "' is the superuser (uid 0), which cannot be an owner");
    }
    if (account->uid == server_uid_) {
        return Result<OsIdentity>::failure("The owner '" + user + "' is the server account, which cannot be an owner");
    }

    auto groups = db_.groups_of(*account);
    gid_t gid   = account->gid;
    if (group) {
        auto found = db_.find_group(*group);
        if (!found) {
            return Result<OsIdentity>::failure("The owner '" + user + "' names an unknown group '" + *group + "'");
        }
        if (std::find(groups.begin(), groups.end(), found->gid) == groups.end()) {
            return Result<OsIdentity>::failure("The owner '" + user + "' is not a member of the group '" + *group +
                                               "'");
        }
        gid = found->gid;
    }

    return Result<OsIdentity>::success(
        OsIdentity{account->name, account->uid, gid, groups, account->home, account->shell});
}

} // namespace ecf::os
