// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>
#include <vector>

#include <sys/types.h>

#include "ecflow/core/os/AccountDatabase.hpp"

namespace ecf::os {

///
/// @brief The privileges of the server process to change identity, behind an interface so that the checks made at
///        start-up can be tested without them.
///
/// @details On Linux, a server changes identity with the capabilities CAP_SETUID and CAP_SETGID, or as root;
///          elsewhere, as root only.
///
class Privileges {
public:
    virtual ~Privileges() = default;

    ///
    /// @brief Returns the effective user of the process, i.e. the server account.
    ///
    /// @return The user identifier.
    ///
    virtual uid_t effective_uid() const = 0;

    ///
    /// @brief Tells whether the platform has capabilities (Linux).
    ///
    /// @return true on Linux; false otherwise.
    ///
    virtual bool capabilities_supported() const = 0;

    ///
    /// @brief Tells whether CAP_SETUID and CAP_SETGID are in the permitted set of the process.
    ///
    /// @return true when both are permitted; false otherwise, and always where capabilities are not supported.
    ///
    virtual bool has_identity_capabilities() const = 0;

    ///
    /// @brief Raises CAP_SETUID and CAP_SETGID to the effective set of the process, so that the process and the
    ///        processes it forks can change identity.
    ///
    /// @throws std::runtime_error if they are not permitted, or cannot be raised.
    ///
    virtual void raise_identity_capabilities() = 0;

    ///
    /// @brief Checks that the process can take the given identity, in a child process that exits at once.
    ///
    /// @param[in] identity The identity to take.
    /// @throws std::runtime_error with the reason, if the identity cannot be taken.
    ///
    virtual void trial_switch(const OsIdentity& identity) = 0;

    ///
    /// @brief Checks that the executable of the process cannot give its privileges to other accounts.
    ///
    /// @details See inspect_executable().
    ///
    /// @return The reason the executable is refused, or no value when it is accepted.
    ///
    virtual std::optional<std::string> check_executable() const = 0;
};

///
/// @brief Checks that an executable cannot give privileges to the accounts able to run it.
///
/// @details An executable that is set-user-ID or set-group-ID, or (on Linux) carries file capabilities, must be
///          owned by root or the server account, not be executable by its group or others, and have no access
///          control list (POSIX on Linux, extended on macOS), which could grant execution to other accounts. It is
///          refused when it cannot be inspected.
///
/// @param[in] path The path of the executable.
/// @param[in] server_uid The user identifier of the server account.
/// @return The reason the executable is refused, or no value when it is accepted.
///
std::optional<std::string> inspect_executable(const std::string& path, uid_t server_uid);

///
/// @brief Returns the groups taken when changing to the given identity.
///
/// @details The primary group comes first, followed by the supplementary groups, without duplicates of the primary
///          group. On macOS, which refuses more than NGROUPS (16) groups, the list is truncated, the further
///          memberships being resolved by the system.
///
/// @param[in] identity The identity taken.
/// @return The groups, primary group first.
///
std::vector<gid_t> groups_of(const OsIdentity& identity);

///
/// @brief The privileges of the current process.
///
class SystemPrivileges final : public Privileges {
public:
    uid_t effective_uid() const override;
    bool capabilities_supported() const override;
    bool has_identity_capabilities() const override;
    void raise_identity_capabilities() override;
    void trial_switch(const OsIdentity& identity) override;
    std::optional<std::string> check_executable() const override;
};

} // namespace ecf::os
