/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/parser/VerifyParser.hpp"

#include <stdexcept>

#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Extract.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

using namespace ecf;
using namespace std;

bool VerifyParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // expect:
    //    verify <state>:<expected> # actual  i.e
    //    verify complete:3 # 2
    if (lineTokens.size() < 2)
        throw std::runtime_error("VerifyParser::doParse: Invalid verify :" + line);

    if (!nodeStack().empty()) {
        Node* node = nodeStack_top();

        std::string stateInt = lineTokens[1];

        size_t colonPos = stateInt.find_first_of(':');
        if (colonPos == std::string::npos)
            throw std::runtime_error("Invalid verify :" + line);

        std::string state    = stateInt.substr(0, colonPos);
        std::string expected = stateInt.substr(colonPos + 1);
        // cout << "state = " << state << "\n";
        // cout << "expected = " << expected << "\n";

        if (!NState::isValid(state)) {
            throw std::runtime_error("VerifyParser::doParse: Invalid state :" + line);
        }

        NState::State theState  = NState::toState(state);
        int theExpectedStateCnt = Extract::theInt(expected, "Invalid verify");

        // STATE
        int actual = 0;
        if (rootParser()->get_file_type() != PrintStyle::DEFS) {
            if (lineTokens.size() >= 4) {
                if (lineTokens[2] == "#") {
                    try {
                        actual = ecf::convert_to<int>(lineTokens[3]);
                    }
                    catch (const ecf::bad_conversion&) { /* ignore could be other comment */
                    }
                }
            }
        }

        node->addVerify(VerifyAttr(theState, theExpectedStateCnt, actual));
    }
    return true;
}
