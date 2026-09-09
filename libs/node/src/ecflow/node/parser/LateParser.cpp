/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/parser/LateParser.hpp"

#include <stdexcept>

#include "ecflow/attribute/LateAttr.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

using namespace ecf;

bool LateParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    if (lineTokens.size() < 3) {
        throw std::runtime_error("LateParser::doParse: Invalid late :" + line);
    }

    // late -s +00:15  -a  20:00  -c +02:00     #The option can be in any order
    //  0   1    2      3   4     5     6        7     8     9  10 11 12  13
    // late -s +00:15  -c +02:00                # not all options are needed
    //  0    1   2      3   4                   5

    LateAttr lateAttr; // lateAttr.isNull() will return true;
    size_t start_index = 1;
    LateAttr::parse(lateAttr, line, lineTokens, start_index);

    // state
    if (rootParser()->get_file_type() != PrintStyle::DEFS && lineTokens[lineTokens.size() - 1] == "late") {
        lateAttr.setLate(true);
    }

    nodeStack_top()->addLate(lateAttr);
    return true;
}
