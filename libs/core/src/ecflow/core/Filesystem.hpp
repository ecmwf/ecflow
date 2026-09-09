/*
 * SPDX-FileCopyrightText: 2023- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_core_Filesystem_HPP
#define ecflow_core_Filesystem_HPP

#include <filesystem>

namespace fs = std::filesystem;

namespace ecf {
namespace fsx {

///
/// @brief Generate a unique path from the given model.
///
/// Each '%' character in @p model is replaced by a random hexadecimal digit (0-9, A-F).
///
/// @param model the path model, with '%' characters as placeholders for random hex digits
/// @return the generated path with all '%' characters replaced by random hex digits
///
std::filesystem::path unique_path(std::string model);

///
/// @brief Get the last write time of the given path as a std::chrono::system_clock::time_point.
///
/// @param path the path to query
/// @return the last write time of @p path
/// @throws std::filesystem::filesystem_error if @p path does not exist or cannot be accessed
///
std::chrono::system_clock::time_point last_write_time(const std::filesystem::path& path);

} // namespace fsx
} // namespace ecf

#endif /* ecflow_core_Filesystem_HPP */
