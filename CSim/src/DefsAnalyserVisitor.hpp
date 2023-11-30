/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_simulator_DefsAnalyserVisitor_HPP
#define ecflow_simulator_DefsAnalyserVisitor_HPP

#include <set>
#include <sstream>

#include "NodeTreeVisitor.hpp"

class Node;

namespace ecf {

class DefsAnalyserVisitor final : public NodeTreeVisitor {
public:
    DefsAnalyserVisitor();
    std::string report() const { return ss_.str(); }

    bool traverseObjectStructureViaVisitors() const override { return true; }
    void visitDefs(Defs*) override;
    void visitSuite(Suite*) override;
    void visitFamily(Family*) override;
    void visitNodeContainer(NodeContainer*) override;
    void visitTask(Task*) override;

private:
    void analyse(Node* n, std::set<Node*>& dependentNodes, bool dependent = false);
    void analyseExpressions(Node* node, std::set<Node*>& dependentNodes, bool trigger, bool dependent);

    std::stringstream ss_;
    std::set<Node*> analysedNodes_; // The node we  analysed
};

} // namespace ecf

#endif /* ecflow_simulator_DefsAnalyserVisitor_HPP */
