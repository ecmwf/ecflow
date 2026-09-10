/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/parser/MeterParser.hpp"

#include <stdexcept>

#include "ecflow/core/Extract.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

using namespace ecf;

bool MeterParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // meter 0 100 100 # value
    size_t line_tokens_size = lineTokens.size();
    if (line_tokens_size < 4) {
        throw std::runtime_error("MeterParser::doParse: Invalid meter :" + line);
    }

    if (nodeStack().empty()) {
        throw std::runtime_error("MeterParser::doParse: Could not add meter as node stack is empty at line: " + line);
    }

    int min = Extract::value<int>(lineTokens[2], "Invalid meter : " + line);
    int max = Extract::value<int>(lineTokens[3], "Invalid meter : " + line);
    int colorChange =
        Extract::optional_value<int>(lineTokens, 4, std::numeric_limits<int>::max(), "Invalid meter : " + line);

    // state
    int value = std::numeric_limits<int>::max();
    if (rootParser()->get_file_type() != PrintStyle::DEFS) {
        bool comment_fnd = false;
        for (size_t i = 3; i < line_tokens_size; i++) {
            if (comment_fnd) {
                // token after comment is the value
                value = Extract::value<int>(lineTokens[i], "MeterParser::doParse, could not extract meter value");
                break;
            }
            if (lineTokens[i] == "#") {
                comment_fnd = true;
            }
        }
    }

    bool check = (rootParser()->get_file_type() != PrintStyle::NET);

    nodeStack_top()->add_meter(lineTokens[1], min, max, colorChange, value, check);

    return true;
}
