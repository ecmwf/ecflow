#ifndef RESOLVE_EXTERNS_VISITOR_HPP_
#define RESOLVE_EXTERNS_VISITOR_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2016 ECMWF. 
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
#include <string>
#include <set>
class Node;
class Ast;

namespace ecf {

class ResolveExternsVisitor : public NodeTreeVisitor {
public:
	ResolveExternsVisitor(Defs*);

	virtual bool traverseObjectStructureViaVisitors() const { return true;}
	virtual void visitDefs(Defs*);
	virtual void visitSuite(Suite*);
	virtual void visitFamily(Family*);
	virtual void visitNodeContainer(NodeContainer*);
	virtual void visitTask(Task*);

private:
	void setup(Node*);
	void doSetup(  Node* node, Ast* ast) ;

  	Defs* defs_;
};

class AstResolveExternVisitor : public ExprAstVisitor {
public:
	AstResolveExternVisitor( Node*, Defs*);
	virtual ~AstResolveExternVisitor();

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
 	virtual void visitEventState(AstEventState*){}
 	virtual void visitNode(AstNode*);
  	virtual void visitVariable(AstVariable*);

private:
	void addExtern(const std::string& absNodePath, const std::string& var = "");
	Node* triggerNode_;
	Defs* defs_;
};

}
#endif
