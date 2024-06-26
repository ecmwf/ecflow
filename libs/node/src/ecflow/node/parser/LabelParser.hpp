/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_parser_LabelParser_HPP
#define ecflow_node_parser_LabelParser_HPP

#include "ecflow/node/parser/Parser.hpp"

class LabelParser : public Parser {
public:
    explicit LabelParser(DefsStructureParser* p) : Parser(p) {}
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
    const char* keyword() const override { return "label"; }
};

#endif /* ecflow_node_parser_LabelParser_HPP */
