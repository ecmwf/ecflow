/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_SimpleExprParser_HPP
#define ecflow_node_SimpleExprParser_HPP

#include <memory>
#include <string>

#include "ecflow/node/ExprAst.hpp"

namespace ecf::expression {

///
/// @brief Provides a fast-path parser for simple equality expressions, bypassing Boost.Spirit.
///
/// @details Handles expressions of the following forms without the Boost.Spirit overhead:
///   - @c /path/to/node==<state>
///   - @c /path/to/node == <state>
///   - @c /path/to/node eq <state>
///   - @c <integer>==<integer>
///   - @c <integer> == <integer>
///   - @c <integer> eq <integer>
///
/// When the expression does not match any simple form, doParse() returns false and the caller
/// falls back to the full Spirit parser.
///
class SimpleExprParser {
public:
    ///
    /// @brief Constructs a SimpleExprParser bound to the given expression string.
    ///
    /// @param[in] expression The expression text to attempt fast-path parsing on.
    ///
    explicit SimpleExprParser(const std::string& expression)
        : expr_(expression) {}

    SimpleExprParser(const SimpleExprParser&)            = delete;
    SimpleExprParser& operator=(const SimpleExprParser&) = delete;
    SimpleExprParser(SimpleExprParser&&)                 = delete;
    SimpleExprParser& operator=(SimpleExprParser&&)      = delete;

    ///
    /// @brief Attempts to parse the bound expression via the simple fast-path.
    ///
    /// @return true when the expression is a recognised simple form and was parsed
    ///         successfully; false otherwise.
    ///
    bool doParse();

    ///
    /// @brief Returns ownership of the AST produced by a successful doParse().
    ///
    /// @return The AST, or an empty unique_ptr when doParse() returned false.
    ///
    std::unique_ptr<AstTop> ast() { return std::move(ast_); }

private:
    const std::string& expr_;
    std::unique_ptr<AstTop> ast_;
};

} // namespace ecf::expression

#endif /* ecflow_node_SimpleExprParser_HPP */
