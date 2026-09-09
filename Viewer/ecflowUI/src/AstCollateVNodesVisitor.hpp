/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_AstCollateVNodeVisitor_HPP
#define ecflow_viewer_AstCollateVNodeVisitor_HPP

#include <vector>

#include "ecflow/node/ExprAstVisitor.hpp"

class VItem;

class AstCollateVNodesVisitor : public ecf::ExprAstVisitor {
public:
    explicit AstCollateVNodesVisitor(std::vector<VItem*>&);
    ~AstCollateVNodesVisitor() override;

    void visitTop(AstTop*) override {}
    void visitRoot(AstRoot*) override {}
    void visitAnd(AstAnd*) override {}
    void visitNot(AstNot*) override {}
    void visitPlus(AstPlus*) override {}
    void visitMinus(AstMinus*) override {}
    void visitDivide(AstDivide*) override {}
    void visitMultiply(AstMultiply*) override {}
    void visitModulo(AstModulo*) override {}
    void visitOr(AstOr*) override {}
    void visitEqual(AstEqual*) override {}
    void visitNotEqual(AstNotEqual*) override {}
    void visitLessEqual(AstLessEqual*) override {}
    void visitGreaterEqual(AstGreaterEqual*) override {}
    void visitGreaterThan(AstGreaterThan*) override {}
    void visitLessThan(AstLessThan*) override {}
    void visitLeaf(AstLeaf*) override {}
    void visitInteger(AstInteger*) override {}
    void visitInstant(AstInstant*) override {}
    void visitFunction(AstFunction*) override {}
    void visitNodeState(AstNodeState*) override {}
    void visitEventState(AstEventState*) override;
    void visitNode(AstNode*) override;
    void visitVariable(AstVariable*) override;
    void visitParentVariable(AstParentVariable*) override;
    void visitFlag(AstFlag*) override;

private:
    std::vector<VItem*>& items_;
};

#endif /* ecflow_viewer_AstCollateVNodeVisitor_HPP */
