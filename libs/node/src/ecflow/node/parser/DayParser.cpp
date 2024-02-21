/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/parser/DayParser.hpp"

#include <stdexcept>

#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

using namespace std;

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
