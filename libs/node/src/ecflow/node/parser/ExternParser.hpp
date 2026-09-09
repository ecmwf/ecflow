/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_parser_ExternParser_HPP
#define ecflow_node_parser_ExternParser_HPP

#include "ecflow/node/parser/Parser.hpp"

///
/// Externs: will typically have a absolute path, however sometimes when
///          suite are generated incrementally relative paths can be added
/// extern a           path =  a
/// extern /a/b/c      path = /a/b/c
/// extern a/b/c       path = a/b/c
/// extern /a/b/c:YMD  path = /a/b/c   variable:YMD (i.e event, meter, variable, repeat, generated variable)
///
/// Externs are not persisted, why ?:
///   o Externs are un-resolved references to node paths in trigger expressions and inlimits
///     These references could be dynamically generated.
///   o Saves on network bandwidth and checkpoint file size.
/// Hence, externs are *ONLY* used on the client side.
///

class ExternParser : public Parser {
public:
    explicit ExternParser(DefsStructureParser* p)
        : Parser(p) {}
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
    const char* keyword() const override { return "extern"; }
};

#endif /* ecflow_node_parser_ExternParser_HPP */
