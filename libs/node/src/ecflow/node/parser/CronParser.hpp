/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_parser_CronParser_HPP
#define ecflow_node_parser_CronParser_HPP

#include "ecflow/node/parser/Parser.hpp"

class CronParser : public Parser {
public:
    explicit CronParser(DefsStructureParser* p)
        : Parser(p) {}
    const char* keyword() const override { return "cron"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
};

#endif /* ecflow_node_parser_CronParser_HPP */
