/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

///
/// @file ExprParserV2.cpp
/// @brief Handcrafted lexer/parser for trigger/complete expressions (V2 implementation).
///
/// This translation unit will contain the full recursive-descent implementation once Phase 3 is
/// complete. During Phase 2 (skeleton) every call to doParse() returns false with an explicit
/// diagnostic, so the u_expr_v1_v2 harness reports the comparison delta without crashing.
///

#include "ecflow/node/ExprParserV2.hpp"

namespace ecf::expression::v2 {

ExprParser::ExprParser(const std::string& expression) : expr_(expression) {
}

bool ExprParser::doParse(std::string& errorMsg) {
    errorMsg = "V2 parser not yet implemented (Phase 2 skeleton)";
    return false;
}

} // namespace ecf::expression::v2
