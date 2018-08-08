#ifndef FLATANALYSERVISITOR_HPP_
#define FLATANALYSERVISITOR_HPP_

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
class Node;

namespace ecf {

class FlatAnalyserVisitor : public NodeTreeVisitor {
public:
	FlatAnalyserVisitor();
	std::string report() const { return ss_.str();}

	bool traverseObjectStructureViaVisitors() const override { return true;}
	void visitDefs(Defs*) override;
	void visitSuite(Suite*) override;
	void visitFamily(Family*) override;
	void visitNodeContainer(NodeContainer*) override;
	void visitTask(Task*) override;

private:
	bool analyse(Node* n);
  	std::stringstream ss_;
};

}
#endif
