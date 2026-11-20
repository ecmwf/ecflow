// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>
#include <vector>

#include <sys/types.h>

namespace ecf::os {

///
/// @brief A user account, as found in the OS account database.
///
struct Account
{
    std::string name;  ///< The user name.
    uid_t uid;         ///< The user identifier.
    gid_t gid;         ///< The primary group identifier.
    std::string home;  ///< The home directory.
    std::string shell; ///< The login shell.
};

///
/// @brief A group, as found in the OS account database.
///
struct Group
{
    std::string name; ///< The group name.
    gid_t gid;        ///< The group identifier.
};

///
/// @brief A fully resolved OS identity, under which commands are run and files accessed.
///
struct OsIdentity
{
    std::string user;          ///< The user name.
    uid_t uid;                 ///< The user identifier.
    gid_t gid;                 ///< The group identifier to use (primary, or the one requested).
    std::vector<gid_t> groups; ///< All the groups of the user (primary and supplementary).
    std::string home;          ///< The home directory.
    std::string shell;         ///< The login shell.
};

///
/// @brief Looks up accounts and groups by name.
///
/// @details An interface, so that the rules using it can be tested with a fake database. Lookups are made in the
///          server process, never between fork and exec.
///
class AccountDatabase {
public:
    virtual ~AccountDatabase() = default;

    ///
    /// @brief Looks up a user account by name.
    ///
    /// @param[in] name The user name.
    /// @return The account, or no value when no such user exists.
    /// @throws std::runtime_error if the database cannot be queried.
    ///
    virtual std::optional<Account> find_user(const std::string& name) const = 0;

    ///
    /// @brief Looks up a group by name.
    ///
    /// @param[in] name The group name.
    /// @return The group, or no value when no such group exists.
    /// @throws std::runtime_error if the database cannot be queried.
    ///
    virtual std::optional<Group> find_group(const std::string& name) const = 0;

    ///
    /// @brief Returns all the groups of an account.
    ///
    /// @param[in] account The account.
    /// @return The primary group followed by the supplementary groups, without duplicates.
    /// @throws std::runtime_error if the database cannot be queried.
    ///
    virtual std::vector<gid_t> groups_of(const Account& account) const = 0;
};

///
/// @brief The account database of the operating system (getpwnam_r, getgrnam_r, getgrouplist).
///
class SystemAccountDatabase final : public AccountDatabase {
public:
    std::optional<Account> find_user(const std::string& name) const override;
    std::optional<Group> find_group(const std::string& name) const override;
    std::vector<gid_t> groups_of(const Account& account) const override;
};

} // namespace ecf::os
