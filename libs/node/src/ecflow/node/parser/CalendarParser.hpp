/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_parser_CalendarParser_HPP
#define ecflow_node_parser_CalendarParser_HPP

#include "ecflow/node/parser/Parser.hpp"

class CalendarParser : public Parser {
public:
    explicit CalendarParser(DefsStructureParser* p)
        : Parser(p) {}
    const char* keyword() const override { return "calendar"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
};

#endif /* ecflow_node_parser_CalendarParser_HPP */
