//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef ASTCOLLATEVNODESVISITOR_HPP
#define ASTCOLLATEVNODESVISITOR_HPP

#include "ExprAstVisitor.hpp"

#include <vector>

class VItem;

class AstCollateVNodesVisitor : public ecf::ExprAstVisitor
{
public:
  AstCollateVNodesVisitor(std::vector<VItem*>& );
  virtual ~AstCollateVNodesVisitor();

  virtual void visitTop(AstTop*){}
  virtual void visitRoot(AstRoot*){}
  virtual void visitAnd(AstAnd*){}
  virtual void visitNot(AstNot*){}
  virtual void visitPlus(AstPlus*){}
  virtual void visitMinus(AstMinus*){}
  virtual void visitDivide(AstDivide*){}
  virtual void visitMultiply(AstMultiply*){}
  virtual void visitModulo(AstModulo*){}
  virtual void visitOr(AstOr*){}
  virtual void visitEqual(AstEqual*){}
  virtual void visitNotEqual(AstNotEqual*){}
  virtual void visitLessEqual(AstLessEqual*){}
  virtual void visitGreaterEqual(AstGreaterEqual*){}
  virtual void visitGreaterThan(AstGreaterThan*){}
  virtual void visitLessThan(AstLessThan*){}
  virtual void visitLeaf(AstLeaf*){}
  virtual void visitInteger(AstInteger*){}
  virtual void visitFunction(AstFunction*){}
  virtual void visitNodeState(AstNodeState*){}
  virtual void visitEventState(AstEventState*);
  virtual void visitNode(AstNode*);
  virtual void visitVariable(AstVariable*);
  virtual void visitParentVariable(AstParentVariable*);
  virtual void visitFlag(AstFlag*);

private:
  std::vector<VItem*>& items_;
};

#endif // ASTCOLLATEVNODESVISITOR_HPP

