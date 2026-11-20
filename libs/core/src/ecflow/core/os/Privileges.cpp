// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/os/Privileges.hpp"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <grp.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#include <sys/stat.h>
#include <sys/wait.h>

#if defined(__linux__)
    #include <linux/capability.h>
    #include <sys/syscall.h>
    #include <sys/xattr.h>
#elif defined(__APPLE__)
    #include <sys/acl.h>
    #include <sys/param.h>

    #include <mach-o/dyld.h>
#endif

namespace ecf::os {

namespace {

#if defined(__linux__)

constexpr unsigned identity_capabilities = (1u << CAP_SETUID) | (1u << CAP_SETGID); // both below 32

struct CapabilitySets
{
    __user_cap_header_struct header{_LINUX_CAPABILITY_VERSION_3, 0};
    __user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3]{};
};

CapabilitySets get_capabilities() {
    CapabilitySets sets;
    if (::syscall(SYS_capget, &sets.header, sets.data) != 0) {
        throw std::runtime_error(std::string("could not read the capabilities of the server: ") + std::strerror(errno));
    }
    return sets;
}

#endif

int set_groups(std::size_t count, const gid_t* groups) {
#if defined(__linux__)
    return static_cast<int>(::syscall(SYS_setgroups, count, groups));
#else
    return ::setgroups(static_cast<int>(count), groups);
#endif
}

int set_gid(gid_t gid) {
#if defined(__linux__)
    return static_cast<int>(::syscall(SYS_setgid, gid));
#else
    return ::setgid(gid);
#endif
}

int set_uid(uid_t uid) {
#if defined(__linux__)
    return static_cast<int>(::syscall(SYS_setuid, uid));
#else
    return ::setuid(uid);
#endif
}

} // namespace

uid_t SystemPrivileges::effective_uid() const {
    return ::geteuid();
}

bool SystemPrivileges::capabilities_supported() const {
#if defined(__linux__)
    return true;
#else
    return false;
#endif
}

bool SystemPrivileges::has_identity_capabilities() const {
#if defined(__linux__)
    try {
        return (get_capabilities().data[0].permitted & identity_capabilities) == identity_capabilities;
    }
    catch (const std::runtime_error&) {
        return false;
    }
#else
    return false;
#endif
}

void SystemPrivileges::raise_identity_capabilities() {
#if defined(__linux__)
    auto sets = get_capabilities();
    if ((sets.data[0].permitted & identity_capabilities) != identity_capabilities) {
        throw std::runtime_error("CAP_SETUID and CAP_SETGID are not permitted to the server");
    }
    sets.data[0].effective |= identity_capabilities;
    if (::syscall(SYS_capset, &sets.header, sets.data) != 0) {
        throw std::runtime_error(std::string("could not raise CAP_SETUID and CAP_SETGID: ") + std::strerror(errno));
    }
#else
    throw std::runtime_error("capabilities are not supported on this platform");
#endif
}

void SystemPrivileges::trial_switch(const OsIdentity& identity) {
    // The account was looked up, and the groups prepared, before forking
    const std::vector<gid_t> groups = groups_of(identity);
    pid_t pid                       = ::fork();
    if (pid < 0) {
        throw std::runtime_error(std::string("could not fork to check the change of identity: ") +
                                 std::strerror(errno));
    }
    if (pid == 0) {
        // Raw system calls on Linux: the libc wrappers synchronise all the threads of the parent (setxid), which
        // is not safe after fork. On macOS, these functions are direct system calls.
        int step = 1;
        if (set_groups(groups.size(), groups.data()) == 0) {
            step = 2;
            if (set_gid(identity.gid) == 0) {
                step = 3;
                if (set_uid(identity.uid) == 0 && ::getuid() == identity.uid && ::geteuid() == identity.uid) {
                    ::_exit(0);
                }
            }
        }
        ::_exit(10 + step);
    }

    int status = 0;
    while (::waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            throw std::runtime_error(std::string("could not check the change of identity: ") + std::strerror(errno));
        }
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return;
    }
    std::string failed = "the change of identity";
    if (WIFEXITED(status)) {
        switch (WEXITSTATUS(status)) {
            case 11:
                failed = "setgroups";
                break;
            case 12:
                failed = "setgid";
                break;
            case 13:
                failed = "setuid";
                break;
            default:
                break;
        }
    }
    throw std::runtime_error("could not take the identity of '" + identity.user + "' (" + failed + " failed)");
}

