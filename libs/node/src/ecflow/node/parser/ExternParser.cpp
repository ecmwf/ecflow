/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/parser/ExternParser.hpp"

#include <stdexcept>

#include "ecflow/node/Defs.hpp"

bool ExternParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // cout << "line = " << line << "\n";
    if (lineTokens.size() < 2) {
        throw std::runtime_error("ExternParser::doParse Invalid extern " + line);
    }

    // Guard against
    // extern   # empty extern with a comment
    // extern   #empty extern with a comment
    if (lineTokens[1][0] == '#') {
        throw std::runtime_error("ExternParser::doParse Invalid extern paths." + line);
    }

    // Expecting:
    //   extern <path>
    //   extern <path>:<attr>
    // where attr is the name of [ event, meter, repeat, variable, generated variable ]
    //
    // We will not split it up:

    // cout << "add extern  = '" << lineTokens[1] << "'\n";
    defsfile()->add_extern(lineTokens[1]);

    return true;
}
