/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/parser/DayParser.hpp"

#include <stdexcept>

#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

bool DayParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    //  day monday  # free expired
    //  day tuesday # expired
    if (lineTokens.size() < 2) {
        throw std::runtime_error("DayParser::doParse: Invalid day :" + line);
    }
    if (nodeStack().empty()) {
        throw std::runtime_error("DayParser::doParse: Could not add day as node stack is empty at line: " + line);
    }

    // parse day and state
    nodeStack_top()->addDay(DayAttr::create(lineTokens, rootParser()->get_file_type() != PrintStyle::DEFS));

    return true;
}
