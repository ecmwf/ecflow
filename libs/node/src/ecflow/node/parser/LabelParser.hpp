/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_parser_LabelParser_HPP
#define ecflow_node_parser_LabelParser_HPP

#include "ecflow/node/parser/Parser.hpp"

class LabelParser : public Parser {
public:
    explicit LabelParser(DefsStructureParser* p)
        : Parser(p) {}
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
    const char* keyword() const override { return "label"; }
};

#endif /* ecflow_node_parser_LabelParser_HPP */
