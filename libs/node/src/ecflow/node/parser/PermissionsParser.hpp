// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/node/parser/Parser.hpp"

///
/// @brief Parses the line of a Permissions attribute: the keyword followed by the JSON form.
///
/// For example: permissions [{"user": "alice", "permissions": "rwx"}]
///
class PermissionsParser : public Parser {
public:
    explicit PermissionsParser(DefsStructureParser* p)
        : Parser(p) {}

    const char* keyword() const override { return "permissions"; }

    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;

private:
    bool server_attribute_read_{false}; // a server attribute was read from the document being parsed
};
