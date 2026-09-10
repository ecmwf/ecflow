/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/parser/CalendarParser.hpp"

#include <stdexcept>

#include "ecflow/node/Suite.hpp"

using namespace ecf;

bool CalendarParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    if (lineTokens.size() < 2) {
        throw std::runtime_error("CalendarParser::doParse: Invalid calendar :" + line);
    }
    if (nodeStack().empty()) {
        throw std::runtime_error("CalendarParser::doParse: Could not add calendar as node stack is empty at line: " +
                                 line);
    }

    Suite* suite = nodeStack_top()->isSuite();
    if (!suite) {
        throw std::runtime_error("Calendar can only be added to suites and not " + nodeStack_top()->debugType());
    }
    suite->set_calendar().read_state(line, lineTokens);

    return true;
}
