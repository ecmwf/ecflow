/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/parser/GenericParser.hpp"

#include <stdexcept>

#include "ecflow/attribute/GenericAttr.hpp"
#include "ecflow/node/Node.hpp"

using namespace ecf;

bool GenericParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // expected format:
    //    generic <key> <value1> <value2> .... # any comment after a first # is ignored
    // examples:
    //    generic fred va1 val2 # some comment
    //    generic fred # another comment
    if (lineTokens.size() < 2) {
        throw std::runtime_error("GenericParser::doParse: Invalid generic :" + line);
    }

    if (nodeStack().empty()) {
        throw std::runtime_error("GenericParser::doParse: Could not add generic as node stack is empty at line: " +
                                 line);
    }

    size_t line_tokens_size = lineTokens.size();
    std::vector<std::string> values;
    values.reserve(line_tokens_size);
    for (size_t i = 2; i < line_tokens_size; i++) {
        if (lineTokens[i][0] == '#') {
            break;
        }
        values.push_back(lineTokens[i]);
    }

    Node* node = nodeStack_top();
    node->add_generic(GenericAttr(lineTokens[1], values));
    return true;
}
