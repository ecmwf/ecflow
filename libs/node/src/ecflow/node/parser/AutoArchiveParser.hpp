/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_parser_AutoArchiveParser_HPP
#define ecflow_node_parser_AutoArchiveParser_HPP

#include "ecflow/node/parser/Parser.hpp"

class AutoArchiveParser : public Parser {
public:
    explicit AutoArchiveParser(DefsStructureParser* p)
        : Parser(p) {}
    const char* keyword() const override { return "autoarchive"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
};

#endif /* ecflow_node_parser_AutoArchiveParser_HPP */
