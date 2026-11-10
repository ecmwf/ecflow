// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/os/AccountDatabase.hpp"

#include <climits>
#include <cstring>
#include <grp.h>
#include <pwd.h>
#include <stdexcept>
#include <unistd.h>

#include "ecflow/core/os/AccountLookup.hpp"

namespace ecf::os {

namespace {

std::size_t initial_buffer_size(int name) {
    long size = ::sysconf(name);
    return size > 0 ? std::min(static_cast<std::size_t>(size), detail::max_lookup_buffer_size) : std::size_t{16384};
}

} // namespace

std::optional<Account> SystemAccountDatabase::find_user(const std::string& name) const {
    passwd entry{};
    passwd* result = nullptr;
    std::vector<char> buffer(initial_buffer_size(_SC_GETPW_R_SIZE_MAX));
    int error = detail::lookup_with_growing_buffer(::getpwnam_r, name, entry, result, buffer);
    if (result == nullptr) {
        if (detail::is_not_found(error)) {
            return std::nullopt;
        }
        throw std::runtime_error("Could not look up the user '" + name + "': " + std::strerror(error));
    }
    return Account{result->pw_name, result->pw_uid, result->pw_gid, result->pw_dir, result->pw_shell};
}

std::optional<Group> SystemAccountDatabase::find_group(const std::string& name) const {
    group entry{};
    group* result = nullptr;
    std::vector<char> buffer(initial_buffer_size(_SC_GETGR_R_SIZE_MAX));
    int error = detail::lookup_with_growing_buffer(::getgrnam_r, name, entry, result, buffer);
    if (result == nullptr) {
        if (detail::is_not_found(error)) {
            return std::nullopt;
        }
        throw std::runtime_error("Could not look up the group '" + name + "': " + std::strerror(error));
    }
    return Group{result->gr_name, result->gr_gid};
}

std::vector<gid_t> SystemAccountDatabase::groups_of(const Account& account) const {
#if defined(__APPLE__)
    // macOS declares getgrouplist with int identifiers
    if (account.gid > static_cast<gid_t>(INT_MAX)) {
        throw std::runtime_error("Could not list the groups of the user '" + account.name +
                                 "': the primary group identifier is out of range");
    }
    return detail::list_groups<int>(::getgrouplist, account.name, account.gid);
#else
    return detail::list_groups<gid_t>(::getgrouplist, account.name, account.gid);
#endif
}

} // namespace ecf::os
