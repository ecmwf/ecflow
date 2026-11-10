// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/parser/OwnerParser.hpp"

#include <stdexcept>

#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/node/Node.hpp"

bool OwnerParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // expected format:
    //    owner <user>[:<group>]   # any comment after a first # is ignored
    if (lineTokens.size() < 2 || lineTokens[1][0] == '#') {
        throw std::runtime_error("OwnerParser::doParse: Missing owner value: " + line);
    }
    if (lineTokens.size() > 2 && lineTokens[2][0] != '#') {
        throw std::runtime_error("OwnerParser::doParse: The owner takes a single value: " + line);
    }
    if (nodeStack().empty()) {
        throw std::runtime_error("OwnerParser::doParse: Could not add owner as node stack is empty at line: " + line);
    }

    nodeStack_top()->addOwner(ecf::OwnerAttr::create(lineTokens[1]));
    return true;
}
