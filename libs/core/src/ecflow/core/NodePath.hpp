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

class NodePath {
public:
    // Disable default construction
    NodePath() = delete;
    // Disable copy (and move) semantics
    NodePath(const NodePath&)                  = delete;
    const NodePath& operator=(const NodePath&) = delete;

    /// returns the path as a vector of strings, preserving the order
    /// Note: multiple path separator '/' are treated as one separator.
    /// Mimics unix path conventions. hence
    /// '/suite//family///task' will be extracted as 'suite','family','task'
    static void split(const std::string& path, std::vector<std::string>&);

    /// If the path has form:
    ///     <host>:<port>/suite/family/task
    /// extract the host and port. Return OK, if successful
    static bool extractHostPort(const std::string& path, std::string& host, std::string& port);

    /// Given a vector of strings , create a path. "suite","family", returns /suite/family
    static std::string createPath(const std::vector<std::string>&);

    /// Given a path like:   //localhost:3141/suite/family/task
    /// returns              /suite/family/task
    static std::string removeHostPortFromPath(const std::string& path);
};

#endif /* ecflow_core_NodePath_HPP */
