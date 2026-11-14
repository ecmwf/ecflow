// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/parser/PermissionsParser.hpp"

#include <stdexcept>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/node/Node.hpp"

bool PermissionsParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // permissions [{"user": "alice", "permissions": "rwx"}]   # optional comment
    if (nodeStack().empty()) {
        throw std::runtime_error(
            "PermissionsParser::doParse: Could not add permissions as node stack is empty at line: " + line);
    }

    // The JSON form may hold white space, and is therefore taken from the line rather than from its tokens; it cannot
    // hold '#', since user names exclude it
    auto value = line.substr(line.find(keyword()) + std::string(keyword()).size());
    if (auto comment = value.find('#'); comment != std::string::npos) {
        value.erase(comment);
    }

    try {
        nodeStack_top()->add_permissions(ecf::Permissions::parse_json(value));
    }
    catch (const std::exception& e) {
        throw std::runtime_error("PermissionsParser::doParse: Invalid permissions '" + line + "': " + e.what());
    }
    return true;
}
