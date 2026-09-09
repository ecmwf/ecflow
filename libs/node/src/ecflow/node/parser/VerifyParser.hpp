/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_parser_VerifyParser_HPP
#define ecflow_node_parser_VerifyParser_HPP

///
/// \brief A verify attribute parser.
///
/// Note that verify are only used for verification and do not constitute to the structure of a definition file
///

#include "ecflow/node/parser/Parser.hpp"

class VerifyParser : public Parser {
public:
    explicit VerifyParser(DefsStructureParser* p)
        : Parser(p) {}
    const char* keyword() const override { return "verify"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
};

#endif /* ecflow_node_parser_VerifyParser_HPP */
