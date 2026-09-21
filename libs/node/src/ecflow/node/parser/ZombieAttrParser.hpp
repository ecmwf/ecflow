// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/node/parser/Parser.hpp"

class ZombieAttrParser : public Parser {
public:
    explicit ZombieAttrParser(DefsStructureParser* p)
        : Parser(p) {}
    const char* keyword() const override { return "zombie"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
};
