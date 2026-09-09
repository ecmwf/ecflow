/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/parser/DateParser.hpp"

#include <stdexcept>

#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

using namespace ecf;

bool DateParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    //  date 15.11.2009 # <value>   // with PersistStyle::STATE & MIGRATE
    //  date 15.*.*
    //  date *.1.*
    if (lineTokens.size() < 2) {
        throw std::runtime_error("DateParser::doParse: Invalid date :" + line);
    }

    if (nodeStack().empty()) {
        throw std::runtime_error("DateParser::doParse: Could not add date as node stack is empty at line: " + line);
    }

    // DateAttr::create can throw for invalid dates
    nodeStack_top()->addDate(DateAttr::create(lineTokens, rootParser()->get_file_type() != PrintStyle::DEFS));

    return true;
}
