/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_parser_RepeatParser_HPP
#define ecflow_node_parser_RepeatParser_HPP

#include "ecflow/core/Chrono.hpp"
#include "ecflow/node/parser/Parser.hpp"

class RepeatParser : public Parser {
public:
    explicit RepeatParser(DefsStructureParser* p) : Parser(p) {}

    const char* keyword() const override { return "repeat"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;

private:
    // void extractDayMonthYear(const std::vector<std::string>& lineTokens,int& x, int& endDate);
    bool get_value(const std::vector<std::string>& lineTokens, int& value) const;
    bool get_value(const std::vector<std::string>& lineTokens, ecf::Instant& value) const;
};

#endif /* ecflow_node_parser_RepeatParser_HPP */
