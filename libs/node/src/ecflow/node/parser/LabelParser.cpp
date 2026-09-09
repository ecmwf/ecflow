/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/parser/LabelParser.hpp"

#include <stdexcept>

#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

using namespace ecf;

bool LabelParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    if (nodeStack().empty()) {
        throw std::runtime_error("LabelParser::doParse: Could not add label as node stack is empty at line: " + line);
    }

    std::string name, value, new_value;
    Label::parse(line, lineTokens, rootParser()->get_file_type() != PrintStyle::DEFS, name, value, new_value);

    bool check = (rootParser()->get_file_type() != PrintStyle::NET);

    nodeStack_top()->add_label(name, value, new_value, check);

    return true;
}
