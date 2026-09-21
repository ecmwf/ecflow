// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/node/parser/Parser.hpp"

class DefsParser : public Parser {
public:
    explicit DefsParser(DefsStructureParser* p);
    DefsParser(DefsStructureParser* p, bool node_parser_only);
    const char* keyword() const override { return "DEFS"; }
};
