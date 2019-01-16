#ifndef EXPRASTVISITOR_HPP_
#define EXPRASTVISITOR_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #7 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <set>
#include <string>
class Node;

class Ast;
class AstTop;
class AstRoot;
class AstAnd;
class AstNot;
class AstPlus;
class AstMinus;
class AstDivide;
class AstMultiply;
class AstModulo;
class AstOr;
class AstEqual;
class AstNotEqual;
class AstLessEqual;
class AstGreaterEqual;
class AstLessThan;
class AstGreaterThan;
class AstLeaf;
class AstInteger;
class AstFunction;
class AstNodeState;
class AstEventState;
class AstNode;
class AstVariable;
class AstParentVariable;
class AstFlag;

namespace ecf {

class ExprAstVisitor {
public:
	virtual ~ExprAstVisitor();

  	virtual void visitTop(AstTop*) = 0;
 	virtual void visitRoot(AstRoot*) = 0;
 	virtual void visitAnd(AstAnd*) = 0;
 	virtual void visitNot(AstNot*) = 0;
 	virtual void visitPlus(AstPlus*) = 0;
 	virtual void visitMinus(AstMinus*) = 0;
 	virtual void visitDivide(AstDivide*) = 0;
   virtual void visitMultiply(AstMultiply*) = 0;
   virtual void visitModulo(AstModulo*) = 0;
 	virtual void visitOr(AstOr*) = 0;
 	virtual void visitEqual(AstEqual*) = 0;
 	virtual void visitNotEqual(AstNotEqual*) = 0;
 	virtual void visitLessEqual(AstLessEqual*) = 0;
 	virtual void visitGreaterEqual(AstGreaterEqual*) = 0;
 	virtual void visitGreaterThan(AstGreaterThan*) = 0;
 	virtual void visitLessThan(AstLessThan*) = 0;
 	virtual void visitLeaf(AstLeaf*) = 0;
   virtual void visitInteger(AstInteger*) = 0;
   virtual void visitFunction(AstFunction*) = 0;
 	virtual void visitNodeState(AstNodeState*) = 0;
 	virtual void visitEventState(AstEventState*) = 0;
 	virtual void visitNode(AstNode*) = 0;
   virtual void visitVariable(AstVariable*) = 0;
   virtual void visitParentVariable(AstParentVariable*) = 0;
   virtual void visitFlag(AstFlag*) = 0;
};

class AstResolveVisitor : public ExprAstVisitor {
public:
   explicit AstResolveVisitor(const Node* );
	virtual ~AstResolveVisitor();

	const std::string& errorMsg() const { return errorMsg_;}

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
   virtual void visitParentVariable(AstParentVariable*);
   virtual void visitFlag(AstFlag*);

private:
	const Node* triggerNode_;
 	std::string errorMsg_;
};

class AstCollateNodesVisitor : public ExprAstVisitor {
public:
   explicit AstCollateNodesVisitor( std::set<Node*>& );
	virtual ~AstCollateNodesVisitor();

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
   virtual void visitParentVariable(AstParentVariable*);
   virtual void visitFlag(AstFlag*);

private:
 	std::set<Node*>& theSet_;
};
}
#endif /* EXPRASTVISITOR_HPP_ */
