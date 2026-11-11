// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace ecf::os::detail {

///
/// @brief The largest buffer used for a single account or group lookup (1 MiB).
///
inline constexpr std::size_t max_lookup_buffer_size = std::size_t{1} << 20;

///
/// @brief Calls a reentrant lookup function (getpwnam_r, getgrnam_r), growing the buffer while it is too small.
///
/// @tparam Entry The type of the entry (passwd, group).
/// @tparam Function The type of the lookup function.
/// @param[in] function The lookup function, with the signature of getpwnam_r.
/// @param[in] name The name to look up.
/// @param[out] entry The entry filled by the lookup.
/// @param[out] result Set by the lookup to &entry when found, or to nullptr.
/// @param[in,out] buffer The buffer for the strings of the entry; doubled while the lookup reports ERANGE.
/// @return The error code of the last call (0 on success or when not found).
/// @throws std::runtime_error if the entry does not fit in max_lookup_buffer_size bytes.
///
template <typename Entry, typename Function>
int lookup_with_growing_buffer(Function function,
                               const std::string& name,
                               Entry& entry,
                               Entry*& result,
                               std::vector<char>& buffer) {
    for (;;) {
        result    = nullptr;
        int error = function(name.c_str(), &entry, buffer.data(), buffer.size(), &result);
        if (error != ERANGE) {
            return error;
        }
        if (buffer.size() >= max_lookup_buffer_size) {
            throw std::runtime_error("Could not look up '" + name + "': the entry is larger than " +
                                     std::to_string(max_lookup_buffer_size) + " bytes");
        }
        buffer.resize(std::min(buffer.size() * 2, max_lookup_buffer_size));
    }
}

///
/// @brief Tells whether the error code of a reentrant lookup means that the name does not exist.
///
/// @details POSIX reports a missing entry as a null result without error; some systems use these codes instead.
///
/// @param[in] error The error code returned by the lookup.
/// @return true when the name does not exist, false for a failure of the lookup itself.
///
inline bool is_not_found(int error) {
    return error == 0 || error == ENOENT || error == ESRCH || error == EBADF || error == EPERM;
}

///
/// @brief Lists the groups of a user with a getgrouplist-like function, growing the list while it is too small.
///
/// @tparam GroupId The type of the group identifiers taken by the listing function (int on macOS, gid_t otherwise).
/// @tparam Gid The type of the group identifiers returned (gid_t).
/// @tparam Function The type of the listing function.
/// @param[in] function The listing function, with the signature of getgrouplist.
/// @param[in] user The user name.
/// @param[in] gid The primary group of the user.
/// @return The primary group followed by the supplementary groups, without duplicates.
/// @throws std::runtime_error if the groups cannot be listed.
///
template <typename GroupId, typename Gid, typename Function>
std::vector<Gid> list_groups(Function function, const std::string& user, Gid gid) {
    constexpr int max_groups = 65536;
    int count                = 64;
    std::vector<GroupId> ids;
    while (count <= max_groups) {
        ids.resize(static_cast<std::size_t>(count));
        int requested = count;
        if (function(user.c_str(), static_cast<GroupId>(gid), ids.data(), &count) >= 0) {
            ids.resize(static_cast<std::size_t>(std::clamp(count, 0, requested)));
            std::vector<Gid> groups{gid};
            for (auto id : ids) {
                auto group = static_cast<Gid>(id);
                if (std::find(groups.begin(), groups.end(), group) == groups.end()) {
                    groups.push_back(group);
                }
            }
            return groups;
        }
        // The list was too small: some systems report the size needed, others leave it unchanged
        count = std::max(count, requested * 2);
    }
    throw std::runtime_error("Could not list the groups of the user '" + user + "'");
}

} // namespace ecf::os::detail
