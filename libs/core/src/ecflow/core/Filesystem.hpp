/*
 * Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Filesystem_HPP
#define ecflow_core_Filesystem_HPP

#include <filesystem>

namespace fs = std::filesystem;

namespace ecf {
namespace fsx {

/**
 * Generate a unique path from the given model.
 * The one or more '%' characters in the model which will be replaced by random hexadecimal characters (0-9, A-F).
 *
 * @param model the model for the unique path
 * @return the unique path
 */
std::filesystem::path unique_path(std::string model);


/**
 * Get the last write time of the given path, as a std::chrono::system_clock::time_point.
 *
 * @param path the path
 * @return the last write time
 */
std::chrono::system_clock::time_point last_write_time(const std::filesystem::path& path);

} // namespace fsx
} // namespace ecf

#endif /* ecflow_core_Filesystem_HPP */
