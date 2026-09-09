/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_simulator_AstAnalyserVisitor_HPP
#define ecflow_simulator_AstAnalyserVisitor_HPP

#include <set>

#include "ecflow/node/ExprAstVisitor.hpp"

class Node;

namespace ecf {

class AstAnalyserVisitor : public ExprAstVisitor {
public:
    AstAnalyserVisitor();
    ~AstAnalyserVisitor() override;

    const std::set<Node*>& dependentNodes() const { return dependentNodes_; }
    const std::set<std::string>& dependentNodePaths() const { return dependentNodePaths_; }

    void visitTop(AstTop*) override;
    void visitRoot(AstRoot*) override;
    void visitAnd(AstAnd*) override;
    void visitNot(AstNot*) override;
    void visitPlus(AstPlus*) override;
    void visitMinus(AstMinus*) override;
    void visitDivide(AstDivide*) override;
    void visitMultiply(AstMultiply*) override;
    void visitModulo(AstModulo*) override;
    void visitOr(AstOr*) override;
    void visitEqual(AstEqual*) override;
    void visitNotEqual(AstNotEqual*) override;
    void visitLessEqual(AstLessEqual*) override;
    void visitGreaterEqual(AstGreaterEqual*) override;
    void visitGreaterThan(AstGreaterThan*) override;
    void visitLessThan(AstLessThan*) override;
    void visitLeaf(AstLeaf*) override;
    void visitInteger(AstInteger*) override;
    void visitInstant(AstInstant*) override;
    void visitFunction(AstFunction*) override;
    void visitNodeState(AstNodeState*) override;
    void visitEventState(AstEventState*) override;
    void visitNode(AstNode*) override;
    void visitVariable(AstVariable*) override;
    void visitParentVariable(AstParentVariable*) override;
    void visitFlag(AstFlag*) override;

private:
    std::set<Node*> dependentNodes_;
    std::set<std::string> dependentNodePaths_;
};

} // namespace ecf

#endif /* ecflow_simulator_AstAnalyserVisitor_HPP */
