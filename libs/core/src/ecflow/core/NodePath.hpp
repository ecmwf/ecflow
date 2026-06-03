/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_NodePath_HPP
#define ecflow_core_NodePath_HPP

#include <string>
#include <vector>

namespace ecf {

namespace node {

///
/// @brief Split the given node path into its components, using '/' as separator.
///
/// The container storing the path components will be cleared before storing any results.
///
/// Multiple consecutive '/' separators are treated as one, mimicking unix path conventions:
///  the path '/suite//family///task' will be split into [ 'suite', 'family', 'task' ]
///
/// @param path       the input node path to split
/// @param components the vector to store the extracted path components
///
void split_path(const std::string& path, std::vector<std::string>& components);

///
/// @brief Extract host and port values from the given path.
///
/// Expects the path to have the following form and extracts the @c <host> and @c <port> values:
///     @c <host>:<port>/suite/family/task
///
/// @param path the input node path to extract host and port from
/// @param host the buffer to store the extracted host value
/// @param port the buffer to store the extracted port value
/// @return true if host and port were successfully extracted; false otherwise
///
bool extract_host_and_port_from_path(const std::string& path, std::string& host, std::string& port);

///
/// @brief Create a node path string from the given vector of path components.
///
/// For the components @c [ "suite", "family", "task" ], returns @c "/suite/family/task".
/// Returns an empty string if @p components is empty.
///
/// @param components the path components to join
/// @return the node path string created from the components
///
std::string create_node_path(const std::vector<std::string>& components);

///
/// @brief Remove the host and port prefix from the given path.
///
/// For the node path @c "<host>:<port>/suite/family/task", returns @c "/suite/family/task".
///
/// @param path the input node path to remove the host and port prefix from
/// @return the node path with the host and port prefix removed
///
std::string remove_host_and_port_from_path(const std::string& path);

///
/// @brief Check if the given path is an absolute path.
///
/// @param path the path to check
/// @return true if @p path is an absolute path; false otherwise
///
bool is_absolute_path(const std::string& path);

} // namespace node

} // namespace ecf

#endif /* ecflow_core_NodePath_HPP */
