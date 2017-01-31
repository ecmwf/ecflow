#ifndef DEFSANALYSERVISITOR_HPP_
#define DEFSANALYSERVISITOR_HPP_

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

#include "NodeTreeVisitor.hpp"
#include <sstream>
#include <set>
#include <vector>
class Node;

namespace ecf {

class DefsAnalyserVisitor : public NodeTreeVisitor {
public:
	DefsAnalyserVisitor();
	std::string report() const { return ss_.str();}

	virtual bool traverseObjectStructureViaVisitors() const { return true;}
	virtual void visitDefs(Defs*);
	virtual void visitSuite(Suite*);
	virtual void visitFamily(Family*);
	virtual void visitNodeContainer(NodeContainer*);
	virtual void visitTask(Task*);

private:
	void analyse(Node* n,std::set<Node*>& dependentNodes, bool dependent =  false);
	void analyseExpressions(Node* node,std::set<Node*>& dependentNodes, bool trigger, bool dependent);

  	std::stringstream ss_;
  	std::set<Node*> analysedNodes_;  // The node we  analysed
};
}
#endif
