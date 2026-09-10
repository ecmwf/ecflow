/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/parser/DefsStateParser.hpp"

#include <stdexcept>

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

bool DefsStateParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // cout << "line = " << line << "\n";
    if (lineTokens.size() < 2) {
        throw std::runtime_error("DefsStateParser::doParse Invalid defs_state " + line);
    }

    if (lineTokens[1] == PrintStyle::to_string(PrintStyle::STATE)) {
        rootParser()->set_file_type(PrintStyle::STATE);
    }
    else if (lineTokens[1] == PrintStyle::to_string(PrintStyle::MIGRATE)) {
        rootParser()->set_file_type(PrintStyle::MIGRATE);
    }
    else if (lineTokens[1] == PrintStyle::to_string(PrintStyle::NET)) {
        rootParser()->set_file_type(PrintStyle::NET);
    }
    else {
        throw std::runtime_error("DefsStateParser::doParse: file type not specified : " + line);
    }

    defsfile()->read_state(line, lineTokens); // this can throw
    return true;
}

bool HistoryParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // cout << "line = " << line << "\n";
    defsfile()->read_history(line, lineTokens); // this can throw
    return true;
}
