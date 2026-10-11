// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/SpawnIdentity.hpp"

#include <cerrno>
#include <cstring>
#include <pwd.h>
#include <unistd.h>
#include <vector>

namespace ecf {

std::optional<SpawnIdentity> resolve_spawn_identity(const std::string& user, std::string& error) {
    error.clear();
    if (user.empty()) {
        error = "no owner is recorded for the task";
        return std::nullopt;
    }

    long size = sysconf(_SC_GETPW_R_SIZE_MAX);
    std::vector<char> buffer(size > 0 ? static_cast<size_t>(size) : 16384);
    struct passwd entry{};
    struct passwd* found = nullptr;
    int status           = getpwnam_r(user.c_str(), &entry, buffer.data(), buffer.size(), &found);
    if (status != 0) {
        error = "the account database could not be read for user '" + user + "': " + std::strerror(status);
        return std::nullopt;
    }
    if (!found) {
        error = "user '" + user + "' is unknown to the system";
        return std::nullopt;
    }
    if (found->pw_uid == 0) {
        error = "user '" + user + "' is root, and no job is ever spawned as root";
        return std::nullopt;
    }

    SpawnIdentity identity;
    identity.name  = found->pw_name ? found->pw_name : user;
    identity.uid   = found->pw_uid;
    identity.gid   = found->pw_gid;
    identity.home  = found->pw_dir ? found->pw_dir : "";
    identity.shell = found->pw_shell ? found->pw_shell : "";
    return identity;
}

} // namespace ecf
