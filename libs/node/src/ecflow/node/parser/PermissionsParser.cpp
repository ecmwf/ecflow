// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/parser/PermissionsParser.hpp"

#include <stdexcept>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

bool PermissionsParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // permissions [{"user": "alice", "permissions": "rwx"}]   # optional comment

    // At the top level, the attribute is that of the server: only state files (checkpoints, synchronisation) carry
    // it, while definitions cannot set it
    const bool server_level = nodeStack().empty();
    if (server_level && rootParser()->get_file_type() == PrintStyle::DEFS) {
        throw std::runtime_error(
            "PermissionsParser::doParse: the server Permissions attribute cannot be set by definitions, at line: " +
            line);
    }

    // The JSON form may hold white space, and is therefore taken from the line rather than from its tokens; it cannot
    // hold '#', since user names exclude it
    auto value = line.substr(line.find(keyword()) + std::string(keyword()).size());
    if (auto comment = value.find('#'); comment != std::string::npos) {
        value.erase(comment);
    }

    try {
        auto permissions = ecf::Permissions::parse_json(value);
        if (server_level) {
            // The server has one attribute, as every node: a document declares it at most once; it replaces the
            // attribute the definitions held before
            if (server_attribute_read_) {
                throw std::runtime_error("the server can only have one permissions attribute");
            }
            server_attribute_read_ = true;
            defsfile()->server_state().set_permissions(permissions);
        }
        else {
            nodeStack_top()->add_permissions(permissions);
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("PermissionsParser::doParse: Invalid permissions '" + line + "': " + e.what());
    }
    return true;
}
