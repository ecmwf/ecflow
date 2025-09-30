/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/parser/InlimitParser.hpp"

#include <stdexcept>

#include "ecflow/core/Extract.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

using namespace std;

bool InlimitParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // inlimit /suite:queue1
    // inlimit disk 50
    // inlimit -n /suite:queue1 2
    // inlimit -n fam
    size_t lineTokens_size = lineTokens.size();
    if (lineTokens_size < 2) {
        throw std::runtime_error("InlimitParser::doParse: Invalid inlimit :" + line);
    }

    if (nodeStack().empty()) {
        throw std::runtime_error("InlimitParser::doParse: Could not add inlimit as node stack is empty at line: " +
                                 line);
    }

    bool limit_this_node_only = false;
    int token_pos             = 1;
    if (lineTokens[token_pos] == "-n") {
        limit_this_node_only = true;
        token_pos++;
    }
    bool limit_submission = false;
    if (lineTokens[token_pos] == "-s") {
        limit_submission = true;
        token_pos++;
    }
    if (limit_this_node_only && limit_submission) {
        throw std::runtime_error(
            "InlimitParser::doParse: can't limit family only(-n) and limit submission(-s) at the same time");
    }

    string path_to_node_holding_the_limit;
    string limitName;
    if (!Extract::pathAndName(lineTokens[token_pos], path_to_node_holding_the_limit, limitName)) {
        throw std::runtime_error("InlimitParser::doParse: Invalid inlimit : " + line);
    }

    token_pos++;
    int tokens = Extract::optional_value<int>(lineTokens, token_pos, 1, "Invalid in limit : " + line);

    bool check = (rootParser()->get_file_type() != PrintStyle::NET);

    InLimit inlimit(limitName, path_to_node_holding_the_limit, tokens, limit_this_node_only, limit_submission, check);
    if (rootParser()->get_file_type() != PrintStyle::DEFS) {
        token_pos++;
        bool incremented = false;
        for (size_t i = token_pos; i < lineTokens_size; i++) {
            // see InLimit::print(..) is to why "incremented:1"
            if (lineTokens[i].find("incremented:1") != std::string::npos) {
                incremented = true;
                break;
            }
        }
        inlimit.set_incremented(incremented);
    }

    //  cout << inlimit.toString() << "\n";
    Node* node = nodeStack_top();
    node->addInLimit(inlimit, check);

    return true;
}
