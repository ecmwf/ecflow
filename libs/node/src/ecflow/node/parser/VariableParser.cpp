/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/parser/VariableParser.hpp"

#include <stdexcept>

#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

using namespace ecf;

bool VariableParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // Note: For migrate the defs can have variables
    Node* node = nullptr;
    if (nodeStack().empty()) {
        if (!parsing_defs_) {
            throw std::runtime_error(
                "VariableParser::doParse: Could not add variable, as node stack is empty at line: " + line);
        }
    }
    else {
        node = nodeStack_top();
    }

    size_t line_tokens_size = lineTokens.size();
    if (line_tokens_size < 3) {
        std::ostringstream ss;
        ss << "VariableParser::doParse: expected at least 3 tokens, found " << line_tokens_size << " on line:" << line
           << "\n";
        if (node) {
            ss << "At node: " << node->debugNodePath() << "\n";
        }
        throw std::runtime_error(ss.str());
    }

    //
    // We essentially repeat the tokenisation process, because we need the address of the
    // first value character. This is used as a bounding box to extract the value from the line,
    // and thus avoid loosing repeated spaces in the variable value.
    //
    std::vector<std::string_view> token_idxs;
    ecf::algorithm::split_at(token_idxs, line);

    //
    // Determine the quote character
    // (1) Get the first character of the value, which is expected to be the 3rd token in the line (i.e. index 2).
    //
    // The first character can be:
    //  - ', meaning that the value is surrounded by single quotes
    //  - ", meaning that the value is surrounded by double quotes
    //  - any other character, meaning that the value is not quoted and extends until the next whitespace (or eol)
    //
    auto value_token         = token_idxs.at(2);
    auto first_char_of_value = value_token[0];

    //
    // (2) Apart from the selected quote character, determine also the first and last quote character locations.
    // This range defines the variable value, which can be any string of characters even if it contains spaces.
    //
    size_t first_quote  = std::string::npos;
    size_t last_quote   = std::string::npos;
    char selected_quote = '\0';

    const auto s_quote = '\'';
    const auto d_quote = '\"';

    if (first_char_of_value != s_quote && first_char_of_value != d_quote) {
        // Neither ' nor " were found
        // Markers are set to indicate as much
    }
    else {
        // Found a quote character, determine the quote type and the last quote location
        auto addr_at_0     = static_cast<const char*>(&line[0]);
        auto addr_at_quote = static_cast<const char*>(&token_idxs.at(2)[0]);

        first_quote    = std::distance(addr_at_0, addr_at_quote);
        selected_quote = first_char_of_value;
        last_quote     = line.find_last_of(selected_quote);
    }

    //
    // In case of a delimited value, there must be two quotes (marking the begin and end of the value)
    //
    if (first_quote != std::string::npos && last_quote != std::string::npos && !(first_quote < last_quote)) {
        std::ostringstream ss;
        ss << "VariableParser::doParse: Mismatched quote detected (only one quote found) in line: " << line << "\n";
        if (node) {
            ss << "At node: " << node->debugNodePath() << "\n";
        }
        throw std::runtime_error(ss.str());
    }

    std::string name;
    std::string value;
    std::string marker;

    if (lineTokens[1].find(':') != std::string::npos) {
        // CASE: handling an alias variable...
        //   a) edit <name>:<default-value> '<actual-value>' # some comment
        //   b) edit <name>:<some default value> '<some actual value>' # some comment
        //
        // - The presence of the `:` character in the second token is used as an indicator for
        //   an alias variable, which has a different format than a regular variable.
        //
        // - If single quotes are present, then they enclose the actual value.
        //
        // - If single quotes are absent, then the 2nd and 3rd tokens in the line are the name and value respectively,
        //   and the rest of the line is considered as comentary
        //
        // - consecutive spaces in both default and actual value are retained without loss

        auto addr_at_0    = static_cast<const char*>(&line[0]);
        auto addr_at_name = static_cast<const char*>(&token_idxs.at(1)[0]);

        size_t name_start        = std::distance(addr_at_0, addr_at_name);
        size_t first_alias_quote = line.find('\'');
        size_t last_alias_quote  = line.rfind('\'');

        if (first_alias_quote == std::string::npos && last_alias_quote == std::string::npos) {

            name  = lineTokens[1];
            value = lineTokens[2];

            const size_t first_coment_token = 3;
            for (size_t i = first_coment_token; i < lineTokens.size(); ++i) {
                if (i > first_coment_token) {
                    marker += " ";
                }
                marker += lineTokens[i];
            }
        }
        else {
            name   = line.substr(name_start, first_alias_quote - name_start - 1);
            value  = line.substr(first_alias_quote + 1, last_alias_quote - first_alias_quote - 1);
            marker = line.substr(last_alias_quote + 1);
        }
    }
    else {
        if (first_quote == std::string::npos && last_quote == std::string::npos) {
            // CASE: unquoted value, e.g.
            //   1) edit <name> <value>            [OK]
            //   2) edit <name> <value> # comment  [OK]
            //   3) edit <name> # comment          [NOK!]

            name  = lineTokens[1];
            value = lineTokens[2];

            // Handle case 3)!
            if (value[0] == '#') {
                std::ostringstream ss;
                ss << "VariableParser::doParse: Expected value but found comment at line:" << line << "\n";
                if (node) {
                    ss << "At node: " << node->debugNodePath() << "\n";
                }
                throw std::runtime_error(ss.str());
            }

            const size_t first_coment_token = 3;
            for (size_t i = first_coment_token; i < lineTokens.size(); ++i) {
                if (i > first_coment_token) {
                    marker += " ";
                }
                marker += lineTokens[i];
            }
        }
        else {
            // CASE: quoted value, e.g.
            //   1) edit <name> '<value>' (# comment)
            //   2) edit <name> "<value>" (# comment)

            name   = lineTokens[1];
            value  = line.substr(first_quote + 1, last_quote - first_quote - 1);
            marker = line.substr(last_quote + 1);
        }
    }

    //
    // Ensure that after the extracted there is always a # indicating the start of a comment
    //
    auto comment = std::find_if(marker.begin(), marker.end(), [](char c) { return c != ' ' and c != '\t'; });
    if (comment != marker.end() && *comment != '#') {
        std::ostringstream ss;
        ss << "VariableParser::doParse: Invalid comment at line: " << line << " -- non-# found after value\n";
        if (node) {
            ss << "At node: " << node->debugNodePath() << "\n";
        }
        throw std::runtime_error(ss.str());
    }

    //
    // Retrieve print style
    //
    bool net = rootParser()->get_file_type() == PrintStyle::NET;

    //
    // Populate the tree, with the actual variable
    //
    if (node) {
        if (net || node->isAlias()) {
            node->add_variable_bypass_name_check(name, value); // bypass name checking
        }
        else {
            node->add_variable(name, value);
        }
    }
    else {
        //
        // The distinction between server and user variables, is done by checking the presence of a comment `# server`
        //
        // This comment is only considered for variables defined at the root level (i.e. when nodeStack is empty),
        // as variables defined at any other level are always user variables.
        //
        ecf::algorithm::trim(marker);
        bool server_variable = marker == "# server";
        if (server_variable) {
            defsfile()->server_state().add_or_update_server_variable(name, value);
        }
        else {
            defsfile()->server_state().add_or_update_user_variables(name, value);
        }
    }

    return true;
}
