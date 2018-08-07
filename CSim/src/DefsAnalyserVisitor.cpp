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

#include "DefsAnalyserVisitor.hpp"
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
DefsAnalyserVisitor::DefsAnalyserVisitor() = default;

void DefsAnalyserVisitor::visitDefs( Defs* d) {
	BOOST_FOREACH(suite_ptr s, d->suiteVec()) { s->acceptVisitTraversor(*this); }
}

void DefsAnalyserVisitor::visitSuite( Suite* s)   { visitNodeContainer(s);}
void DefsAnalyserVisitor::visitFamily( Family* f) { visitNodeContainer(f);}

void DefsAnalyserVisitor::visitNodeContainer(NodeContainer* nc)
{
 	std::set<Node*> dependentNodes;
	analyse(nc,dependentNodes);

	BOOST_FOREACH(node_ptr t, nc->nodeVec())       { t->acceptVisitTraversor(*this);}
}

void DefsAnalyserVisitor::visitTask( Task* t)
{
 	std::set<Node*> dependentNodes;
	analyse(t,dependentNodes);
}

void DefsAnalyserVisitor::analyse(Node* node,std::set<Node*>& dependentNodes, bool dependent)
{
	// ***************************************************************
	// Do a depth first search to find the root cause of the blockage
	// ***************************************************************
//#ifdef DEBUG
//	if (dependent)  {
//		ss_ << "DefsAnalyserVisitor::analyse " << node->debugType() << Str::COLON() << node->absNodePath();
//		ss_ << " state(" << NState::toString(node->state()) << ")\n";
//	}
//#endif
	if (analysedNodes_.find(node) != analysedNodes_.end()) return;
	analysedNodes_.insert( node );
	if (node->state() == NState::COMPLETE ) return;

	if (node->state() == NState::QUEUED) {
		std::vector<std::string> theReasonWhy;
		node->why(theReasonWhy);
		for(const auto & i : theReasonWhy) {
			Indentor::indent(ss_) << "Reason: " << i << "\n";
		}
	}


 	/// Note a complete expression that does not evaluate, does *NOT* hold the node
 	/// It merly sets node to complete.
 	if ( node->completeAst() && !node->evaluateComplete()) {
 		// Follow nodes referenced in the complete expressions
 		analyseExpressions(node,dependentNodes,false,dependent);

 		// follow child nodes
  		auto* nc =  dynamic_cast<NodeContainer*>(node);
 		if (nc) {
 			  BOOST_FOREACH(node_ptr t, nc->nodeVec())       { t->acceptVisitTraversor(*this);}
 		}
	}


  	if ( node->triggerAst() && !node->evaluateTrigger()  ) {
 		// Follow nodes referenced in the trigger expressions
 		analyseExpressions(node,dependentNodes,true,dependent);

		// follow child nodes
 		auto* nc =  dynamic_cast<NodeContainer*>(node);
 		if (nc) {
 			BOOST_FOREACH(node_ptr t, nc->nodeVec())       { t->acceptVisitTraversor(*this);}
  		}
  	}
}

void DefsAnalyserVisitor::analyseExpressions(Node* node,std::set<Node*>& dependentNodes, bool trigger, bool dependent)
{
 	Indentor in; Indentor::indent(ss_);
	if ( dependent ) ss_ << "DEPENDENT ";
	if (trigger) {
		ss_ << node->debugNodePath() << " holding on trigger expression '" << node->triggerExpression() << "'\n";
	}
	else {
		ss_ << node->debugNodePath() << " holding on complete expression '" << node->completeExpression() << "'\n";
	}


	AstAnalyserVisitor astVisitor;
	if ( trigger ) {
		node->triggerAst()->accept(astVisitor);
		ss_ << *node->triggerAst();
	}
 	else  {
 		node->completeAst()->accept(astVisitor);
 		ss_ << *node->completeAst();
 	}

	// Warn about NULL node references in the trigger expressions
	BOOST_FOREACH(const string& nodePath, astVisitor.dependentNodePaths()) {
		Indentor in; Indentor::indent(ss_) << "'" << nodePath << "' is not defined in the expression\n";
	}

	// **** NOTE: Currently for COMPLETE expression will only follow trigger expressions
	BOOST_FOREACH(Node* triggerNode, astVisitor.dependentNodes()) {


		Indentor in; Indentor::indent(ss_) << "EXPRESSION NODE " << triggerNode->debugNodePath();
		ss_ << " state(" << NState::toString(triggerNode->state()) << ")";
		if (triggerNode->triggerAst()) ss_ << " trigger(evaluation = " << triggerNode->evaluateTrigger() << "))";
		if (analysedNodes_.find(triggerNode) != analysedNodes_.end()) ss_ << " analysed ";
		if (dependentNodes.find(triggerNode) != dependentNodes.end()) ss_ << " ** ";
		ss_ << "\n";

		if ( dependentNodes.find(triggerNode) != dependentNodes.end()) {
 			// possible deadlock make sure
			if (triggerNode->triggerAst()) {
 				AstAnalyserVisitor visitor;
				triggerNode->triggerAst()->accept(visitor);

//				cerr << "Node = " << node->absNodePath() << "\n";
//				cerr << "triggerNode = " << triggerNode->absNodePath() << "\n";
//				BOOST_FOREACH(Node* n,visitor.dependentNodes() ) {
//					cerr << "triggerNode Node dependents = " << n->absNodePath() << "\n";
//				}

				if (visitor.dependentNodes().find(node) !=  visitor.dependentNodes().end()) {
					Indentor in;  Indentor::indent(ss_) << "Deadlock detected between:\n";
					Indentor in2; Indentor::indent(ss_) << node->debugNodePath() << "\n";
	 			                  Indentor::indent(ss_) << triggerNode->debugNodePath() << "\n";
				}
			}
	 		continue;
		}
		dependentNodes.insert( triggerNode );
		analyse(triggerNode,dependentNodes,true);
	}
}

}
