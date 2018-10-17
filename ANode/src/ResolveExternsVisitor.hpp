#ifndef RESOLVE_EXTERNS_VISITOR_HPP_
#define RESOLVE_EXTERNS_VISITOR_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "NodeTreeVisitor.hpp"
#include "ExprAstVisitor.hpp"
class Node;
class Ast;

namespace ecf {

class ResolveExternsVisitor : public NodeTreeVisitor {
public:
   explicit ResolveExternsVisitor(Defs*);

	bool traverseObjectStructureViaVisitors() const override { return true;}
	void visitDefs(Defs*) override;
	void visitSuite(Suite*) override;
	void visitFamily(Family*) override;
	void visitNodeContainer(NodeContainer*) override;
	void visitTask(Task*) override;

private:
	void setup(Node*);
	void doSetup(  Node* node, Ast* ast) ;

  	Defs* defs_;
};

class AstResolveExternVisitor : public ExprAstVisitor {
public:
	AstResolveExternVisitor( Node*, Defs*);
	~AstResolveExternVisitor() override;

  	void visitTop(AstTop*) override{}
 	void visitRoot(AstRoot*) override{}
 	void visitAnd(AstAnd*) override{}
 	void visitNot(AstNot*) override{}
 	void visitPlus(AstPlus*) override{}
 	void visitMinus(AstMinus*) override{}
 	void visitDivide(AstDivide*) override{}
   void visitMultiply(AstMultiply*) override{}
   void visitModulo(AstModulo*) override{}
 	void visitOr(AstOr*) override{}
 	void visitEqual(AstEqual*) override{}
 	void visitNotEqual(AstNotEqual*) override{}
 	void visitLessEqual(AstLessEqual*) override{}
 	void visitGreaterEqual(AstGreaterEqual*) override{}
 	void visitGreaterThan(AstGreaterThan*) override{}
 	void visitLessThan(AstLessThan*) override{}
 	void visitLeaf(AstLeaf*) override{}
 	void visitInteger(AstInteger*) override{}
   void visitFunction(AstFunction*) override{}
 	void visitNodeState(AstNodeState*) override{}
 	void visitEventState(AstEventState*) override{}
 	void visitNode(AstNode*) override;
   void visitVariable(AstVariable*) override;
   void visitParentVariable(AstParentVariable*) override {}
   void visitFlag(AstFlag*) override;

private:
	void addExtern(const std::string& absNodePath, const std::string& var = "");
	Node* triggerNode_;
	Defs* defs_;
};

}
#endif
