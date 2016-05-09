#ifndef ASTANALYSERVISITOR_HPP_
#define ASTANALYSERVISITOR_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$ 
//
// Copyright 2009-2016 ECMWF. 
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
	virtual ~AstAnalyserVisitor();

	const std::set<Node*>& dependentNodes() const { return dependentNodes_;}
	const std::set<std::string>& dependentNodePaths() const { return dependentNodePaths_;}

  	virtual void visitTop(AstTop*);
 	virtual void visitRoot(AstRoot*);
 	virtual void visitAnd(AstAnd*);
 	virtual void visitNot(AstNot*);
 	virtual void visitPlus(AstPlus*);
 	virtual void visitMinus(AstMinus*);
 	virtual void visitDivide(AstDivide*);
   virtual void visitMultiply(AstMultiply*);
   virtual void visitModulo(AstModulo*);
 	virtual void visitOr(AstOr*);
 	virtual void visitEqual(AstEqual*);
 	virtual void visitNotEqual(AstNotEqual*);
 	virtual void visitLessEqual(AstLessEqual*);
 	virtual void visitGreaterEqual(AstGreaterEqual*);
 	virtual void visitGreaterThan(AstGreaterThan*);
 	virtual void visitLessThan(AstLessThan*);
 	virtual void visitLeaf(AstLeaf*);
 	virtual void visitInteger(AstInteger*);
 	virtual void visitString(AstString*);
 	virtual void visitNodeState(AstNodeState*);
 	virtual void visitEventState(AstEventState*);
 	virtual void visitNode(AstNode*);
  	virtual void visitVariable(AstVariable*);

private:
	std::set<Node*> dependentNodes_;
	std::set<std::string> dependentNodePaths_;
};
}
#endif
