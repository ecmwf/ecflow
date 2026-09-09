/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/parser/DefsStatusParser.hpp"

#include <sstream>
#include <stdexcept>

#include "ecflow/core/DState.hpp"
#include "ecflow/node/Node.hpp"

using namespace ecf;

bool DefsStatusParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    if (lineTokens.size() < 2) {
        throw std::runtime_error("DefsStatusParser::doParse: Invalid defstatus :" + line);
    }

    if (!DState::isValid(lineTokens[1])) {
        throw std::runtime_error("DefsStatusParser::doParse: Invalid defstatus state :" + line);
    }

    if (!nodeStack().empty()) {
        Node* node = nodeStack_top();

        // Check default status not already defined for this node.
        auto it = defStatusMap().find(node);
        if (it != defStatusMap().end()) {
            if ((*it).second) {
                throw std::runtime_error(MESSAGE("DefsStatusParser::doParse: " << node->debugType() << " "
                                                                               << node->name()
                                                                               << " already has a default status\n"));
            }
        }
        defStatusMap()[node] = true;
        node->addDefStatus(DState::toState(lineTokens[1]));
    }

    return true;
}
