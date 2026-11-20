// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <system_error>

#include "ecflow/core/Filesystem.hpp"

namespace ecf {

///
/// @brief Determines whether a failed open can be attributed to a missing filesystem path.
///
/// @param[in] path Attempted file path; an empty path does not establish absence.
/// @return True only when the file or a path component is absent.
///
/// Filesystem status errors other than ENOENT or ENOTDIR yield false without throwing.
///
inline bool file_is_absent(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    std::error_code error;
    const auto status = fs::status(path, error);
    return error == std::errc::no_such_file_or_directory || error == std::errc::not_a_directory ||
           (!error && status.type() == fs::file_type::not_found);
}

///
/// @brief Determines whether a configured search root exists as a directory.
///
/// @param[in] path Configured directory path; an empty path is invalid.
/// @return True if the path resolves to a directory without a filesystem status error.
///
/// Filesystem status errors yield false without throwing.
///
inline bool valid_file_root(const std::string& path) {
    std::error_code error;
    return !path.empty() && fs::is_directory(path, error) && !error;
}

} // namespace ecf
