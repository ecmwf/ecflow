/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/ExprDuplicate.hpp"

#include <iostream>
#include <unordered_map>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/node/ExprAst.hpp"

using expression_cache_t = std::unordered_map<std::string, std::unique_ptr<AstTop>>;

static expression_cache_t duplicate_expr;

ExprDuplicate::~ExprDuplicate() {
    duplicate_expr.clear();
}

void ExprDuplicate::dump(const std::string& msg) {
    std::cout << "ExprDuplicate::dump server(" << Ecf::server() << ") " << msg << "\n";
    for (const auto& i : duplicate_expr) {
        std::cout << "   " << i.first << " :" << i.second.get() << "\n";
    }
}

std::unique_ptr<AstTop> ExprDuplicate::find(const std::string& expr) {
    if (auto it = duplicate_expr.find(expr); it != duplicate_expr.end()) {
        return std::unique_ptr<AstTop>((*it).second->clone());
    }
    return std::unique_ptr<AstTop>();
}

void ExprDuplicate::add(const std::string& expr, AstTop* ast) {
    assert(!expr.empty() && ast);
    auto clone = std::unique_ptr<AstTop>(ast->clone());
    duplicate_expr.emplace(expr, std::move(clone));
}
