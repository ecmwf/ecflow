/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_ExprParserV1_HPP
#define ecflow_node_ExprParserV1_HPP

#include <memory>
#include <string>

#include "ecflow/node/ExprAst.hpp"

namespace ecf::expression::v1 {

///
/// @brief Parses a trigger/complete expression and builds an abstract syntax tree (AST).
///
/// @details The parser applies a two-stage strategy:
///   1. A fast-path SimpleExprParser handles simple equality expressions without Boost.Spirit.
///   2. The full Boost.Spirit Classic grammar handles everything else.
///
/// The ExprDuplicate cache is consulted first so identical expressions incur the parse cost
/// only once per process lifetime.
///
/// @invariant After a successful doParse(), getAst() returns a non-null pointer and the
///            returned ast() unique_ptr is non-null until moved.
///
class ExprParser {
public:
    ///
    /// @brief Constructs an ExprParser bound to the given expression string.
    ///
    /// @param[in] expression The trigger/complete expression text to parse.
    ///
    explicit ExprParser(const std::string& expression);

    ExprParser(const ExprParser&)                  = delete;
    const ExprParser& operator=(const ExprParser&) = delete;
    ExprParser(ExprParser&&)                       = delete;
    ExprParser& operator=(ExprParser&&)            = delete;

    ~ExprParser() = default;

    ///
    /// @brief Parses the bound expression and builds the AST.
    ///
    /// @param[out] errorMsg Diagnostic message populated when parsing fails; empty on success.
    /// @return true when the expression is accepted and the AST is valid; false otherwise.
    ///
    bool doParse(std::string& errorMsg);

    ///
    /// @brief Returns ownership of the AST, leaving this parser with a null AST.
    ///
    /// @return The abstract syntax tree produced by doParse(), or an empty unique_ptr when
    ///         doParse() has not been called or returned false.
    ///
    std::unique_ptr<AstTop> ast() { return std::move(ast_); }

    ///
    /// @brief Returns a non-owning pointer to the AST.
    ///
    /// @return Raw pointer to the AST, or nullptr when doParse() has not succeeded.
    ///
    AstTop* getAst() const { return ast_.get(); }

private:
    std::unique_ptr<AstTop> ast_;
    std::string expr_;
};

} // namespace ecf::expression::v1

#endif /* ecflow_node_ExprParserV1_HPP */
