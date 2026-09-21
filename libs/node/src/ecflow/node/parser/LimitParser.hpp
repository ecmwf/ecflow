// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/node/parser/Parser.hpp"

class LimitParser : public Parser {
public:
    explicit LimitParser(DefsStructureParser* p)
        : Parser(p) {}
    const char* keyword() const override { return "limit"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
};
