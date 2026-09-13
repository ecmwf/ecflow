/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_test_scaffold_LockFile_HPP
#define ecflow_test_scaffold_LockFile_HPP

#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <optional>
#include <string>
#include <unistd.h>

#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/ecflow_source_build_dir.h"

namespace ecf::test::scaffold {

///
/// @brief Owns a lock file on the filesystem, used by tests to reserve a resource (typically a TCP port) across
///        processes.
///
/// The lock file is created atomically, with an exclusive-create open, so that when several processes race to create
/// the same lock file exactly one of them succeeds. A check-then-create sequence would allow two processes to both
/// observe the file as absent and both claim the lock.
///
/// The lock file is removed when the owning instance is destroyed, unless ownership has been relinquished with
/// release().
///
/// @invariant While an instance owns a lock, path() names an existing regular file created by this process.
///
class LockFile {
public:
    ///
    /// @brief Resolves the directory in which lock files are stored.
    ///
    /// All test processes must agree on the same directory, so that a lock created by one process is visible to the
    /// others; the project source directory is used by default, and can be overridden with the ECF_PORT_LOCK_DIR
    /// environment variable.
    ///
    /// @return The lock directory
    ///
    static fs::path lock_directory() {
        std::string path = CMAKE_ECFLOW_SOURCE_DIR();
        ecf::environment::get("ECF_PORT_LOCK_DIR", path);
        return fs::path{path};
    }

    ///
    /// @brief Resolves the path of the lock file associated with the given port.
    ///
    /// @param[in] port The port, as text
    /// @return The path of the lock file, inside lock_directory()
    ///
    static fs::path port_lock_path(const std::string& port) { return lock_directory() / (port + ".lock"); }

    ///
    /// @brief Attempts to acquire the lock at the given path.
    ///
    /// @param[in] lock_file The path of the lock file
    /// @return A LockFile owning the created file, or std::nullopt if the lock file already exists or could not be
    ///         created (for example, due to permissions)
    ///
    static std::optional<LockFile> make_lock(const fs::path& lock_file) {
        if (create_file(lock_file)) {
            return LockFile{lock_file};
        }
        return std::nullopt;
    }

    LockFile(const LockFile&)            = delete;
    LockFile& operator=(const LockFile&) = delete;

    ///
    /// @brief Transfers lock ownership; the moved-from instance no longer owns the lock file.
    ///
    LockFile(LockFile&& other) noexcept
        : lock_file_(other.release()) {}

    LockFile& operator=(LockFile&& other) noexcept {
        if (this != &other) {
            remove();
            lock_file_ = other.release();
        }
        return *this;
    }

    ~LockFile() { remove(); }

    ///
    /// @brief The path of the owned lock file, or an empty path when ownership has been relinquished.
    ///
    [[nodiscard]] const fs::path& path() const { return lock_file_; }

    ///
    /// @brief Relinquishes ownership of the lock file, which is kept on the filesystem.
    ///
    /// The caller becomes responsible for removing the lock file.
    ///
    /// @return The path of the lock file
    ///
    fs::path release() {
        fs::path released;
        std::swap(released, lock_file_);
        return released;
    }

private:
    explicit LockFile(const fs::path& lock_file)
        : lock_file_(lock_file) {
        assert(!lock_file_.empty());
        assert(fs::exists(lock_file_));
        assert(fs::is_regular_file(lock_file_));
    }

    void remove() {
        if (!lock_file_.empty()) {
            fs::remove(lock_file_);
            lock_file_.clear();
        }
    }

    ///
    /// @brief Creates the given lock file atomically.
    ///
    /// @param[in] path The lock file path
    /// @return true if the lock file was created by this call; false if it already exists, or could not be created
    ///
    static bool create_file(const fs::path& path) {
        std::cout << " *** Attempting to create lock file: " << path << std::endl;
        int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (fd < 0) {
            if (errno == EEXIST) {
                std::cout << " *** Found an existing lock file! Giving up..." << std::endl;
            }
            else {
                std::cout << " *** Unable to create a lock file (" << std::strerror(errno) << ")! Giving up..."
                          << std::endl;
            }
            return false;
        }
        ::close(fd);
        std::cout << " *** Created lock file: " << path << std::endl;
        return true;
    }

    fs::path lock_file_{};
};

} // namespace ecf::test::scaffold

#endif /* ecflow_test_scaffold_LockFile_HPP */
