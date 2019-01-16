 //============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include "FlatAnalyserVisitor.hpp"
#include "AstAnalyserVisitor.hpp"
#include "ExprAst.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Indentor.hpp"
#include "Str.hpp"

using namespace std;

namespace ecf {

///////////////////////////////////////////////////////////////////////////////
FlatAnalyserVisitor::FlatAnalyserVisitor() {}

void FlatAnalyserVisitor::visitDefs( Defs* d) {
	BOOST_FOREACH(suite_ptr s, d->suiteVec()) { s->acceptVisitTraversor(*this); }
}

void FlatAnalyserVisitor::visitSuite( Suite* s)   { visitNodeContainer(s);}
void FlatAnalyserVisitor::visitFamily( Family* f) { visitNodeContainer(f);}

void FlatAnalyserVisitor::visitNodeContainer(NodeContainer* nc)
{
	if (nc->state() == NState::COMPLETE )  return;

 	Indentor in;
 	bool traverseChildren = analyse(nc);

 	// Dont bother traversing children if parent is holding on trigger/complete expression
 	if (traverseChildren) {
 		BOOST_FOREACH(node_ptr t, nc->nodeVec())       { t->acceptVisitTraversor(*this);}
 	}
}

void FlatAnalyserVisitor::visitTask( Task* t)
{
 	Indentor in;
 	analyse(t);
}

bool FlatAnalyserVisitor::analyse(Node* node)
{
	bool traverseChildren = true;

	Indentor::indent(ss_) << node->debugType() << Str::COLON() << node->name() << " state(" << NState::toString(node->state()) << ")";
	if (node->state() != NState::COMPLETE ) {

		if (node->repeat().isInfinite()) {
			ss_ << " may **NEVER** complete due to " << node->repeat().toString();
		}
		ss_ << "\n";

		if (node->state() == NState::QUEUED) {
			std::vector<std::string> theReasonWhy;
			node->why(theReasonWhy);
 			for(size_t i = 0; i < theReasonWhy.size(); ++i) {
 				Indentor::indent(ss_) << "Reason: " << theReasonWhy[i] << "\n";
 			}
 		}

	   /// Note a complete expression that does not evaluate, does *NOT* hold the node
	   /// It merly sets node to complete.
		if ( node->completeAst() && !node->evaluateComplete()) {
			Indentor::indent(ss_) << "holding on complete expression '" << node->completeExpression() << "'\n";

			AstAnalyserVisitor astVisitor;
	 		node->completeAst()->accept(astVisitor);
	 		BOOST_FOREACH(const string& nodePath, astVisitor.dependentNodePaths()) {
	 			Indentor in; Indentor::indent(ss_) << "'" << nodePath << "' is not defined in the expression\n";
	 		}
	 		ss_ << *node->completeAst();

			traverseChildren = false;
		}

		if ( node->triggerAst() && !node->evaluateTrigger() ) {
			Indentor::indent(ss_) << "holding on trigger expression '" << node->triggerExpression() << "'\n";

			AstAnalyserVisitor astVisitor;
			node->triggerAst()->accept(astVisitor);
	 		BOOST_FOREACH(const string& nodePath, astVisitor.dependentNodePaths()) {
	 			Indentor in; Indentor::indent(ss_) << "'" << nodePath << "' is not defined in the expression\n";
	 		}
			ss_ << *node->triggerAst();

			traverseChildren = false;
		}
	}
  	ss_ << "\n";
  	return traverseChildren;
}

}
