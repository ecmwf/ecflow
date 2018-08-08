#ifndef EXPRASTVISITOR_HPP_
#define EXPRASTVISITOR_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #7 $ 
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
	~AstResolveVisitor() override;

	const std::string& errorMsg() const { return errorMsg_;}

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
   void visitParentVariable(AstParentVariable*) override;
   void visitFlag(AstFlag*) override;

private:
	const Node* triggerNode_;
 	std::string errorMsg_;
};

class AstCollateNodesVisitor : public ExprAstVisitor {
public:
   explicit AstCollateNodesVisitor( std::set<Node*>& );
	~AstCollateNodesVisitor() override;

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
   void visitParentVariable(AstParentVariable*) override;
   void visitFlag(AstFlag*) override;

private:
 	std::set<Node*>& theSet_;
};
}
#endif /* EXPRASTVISITOR_HPP_ */
