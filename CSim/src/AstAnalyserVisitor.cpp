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

#include "AstAnalyserVisitor.hpp"
#include "ExprAst.hpp"
#include "Defs.hpp"

using namespace std;

namespace ecf {

AstAnalyserVisitor::AstAnalyserVisitor()  {}
AstAnalyserVisitor::~AstAnalyserVisitor() {}

void AstAnalyserVisitor::visitTop(AstTop*) {}
void AstAnalyserVisitor::visitRoot(AstRoot*) {}
void AstAnalyserVisitor::visitAnd(AstAnd*) {}
void AstAnalyserVisitor::visitNot(AstNot*) {}
void AstAnalyserVisitor::visitPlus(AstPlus*) {}
void AstAnalyserVisitor::visitMinus(AstMinus*) {}
void AstAnalyserVisitor::visitDivide(AstDivide*) {}
void AstAnalyserVisitor::visitMultiply(AstMultiply*) {}
void AstAnalyserVisitor::visitModulo(AstModulo*) {}
void AstAnalyserVisitor::visitOr(AstOr*) {}
void AstAnalyserVisitor::visitEqual(AstEqual*) {}
void AstAnalyserVisitor::visitNotEqual(AstNotEqual*) {}
void AstAnalyserVisitor::visitLessEqual(AstLessEqual*) {}
void AstAnalyserVisitor::visitGreaterEqual(AstGreaterEqual*) {}
void AstAnalyserVisitor::visitGreaterThan(AstGreaterThan*) {}
void AstAnalyserVisitor::visitLessThan(AstLessThan*) {}
void AstAnalyserVisitor::visitLeaf(AstLeaf*) {}
void AstAnalyserVisitor::visitInteger(AstInteger*) {}
void AstAnalyserVisitor::visitFunction(AstFunction*){};
void AstAnalyserVisitor::visitNodeState(AstNodeState*) {}
void AstAnalyserVisitor::visitEventState(AstEventState*) {}

void AstAnalyserVisitor::visitNode(AstNode* astNode)
{
	Node* refNode = astNode->referencedNode();
 	if ( refNode )  dependentNodes_.insert( refNode);
 	else            dependentNodePaths_.insert(astNode->nodePath());
}

void AstAnalyserVisitor::visitVariable(AstVariable* astVar){}

void AstAnalyserVisitor::visitFlag(AstFlag* astVar)
{
}

}
