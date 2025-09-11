/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/parser/DefsStateParser.hpp"

#include <stdexcept>

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

using namespace std;

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