std::vector<gid_t> groups_of(const OsIdentity& identity) {
    std::vector<gid_t> groups{identity.gid};
    for (auto gid : identity.groups) {
        if (gid != identity.gid) {
            groups.push_back(gid);
        }
    }
#if defined(__APPLE__)
    // macOS refuses more than NGROUPS (16) groups; further memberships are resolved by the system (opendirectoryd)
    if (groups.size() > static_cast<std::size_t>(NGROUPS)) {
        groups.resize(static_cast<std::size_t>(NGROUPS));
    }
#endif
    return groups;
}

std::optional<std::string> inspect_executable(const std::string& path, uid_t server_uid) {
    struct stat info{};
    if (::stat(path.c_str(), &info) != 0) {
        return std::string("it cannot be inspected: ") + std::strerror(errno);
    }

    bool privileged    = (info.st_mode & (S_ISUID | S_ISGID)) != 0;
    bool has_file_caps = false;
#if defined(__linux__)
    errno = 0;
    if (::getxattr(path.c_str(), "security.capability", nullptr, 0) >= 0) {
        has_file_caps = true;
    }
    else if (errno != ENODATA && errno != ENOTSUP) {
        return std::string("its file capabilities cannot be inspected: ") + std::strerror(errno);
    }
#endif
    privileged = privileged || has_file_caps;
    if (!privileged) {
        return std::nullopt; // nothing can be gained by running it
    }

    const std::string what = has_file_caps ? "it carries file capabilities" : "it is set-user-ID or set-group-ID";
    if (info.st_uid != 0 && info.st_uid != server_uid) {
        return what + " and is owned by another account than root or the server account";
    }
    if ((info.st_mode & (S_IXGRP | S_IXOTH)) != 0) {
        return what + " and is executable by other accounts";
    }
    // An access control list could grant execution to other accounts, whatever the mode bits
#if defined(__linux__)
    errno = 0;
    if (::getxattr(path.c_str(), "system.posix_acl_access", nullptr, 0) >= 0) {
        return what + " and has an access control list";
    }
    if (errno != ENODATA && errno != ENOTSUP) {
        return std::string("its access control list cannot be inspected: ") + std::strerror(errno);
    }
#elif defined(__APPLE__)
    // Any extended ACL returned is refused, without enumerating its entries (which could fail)
    errno     = 0;
    acl_t acl = ::acl_get_file(path.c_str(), ACL_TYPE_EXTENDED);
    if (acl != nullptr) {
        ::acl_free(acl);
        return what + " and has an access control list";
    }
    if (errno != ENOENT) {
        return std::string("its access control list cannot be inspected: ") + std::strerror(errno);
    }
#else
    return what + ", and its access control list cannot be inspected on this platform";
#endif
    return std::nullopt;
}

std::optional<std::string> SystemPrivileges::check_executable() const {
#if defined(__linux__)
    return inspect_executable("/proc/self/exe", ::geteuid());
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    ::_NSGetExecutablePath(nullptr, &size);
    std::string path(size, '\0');
    if (::_NSGetExecutablePath(path.data(), &size) != 0) {
        return std::string("its path cannot be found");
    }
    path.resize(std::strlen(path.c_str()));
    return inspect_executable(path, ::geteuid());
#else
    return std::string("it cannot be inspected on this platform");
#endif
}

} // namespace ecf::os
