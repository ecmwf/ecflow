/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/parser/TodayParser.hpp"

#include <stdexcept>

#include "ecflow/attribute/TodayAttr.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

using namespace ecf;
using namespace std;

bool TodayParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    size_t line_tokens_size = lineTokens.size();
    if (line_tokens_size < 2) {
        throw std::runtime_error("TodayParser::doParse: Invalid today :" + line);
    }

    bool parse_state = false;
    bool isFree      = false;
    if (rootParser()->get_file_type() != PrintStyle::DEFS) {
        parse_state      = true;
        bool comment_fnd = false;
        for (size_t i = 2; i < line_tokens_size; i++) {
            if (comment_fnd && lineTokens[i] == "free") {
                isFree = true;
            }
            if (lineTokens[i] == "#") {
                comment_fnd = true;
            }
        }
    }

    size_t index = 1;
    TodayAttr attr(TimeSeries::create(index, lineTokens, parse_state));
    if (isFree) {
        attr.setFree();
    }

    nodeStack_top()->addToday(attr);
    return true;
}
