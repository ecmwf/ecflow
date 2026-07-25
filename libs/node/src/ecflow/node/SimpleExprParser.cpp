/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/SimpleExprParser.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stack>
#include <string>

#include "ecflow/core/Str.hpp"

namespace ecf::expression {

bool has_complex_expressions(const std::string& expr) {
    // we allow . and /
    if (expr.find('(') != std::string::npos) {
        return true;
    }
    if (expr.find(':') != std::string::npos) {
        return true;
    }
    if (expr.find('.') != std::string::npos) {
        return true;
    }
    if (expr.find('/') != std::string::npos) {
        return true;
    }
    if (expr.find(" not ") != std::string::npos) {
        return true;
    }
    if (expr.find(" and ") != std::string::npos) {
        return true;
    }
    if (expr.find(" or ") != std::string::npos) {
        return true;
    }
    if (expr.find('!') != std::string::npos) {
        return true;
    }
    if (expr.find("&&") != std::string::npos) {
        return true;
    }
    if (expr.find("||") != std::string::npos) {
        return true;
    }
    if (expr.find('<') != std::string::npos) {
        return true;
    }
    if (expr.find('>') != std::string::npos) {
        return true;
    }
    if (expr.find('+') != std::string::npos) {
        return true;
    }
    if (expr.find('-') != std::string::npos) {
        return true;
    }
    if (expr.find('*') != std::string::npos) {
        return true;
    }
    if (expr.find('~') != std::string::npos) {
        return true;
    }
    if (expr.find(" ne ") != std::string::npos) {
        return true;
    }
    if (expr.find(" ge ") != std::string::npos) {
        return true;
    }
    if (expr.find("<=") != std::string::npos) {
        return true;
    }
    if (expr.find(">=") != std::string::npos) {
        return true;
    }
    if (expr.find(" le ") != std::string::npos) {
        return true;
    }
    if (expr.find(" gt ") != std::string::npos) {
        return true;
    }
    if (expr.find(" lt ") != std::string::npos) {
        return true;
    }
    return false;
}

bool SimpleExprParser::doParse() {

    // If expression has complex operators, return false
    if (has_complex_expressions(expr_)) {
        return false;
    }

    // Start by trimming the original expression
    auto expression = expr_;
    ecf::algorithm::trim(expression);

    // If expression is empty, return false
    if (expression.empty()) {
        return false;
    }

    // If expression begins/ends with '==' or 'eq', return false
    //   n.b. for 'eq' the comparison is done considering the necessary whitespace after/before the operator
    if (ecf::algorithm::starts_with(expression, "==") || ecf::algorithm::starts_with(expression, "eq ") ||
        ecf::algorithm::ends_with(expression, "==") || ecf::algorithm::ends_with(expression, " eq")) {
        return false;
    }

    // Split the expression into tokens, considering '==' or 'eq' operators.
    //   n.b. for 'eq' the comparison is done considering the necessary whitespace before and after the operator
    std::vector<std::string> operands;
    if (expression.find("==") != std::string::npos) {
        // Expecting expressions such as:
        //   `/path/to/node==<state>`
        //   `/path/to/node == <state>`
        //   `<number>==<number>`
        //   `<number> == <number>`
        ecf::algorithm::split_by(operands, expression, "==");
    }
    else if (expression.find(" eq ") != std::string::npos) {
        // Or, expecting expressions such as:
        //   `/path/to/node eq <state>`
        //   `<number> eq <number>`
        ecf::algorithm::split_by(operands, expression, " eq ");
    }
    else {
        // If the expression does not contain simple operators ('==' or 'eq'), return false
        return false;
    }

    // If the expression does not have exactly two operands, return false
    if (operands.size() != 2) {
        return false;
    }

    // Trim the operands
    ecf::algorithm::trim(operands.front());
    ecf::algorithm::trim(operands.back());

    const auto& left_operand  = operands.front();
    const auto& right_operand = operands.back();

    // If the left operand (i.e. either a `/path/to/node` or `<number>`) contain spaces, return false
    if (left_operand.find(' ') != std::string::npos) {
        return false;
    }

    // If the right operand (i.e. either a `<state>` or `<number>`) contain spaces, return false
    if (right_operand.find(' ') != std::string::npos) {
        return false;
    }

    // If the right operand is a valid `<state>`), parse the expression and return true
    if (DState::isValid(right_operand)) {
        ast_          = std::make_unique<AstTop>();
        Ast* someRoot = new AstEqual();
        someRoot->addChild(new AstNode(left_operand));
        someRoot->addChild(new AstNodeState(DState::toState(right_operand)));
        ast_->addChild(someRoot);
        return true;
    }

    // Otherwise, the expression must be a comparison of two numerical values.
    // Parse the operands as integers, and return accordingly.
    try {
        auto left     = ecf::convert_to<int>(left_operand);
        auto right    = ecf::convert_to<int>(right_operand);
        ast_          = std::make_unique<AstTop>();
        Ast* someRoot = new AstEqual();
        someRoot->addChild(new AstInteger(left));
        someRoot->addChild(new AstInteger(right));
        ast_->addChild(someRoot);
        return true;
    }
    catch (const ecf::bad_conversion&) {
        return false;
    }
}

} // namespace ecf::expression
