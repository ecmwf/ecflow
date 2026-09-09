/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/parser/QueueParser.hpp"

#include <stdexcept>

#include "ecflow/attribute/QueueAttr.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

using namespace ecf;

bool QueueParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    if (nodeStack().empty()) {
        throw std::runtime_error("QueueParser::doParse: Could not add queue, as node stack is empty at line: " + line);
    }

    bool parse_state = false;
    if (rootParser()->get_file_type() != PrintStyle::DEFS) {
        parse_state = true;
    }

    QueueAttr queue_attr;
    QueueAttr::parse(queue_attr, line, lineTokens, parse_state);
    Node* node = nodeStack_top();
    node->add_queue(queue_attr);

    return true;
}
