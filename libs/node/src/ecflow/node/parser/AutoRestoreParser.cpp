/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/parser/AutoRestoreParser.hpp"

#include <stdexcept>

#include "ecflow/node/AutoRestoreAttr.hpp"
#include "ecflow/node/Node.hpp"

using namespace ecf;

bool AutoRestoreParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // autorestore /s1/f1
    // autorestore ../f1

    if (lineTokens.size() < 2) {
        throw std::runtime_error("AutoRestoreParser::doParse: Invalid autorestore :" + line);
    }
    if (nodeStack().empty()) {
        throw std::runtime_error(
            "AutoRestoreParser::doParse: Could not add autorestore as node stack is empty at line: " + line);
    }

    std::vector<std::string> nodes_to_restore;
    for (size_t i = 1; i < lineTokens.size(); i++) {
        if (lineTokens[i][0] == '#') {
            break;
        }
        nodes_to_restore.push_back(lineTokens[i]);
    }

    if (nodes_to_restore.empty()) {
        throw std::runtime_error("AutoRestoreParser::doParse: no paths specified " + line);
    }

    nodeStack_top()->add_autorestore(ecf::AutoRestoreAttr(nodes_to_restore));

    return true;
}
