// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/core/os/AccountDatabase.hpp"
#include "ecflow/core/os/Privileges.hpp"

class Defs;

namespace ecf {

///
/// @brief The ownership settings of the server, as configured.
///
struct OwnershipSettings
{
    std::optional<std::string> owner; ///< The server Owner, as user[:group]; ownership is enabled when present.
    std::string path;                 ///< The PATH of the processes run as owners.
    std::string environment;          ///< The comma-separated names of further variables passed to them.
};

///
/// @brief The outcome of the ownership checks made at start-up.
///
struct OwnershipStatus
{
    bool enabled{false};                   ///< Whether the server changes identity to run commands as owners.
    std::optional<OwnerAttr> server_owner; ///< The server Owner, when enabled.
    std::string privileges;                ///< How the server changes identity, when enabled.
    std::vector<std::string> warnings;     ///< To be logged as warnings.

    ///
    /// @brief Describes the status, as reported to users (e.g. by --stats).
    ///
    /// @return "enabled, server Owner <owner>, <privileges>", or "disabled".
    ///
    [[nodiscard]] std::string summary() const;
};

///
/// @brief Whether this build can enable ownership: only test builds (ENABLE_OWNERSHIP_UNDER_TEST), until the
///        feature is complete.
///
#ifdef ECF_OWNERSHIP_UNDER_TEST
inline constexpr bool ownership_available_in_build = true;
#else
inline constexpr bool ownership_available_in_build = false;
#endif

///
/// @brief Makes the checks of ownership at start-up, before the server accepts any request.
///
/// @details Without a server Owner, ownership is disabled, and nothing is checked. Otherwise, the server Owner must
///          be an eligible account; the server must be able to change identity (as root, or on Linux with
///          CAP_SETUID and CAP_SETGID, which are then raised to its effective set), as verified by taking the
///          identity of the server Owner in a child process; and its executable must not give its privileges to
///          other accounts.
///
/// @param[in] settings The ownership settings.
/// @param[in] accounts The account database.
/// @param[in,out] privileges The privileges of the server process.
/// @param[in] available_in_build Whether this build can enable ownership (see ownership_available_in_build).
/// @return The status of ownership.
/// @throws std::runtime_error with the reason the server must not start.
///
OwnershipStatus check_ownership_at_startup(const OwnershipSettings& settings,
                                           const os::AccountDatabase& accounts,
                                           os::Privileges& privileges,
                                           bool available_in_build);

///
/// @brief Returns the warning due when ownership is disabled, but the definitions hold Owners.
///
/// @param[in] defs The definitions of the server, as restored.
/// @param[in] status The status of ownership.
/// @return The warning, or no value when none is due.
///
std::optional<std::string> owners_without_ownership_warning(const Defs& defs, const OwnershipStatus& status);

} // namespace ecf
