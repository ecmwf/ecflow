/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_parser_RepeatParser_HPP
#define ecflow_node_parser_RepeatParser_HPP

#include "ecflow/core/Chrono.hpp"
#include "ecflow/node/parser/Parser.hpp"

class RepeatParser : public Parser {
public:
    explicit RepeatParser(DefsStructureParser* p)
        : Parser(p) {}

    const char* keyword() const override { return "repeat"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;

private:
    // void extractDayMonthYear(const std::vector<std::string>& lineTokens,int& x, int& endDate);
    bool get_value(const std::vector<std::string>& lineTokens, int& value) const;
    bool get_value(const std::vector<std::string>& lineTokens, ecf::Instant& value) const;
};

#endif /* ecflow_node_parser_RepeatParser_HPP */
