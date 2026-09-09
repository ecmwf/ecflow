/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/core/NodePath.hpp"

#include "ecflow/core/Str.hpp"

namespace ecf {

namespace node {

void split_path(const std::string& path, std::vector<std::string>& thePath) {
    /// The path is of the form "/suite/family/task"
    ecf::algorithm::split_at(thePath, path, ecf::string_constants::path_separator);
}

bool extract_host_and_port_from_path(const std::string& path, std::string& host, std::string& port) {
    if (path.empty()) {
        return false;
    }

    std::vector<std::string> thePath;
    split_path(path, thePath);

    if (thePath.empty()) {
        return false;
    }

    //<host>:<port>/suite/family/task
    // first path should be of form <host>:<port>
    size_t colonPos = thePath[0].find_first_of(':');
    if (colonPos == std::string::npos) {
        return false;
    }

    host = thePath[0].substr(0, colonPos);
    port = thePath[0].substr(colonPos + 1);

    ecf::algorithm::trim(host);
    ecf::algorithm::trim(port);
    if (host.empty()) {
        return false;
    }
    if (port.empty()) {
        return false;
    }

    return true;
}

std::string create_node_path(const std::vector<std::string>& vec) {
    if (vec.empty()) {
        return std::string{};
    }

    std::string ret;
    size_t size = vec.size();
    for (size_t i = 0; i < size; i++) {
        ret += ecf::string_constants::path_separator;
        ret += vec[i];
    }
    return ret;
}

std::string remove_host_and_port_from_path(const std::string& path) {
    std::vector<std::string> pathVec;
    split_path(path, pathVec);

    pathVec.erase(pathVec.begin());
    return create_node_path(pathVec);
}

bool is_absolute_path(const std::string& path) {
    return !path.empty() && path[0] == '/';
}

} // namespace node

} // namespace ecf
