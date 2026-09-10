/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_parser_DefsStateParser_HPP
#define ecflow_node_parser_DefsStateParser_HPP

#include "ecflow/node/parser/Parser.hpp"

class DefsStateParser : public Parser {
public:
    explicit DefsStateParser(DefsStructureParser* p)
        : Parser(p) {}
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
    const char* keyword() const override { return "defs_state"; }
};

class HistoryParser : public Parser {
public:
    explicit HistoryParser(DefsStructureParser* p)
        : Parser(p) {}
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
    const char* keyword() const override { return "history"; }
};

#endif /* ecflow_node_parser_DefsStateParser_HPP */
