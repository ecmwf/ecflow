// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/node/parser/Parser.hpp"

///
/// @brief Parses the `owner user[:group]` attribute of a suite, family or task.
///
class OwnerParser : public Parser {
public:
    explicit OwnerParser(DefsStructureParser* p)
        : Parser(p) {}
    const char* keyword() const override { return "owner"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
};
