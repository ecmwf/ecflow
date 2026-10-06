// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/parser/Parser.hpp"

/**
 * This class is used to parse an Aviso attribute for a Node, stored in a single line of a definition file.
 *
 * The Aviso definition line is composed of the following tokens:
 *  "aviso"
 *    - keyword, mandatory
 *  --name <value>
 *    - string value (must be valid name), mandatory
 *  --listener '<value>'
 *    - json value (enclosed in single quotes), mandatory
 *  --url <value>
 *    - string value (formed of <scheme>://<host>[:<port>]), optional (default: %ECF_AVISO_URL%)
 *  --revision <value>
 *    - unsigned integer value, optional (default: 0)
 *  --auth <value>
 *    - string value (path to the credentials file), optional (default: %ECF_AVISO_AUTH%)
 *  --reason <value>
 *    - string value (enclosed in single quotes), optional (default: empty)
 *  --collapse
 *    - flag, optional: a release consumes all the notifications received, instead of exactly one
 *  --event '<value>'
 *    - JSON value (enclosed in single quotes), optional, state only: the notification that released the node
 *
 * The Aviso v1 options --schema and --polling are rejected in a definition, but accepted and ignored when reading a
 * definition that carries state (e.g. a checkpoint written by an earlier version of ecFlow).
 *
 * The tokens can be separated provided in any order, with any number of spaces between them being disregarded.
 * Apart from the tokens above, retrieved from the definition line, the parser also determines the 'path' of the Node
 * to which the 'Aviso' attribute belongs -- this is stored in the parsed 'Aviso' object.
 */

class AvisoParser : public Parser {
public:
    static constexpr const char* keyword_aviso     = "aviso";
    static constexpr const char* option_name       = "name";
    static constexpr const char* option_listener   = "listener";
    static constexpr const char* option_url        = "url";
    static constexpr const char* option_v1_schema  = "schema";
    static constexpr const char* option_v1_polling = "polling";
    static constexpr const char* option_revision   = "revision";
    static constexpr const char* option_auth       = "auth";
    static constexpr const char* option_reason     = "reason";
    static constexpr const char* option_collapse   = "collapse";
    static constexpr const char* option_event      = "event";

    static ecf::AvisoAttr parse_aviso_line(const std::string& line);
    static ecf::AvisoAttr parse_aviso_line(const std::string& line, const std::string& name);
    static ecf::AvisoAttr parse_aviso_line(const std::string& line, const std::string& name, Node* parent);
    static ecf::AvisoAttr parse_aviso_line(const std::string& line, Node* parent);

    ///
    /// @brief Parses an Aviso attribute line.
    ///
    /// @param line              The line, starting with the keyword `aviso`.
    /// @param parent            The node owning the attribute, or nullptr.
    /// @param accept_v1_options When true, the Aviso v1 options --schema and --polling are accepted and ignored;
    ///                          otherwise, they are rejected.
    /// @return The parsed attribute.
    /// @throws std::runtime_error if the line is invalid, or uses an Aviso v1 option that is not accepted.
    ///
    static ecf::AvisoAttr parse_aviso_line(const std::string& line, Node* parent, bool accept_v1_options);

    explicit AvisoParser(DefsStructureParser* p)
        : Parser(p) {}
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
    const char* keyword() const override { return "aviso"; }
};
