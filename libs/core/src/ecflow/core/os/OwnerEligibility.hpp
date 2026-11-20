// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>

#include <sys/types.h>

#include "ecflow/core/Result.hpp"
#include "ecflow/core/os/AccountDatabase.hpp"

namespace ecf::os {

///
/// @brief Decides whether an account may be an owner, and resolves its identity.
///
/// @details An owner must be an existing account other than the superuser (uid 0) and the server account, and,
///          when a group is given, a member of that group. Looking up the accounts is delegated to an
///          AccountDatabase, which is only queried by the server.
///
class OwnerEligibility {
public:
    ///
    /// @brief Creates the rules for a server running under the given account.
    ///
    /// @param[in] db The account database; must outlive this object.
    /// @param[in] server_uid The user identifier of the server account.
    ///
    OwnerEligibility(const AccountDatabase& db, uid_t server_uid)
        : db_(db),
          server_uid_(server_uid) {}

    ///
    /// @brief Checks that a user (and group) may be an owner, and resolves the identity.
    ///
    /// @param[in] user The user name.
    /// @param[in] group The group name, or no value for the primary group of the user.
    /// @return The resolved identity, or the reason (naming the user) why it may not be an owner.
    /// @throws std::runtime_error if the account database cannot be queried.
    ///
    Result<OsIdentity> check(const std::string& user, const std::optional<std::string>& group) const;

private:
    const AccountDatabase& db_;
    uid_t server_uid_;
};

} // namespace ecf::os
