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
/// The containers storing the path components will be cleared before storing any results.
///
/// Multiple path separator '/' are treated as one separator, mimicing unix path conventions:
///  The path '/suite//family///task' will be split into [ 'suite', 'family', 'task' ]
///
/// @param path The input node path to split.
/// @param components The vector to store the extracted components of the path.
///
void split_path(const std::string& path, std::vector<std::string>& components);

///
/// @brief Retrieve host and port values from the given path.
///
/// Considers the path has the following form, and extracts the <host> and <port> values:
///     <host>:<port>/suite/family/task
///
/// @param path The input node path to extract host and port from.
/// @param host The buffer to store the extracted host value.
/// @param port The buffer to store the extracted port value.
/// @return true if host and port were successfully extracted, false otherwise.
///
bool extract_host_and_port_from_path(const std::string& path, std::string& host, std::string& port);

/// @brief Creates a node path based on the given a vector of strings
///
/// For the components [ "suite", "family", "task" ], returns the string "/suite/family/task".
///
/// @param components The vector with the node path components to use to create the node path string.
/// @return The node path string created from the components.
///
std::string create_node_path(const std::vector<std::string>& components);

///
/// @brief Remove host and port information from the given path.
///
/// @param path The input node path to remove host and port from.
/// @return The node path with host and port removed.
///
/// For the node path "/localhost:3141/suite/family/task", return the path "/suite/family/task"
std::string remove_host_and_port_from_path(const std::string& path);

///
/// @brief Check if the given path is an absolute path.
///
/// @return true if absolute path, false otherwise.
///
bool is_absolute_path(const std::string& path);

} // namespace node

} // namespace ecf

#endif /* ecflow_core_NodePath_HPP */
