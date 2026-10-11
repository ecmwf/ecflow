// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#ifndef ecflow_node_SpawnIdentity_HPP
#define ecflow_node_SpawnIdentity_HPP

#include <optional>
#include <string>

#include <sys/types.h>

namespace ecf {

///
/// @brief Describes the system account a job, kill or status command is spawned as.
///
/// The identity is resolved from the account database of the host, in the server process, before
/// the command is forked; the child process then switches to it and executes the command.
///
struct SpawnIdentity
{
    std::string name;  ///< Login name of the account
    uid_t uid{0};      ///< User id the child switches to
    gid_t gid{0};      ///< Primary group id the child switches to
    std::string home;  ///< Home directory, exported as HOME
    std::string shell; ///< Login shell, exported as SHELL
};

///
/// @brief Resolves the account a command is spawned as.
///
/// The resolution refuses an empty name (no owner recorded on the task), a name unknown to the
/// host, and an account whose uid is 0, so that no job ever runs as root.
///
/// @param[in]  user  Login name of the account, as recorded by the server; may be empty.
/// @param[out] error Why the name is refused; left empty when the identity is resolved.
/// @return The identity, or std::nullopt when @p user is refused.
///
std::optional<SpawnIdentity> resolve_spawn_identity(const std::string& user, std::string& error);

} // namespace ecf

#endif /* ecflow_node_SpawnIdentity_HPP */
