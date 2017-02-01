/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $ 
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

#include "ExprAstVisitor.hpp"
#include "ExprAst.hpp"
#include "Node.hpp"
#include "Log.hpp"

namespace ecf {

//======================================================================================

ExprAstVisitor::~ExprAstVisitor(){}

//======================================================================================
AstResolveVisitor::AstResolveVisitor(const Node* node) : triggerNode_(node) {}
AstResolveVisitor::~AstResolveVisitor() {}

void AstResolveVisitor::visitNode(AstNode* astNode)
{
	//std::cout << "AstResolveVisitor::visitNode errorMsg = " << errorMsg_ << "\n";
	if (  errorMsg_.empty()) {

		astNode->setParentNode(const_cast<Node*>(triggerNode_));
 		Node* node = astNode->referencedNode( errorMsg_ );
		if ( !node ) {
			// A node can be NULL when its a extern path. In this case errorMsg should be empty
 			return ;
		}
		LOG_ASSERT(errorMsg_.empty(),""); // found Node, make sure errorMsg is empty
  	}
}

void AstResolveVisitor::visitVariable(AstVariable* astVar)
{
	if ( errorMsg_.empty() ) {

		astVar->setParentNode(const_cast<Node*>(triggerNode_));

		/// Use VariableHelper to populate errorMsg_
		VariableHelper varHelper(astVar,errorMsg_);
 	}
}

//===========================================================================================================

AstCollateNodesVisitor::AstCollateNodesVisitor( std::set<Node*>& s) : theSet_(s) {}
AstCollateNodesVisitor::~AstCollateNodesVisitor() {}

void AstCollateNodesVisitor::visitNode(AstNode* astNode)
{
	Node* referencedNode = astNode->referencedNode(); // could be expensive, hence don't call twice
	if ( referencedNode ) theSet_.insert(referencedNode);
}

void AstCollateNodesVisitor::visitVariable(AstVariable* astVar)
{
	Node* referencedNode = astVar->referencedNode(); // could be expensive, hence don't call twice
	if ( referencedNode ) theSet_.insert(referencedNode);
}

}
