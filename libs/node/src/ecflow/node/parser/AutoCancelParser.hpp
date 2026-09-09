/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_parser_AutoCancelParser_HPP
#define ecflow_node_parser_AutoCancelParser_HPP

#include "ecflow/node/parser/Parser.hpp"

class AutoCancelParser : public Parser {
public:
    explicit AutoCancelParser(DefsStructureParser* p)
        : Parser(p) {}
    const char* keyword() const override { return "autocancel"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
};

#endif /* ecflow_node_parser_AutoCancelParser_HPP */
