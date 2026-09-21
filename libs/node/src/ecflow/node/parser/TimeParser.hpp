// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/node/parser/Parser.hpp"

class TimeParser : public Parser {
public:
    explicit TimeParser(DefsStructureParser* p)
        : Parser(p) {}
    const char* keyword() const override { return "time"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
};
