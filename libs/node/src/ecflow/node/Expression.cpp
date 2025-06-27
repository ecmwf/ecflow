/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/Expression.hpp"

#include <cassert>
#include <sstream>
#include <stdexcept>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Indentor.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/ExprParser.hpp"
#include "ecflow/node/Node.hpp"

using namespace std;
using namespace ecf;

///////////////////////////////////////////////////////////////////////////////////////////

std::string PartExpression::toString(const std::string& exprType) const {
    std::stringstream ss;
    ss << exprType; // trigger or complete
    switch (type_) {
        case PartExpression::FIRST:
            ss << " ";
            break;
        case PartExpression::AND:
            ss << " -a ";
            break;
        case PartExpression::OR:
            ss << " -o ";
            break;
        default:
            assert(false);
            break;
    }
    ss << exp_ << "\n";
    return ss.str();
}

std::unique_ptr<AstTop> PartExpression::parseExpressions(std::string& errorMsg) const {
    // #ifdef DEBUG
    //	cout << "PartExpression::parseExpressions '" << exp_ << "'\n";
    // #endif
    if (!exp_.empty()) {
        ExprParser expressionParser(exp_);
        if (expressionParser.doParse(errorMsg)) {

            // returns new allocated memory, if no errors
            std::unique_ptr<AstTop> ast = expressionParser.ast();

            if (errorMsg.empty())
                LOG_ASSERT(ast.get(), "");
            else
                LOG_ASSERT(!ast.get(), "");

            return ast;
        }
    }
    return std::unique_ptr<AstTop>();
}

//===========================================================================

Expression::Expression(const std::string& expression) {
    add(PartExpression(expression));
}

Expression::Expression(const PartExpression& exp) {
    add(exp);
}

Expression::Expression() = default;

Expression::Expression(const Expression& rhs) : vec_(rhs.vec_), free_(rhs.free_) {
}

std::unique_ptr<AstTop> Expression::parse(const std::string& expression_to_parse,
                                          const std::string& error_msg_context) {
    PartExpression exp(expression_to_parse);
    string parseErrorMsg;
    std::unique_ptr<AstTop> ast = exp.parseExpressions(parseErrorMsg);
    if (!ast.get()) {
        std::stringstream ss;
        ss << error_msg_context << " Failed to parse expression '" << expression_to_parse << "'.  " << parseErrorMsg;
        throw std::runtime_error(ss.str());
    }
    return ast;
}

std::unique_ptr<AstTop> Expression::parse_no_throw(const std::string& expression_to_parse,
                                                   std::string& error_msg_context) {
    PartExpression exp(expression_to_parse);
    string parseErrorMsg;
    std::unique_ptr<AstTop> ast = exp.parseExpressions(parseErrorMsg);
    if (!ast.get()) {
        std::stringstream ss;
        ss << error_msg_context << " Failed to parse expression '" << expression_to_parse << "'.  " << parseErrorMsg;
        error_msg_context = ss.str();
    }
    return ast;
}

std::string Expression::compose_expression(const std::vector<PartExpression>& vec) {
    string ret;
    auto theEnd = vec.end();
    for (auto expr = vec.begin(); expr != theEnd; ++expr) {
        if ((*expr).andExpr())
            ret += " AND ";
        else if ((*expr).orExpr())
            ret += " OR ";
        ret += (*expr).expression();
    }
    return ret;
}

void Expression::add(const PartExpression& t) {
    if (vec_.empty()) {
        // The first expression should not have AND or OR
        if (t.andExpr() || t.orExpr()) {
            std::stringstream ss;
            ss << "Expression::add: expression " << t.expression()
               << " failed: The first expression should not have AND or OR set";
            throw std::runtime_error(ss.str());
        }
    }
    else {
        // Subsequent expression must be AND or OR expressions
        if (!t.andExpr() && !t.orExpr()) {
            std::stringstream ss;
            ss << "Expression::add: expression " << t.expression()
               << " failed: Subsequent expression must have AND or OR set";
            throw std::runtime_error(ss.str());
        }
    }
    vec_.push_back(t);
    //	cout << "Expression::add " << expression() << "\n";
}

