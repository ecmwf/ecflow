/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_parser_AvisoParser_HPP
#define ecflow_node_parser_AvisoParser_HPP

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
 *    - string value (formed of <scheme>://<host>[:<port>]), mandatory
 *  --schema <value>
 *    - string value (path to the Aviso schema file, accessible to the ecFlow server), mandatory
 *  --revision <value>
 *    - unsigned integer value, optional (default: 0)
 *
 * The tokens can be separated provided in any order, with any number of spaces between them being disregarded.
 * Apart from the tokens above, retrieved from the definition line, the parser also determines the 'path' of the Node
 * to which the 'Aviso' attribute belongs -- this is stored in the parsed 'Aviso' object.
 */

class AvisoParser : public Parser {
public:
    static constexpr const char* keyword_aviso   = "aviso";
    static constexpr const char* option_name     = "name";
    static constexpr const char* option_listener = "listener";
    static constexpr const char* option_url      = "url";
    static constexpr const char* option_schema   = "schema";
    static constexpr const char* option_revision = "revision";

    explicit AvisoParser(DefsStructureParser* p) : Parser(p) {}
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;
    const char* keyword() const override { return "aviso"; }
};

#endif /* ecflow_node_parser_AvisoParser_HPP */
