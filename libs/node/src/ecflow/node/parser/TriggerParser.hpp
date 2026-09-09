/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_parser_TriggerParser_HPP
#define ecflow_node_parser_TriggerParser_HPP

#include "ecflow/node/parser/Parser.hpp"

class TriggerCompleteParser : public Parser {
protected:
    explicit TriggerCompleteParser(DefsStructureParser* p)
        : Parser(p) {}
    void getExpression(const std::string& line,
                       std::vector<std::string>& lineTokens,
                       std::string& expression,
                       bool& andExr,
                       bool& orExpr,
                       bool& isFree) const;
};

class TriggerParser : public TriggerCompleteParser {
public:
    explicit TriggerParser(DefsStructureParser* p)
        : TriggerCompleteParser(p) {}
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
    const char* keyword() const override { return "trigger"; }
};

class CompleteParser : public TriggerCompleteParser {
public:
    explicit CompleteParser(DefsStructureParser* p)
        : TriggerCompleteParser(p) {}
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
    const char* keyword() const override { return "complete"; }
};

#endif /* ecflow_node_parser_TriggerParser_HPP */