void Expression::add_expr(const std::vector<PartExpression>& vec) {
    for (auto part : vec) {
        if (!empty() && part.expr_type() == PartExpression::FIRST)
            part.set_expr_type(PartExpression::AND);
        add(part);
    }
}

// ============================================================================
// CREATE AST tree for each expression, and COMBINE AST for each expression into a single AST.
// ============================================================================
void Expression::createAST(Node* node, const std::string& exprType, std::string& errorMsg) const {
    size_t theSize = vec_.size();
    for (size_t i = 0; i < theSize; i++) {
        std::string localErrorMsg;
        std::unique_ptr<AstTop> ast = vec_[i].parseExpressions(localErrorMsg);
        if (ast.get()) {

            // We can have multiple trigger/complete expression, combine to a single AST tree
            if (theCombinedAst_.get()) {
                // Must be trigger with -a(and) or -o(or) option's
                LOG_ASSERT(theCombinedAst_->isTop(), "");
                LOG_ASSERT(ast->isTop(), "");
                /* Combine AST tree
                                          top     top2 (this top needs to be deleted)       top            top2
                                           |        |                                ===>    |              |
                                          root1    root2                                   newRoot         NULL;
                                                                                            / \
                                                                                        root1  root2
                 */
                Ast* newRoot = nullptr;
                if (vec_[i].andExpr())
                    newRoot = new AstAnd();
                else if (vec_[i].orExpr())
                    newRoot = new AstOr();
                else
                    LOG_ASSERT(false, ""); // what else can it be.

                if (newRoot) {
                    newRoot->addChild(theCombinedAst_->left());
                    newRoot->addChild(ast->left());
                    theCombinedAst_->addChild(newRoot); // will overwrite

                    // Since we have transferred over root2 it must be set to NULL for top2,
                    // to avoid its child destruction
                    ast->addChild(nullptr); // since its an unique_ptr, no need for explicit delete
                }
            }
            else {
                // The very first expression should _NOT_ be AND/OR trigger. (i.e no -o | -a)
                LOG_ASSERT((!vec_[i].andExpr()) && (!vec_[i].orExpr()), "");
                theCombinedAst_ = std::move(ast); // transfer ownership
                theCombinedAst_->exprType(exprType);
            }
            //			cout << "****************************************************************\n";
            //			cout << theCombinedAst->expression() << "\n";
            //			cout << *theCombinedAst << "\n";
        }
        else {
            std::stringstream ss;
            ss << "Failed to parse " << vec_[i].toString(exprType) << " at " << node->debugNodePath() << " because "
               << localErrorMsg << "\n\n";
            errorMsg += ss.str();
            break;
        }
    }

    if (theCombinedAst_.get()) {
        theCombinedAst_->setParentNode(node);
    }
}

void Expression::setFree() {
    // Only update for a real change
    if (!free_) {
        state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Expression::setFree()\n";
#endif
    }
    free_ = true;
}

void Expression::clearFree() {
    // Only update for a real change
    if (free_) {
        state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Expression::clearFree()\n";
#endif
    }
    free_ = false;
}

template <class Archive>
void PartExpression::serialize(Archive& ar) {
    ar(CEREAL_NVP(exp_));
    CEREAL_OPTIONAL_NVP(ar, type_, [this]() { return type_ != default_expr_type(); }); // conditionally save
}

template <class Archive>
void Expression::serialize(Archive& ar) {
    ar(CEREAL_NVP(vec_));
    CEREAL_OPTIONAL_NVP(ar, free_, [this]() { return free_; }); // conditionally save
}

CEREAL_TEMPLATE_SPECIALIZE(PartExpression);
CEREAL_TEMPLATE_SPECIALIZE(Expression);
