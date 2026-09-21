// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/node/parser/Parser.hpp"

class ClockParser : public Parser {
public:
    explicit ClockParser(DefsStructureParser* p)
        : Parser(p) {}
    const char* keyword() const override { return "clock"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
};

class EndClockParser : public Parser {
public:
    explicit EndClockParser(DefsStructureParser* p)
        : Parser(p) {}
    const char* keyword() const override { return "endclock"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
};
