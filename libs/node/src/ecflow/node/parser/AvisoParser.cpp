/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/parser/AvisoParser.hpp"

#include <stdexcept>

#include "ecflow/core/Str.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

namespace {

auto parse_aviso_line(const std::string& line, Node* parent) {
    std::vector<std::string_view> tokens = ecf::Str::tokenize_quotation(line, "'");

    if (tokens.size() != 3 && tokens.size() != 4) {
        throw std::runtime_error("parse_line: Incorrect number of tokens found. Expected 3 tokens, at line: " + line);
    }

    ecf::AvisoAttr::name_t name         = std::string{tokens[1]};
    ecf::AvisoAttr::listener_t listener = std::string{tokens[2]};

    ecf::AvisoAttr::revision_t revision = 0;
    if (tokens.size() == 4) {
        revision = std::atoi(std::string{tokens[3]}.c_str());
    }

    // TODO[MB]: name & listener validation...

    return ecf::AvisoAttr{parent, name, listener, revision};
}

} // namespace

bool AvisoParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    if (nodeStack().empty()) {
        throw std::runtime_error("AvisoParser::doParse: Could not add aviso as node stack is empty at line: " + line);
    }

    Node* parent = nodeStack_top();

    auto parsed = parse_aviso_line(line, parent);
    nodeStack_top()->addAviso(parsed);

    return true;
}
