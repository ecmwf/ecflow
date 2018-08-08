#ifndef ASTANALYSERVISITOR_HPP_
#define ASTANALYSERVISITOR_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include "ExprAstVisitor.hpp"
#include <iostream>
#include <sstream>
#include <set>
class Node;

namespace ecf {

class AstAnalyserVisitor : public ExprAstVisitor {
public:
	AstAnalyserVisitor();
	~AstAnalyserVisitor() override;

	const std::set<Node*>& dependentNodes() const { return dependentNodes_;}
	const std::set<std::string>& dependentNodePaths() const { return dependentNodePaths_;}

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
}
#endif
