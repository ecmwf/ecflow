/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_ExprDuplicate_HPP
#define ecflow_node_ExprDuplicate_HPP

///
/// \brief For large designs > 90% of triggers are identical.
/// We take advantage of this by using a map to store a *CLONED* ast
/// This saves a huge amount on re-parsing using spirit classic.
///
/// This cloned  AST is maintained in a static map, hence we need to
/// manage the lifetime, to avoid valgrind complaining.
///

#include <memory> // for unique_ptr
#include <string>
class AstTop;

// reclaim memory allocated in map, Avoid valgrind errors
class ExprDuplicate {
private:
public:
    ExprDuplicate() = default;

    // Disable copy (and move) semantics
    ExprDuplicate(const ExprDuplicate&)                  = delete;
    const ExprDuplicate& operator=(const ExprDuplicate&) = delete;
    ExprDuplicate(ExprDuplicate&&)                       = delete;
    ExprDuplicate& operator=(ExprDuplicate&&)            = delete;

    ~ExprDuplicate();

    // for debug only
    static void dump(const std::string& msg);

    // Find the expr in the map, if found returns a CLONED ast, else NULL
    static std::unique_ptr<AstTop> find(const std::string& expr);

    // Add the expr to the map, the ast is cloned.
    static void add(const std::string& expr, AstTop*);
};

#endif /* ecflow_node_ExprDuplicate_HPP */
