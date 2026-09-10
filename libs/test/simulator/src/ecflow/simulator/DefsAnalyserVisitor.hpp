/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_simulator_DefsAnalyserVisitor_HPP
#define ecflow_simulator_DefsAnalyserVisitor_HPP

#include <set>
#include <sstream>

#include "ecflow/node/NodeTreeVisitor.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"

class Node;

namespace ecf {

class DefsAnalyserVisitor final : public NodeTreeVisitor {
public:
    DefsAnalyserVisitor();
    std::string report() const { return buffer_; }

    bool traverseObjectStructureViaVisitors() const override { return true; }
    void visitDefs(Defs*) override;
    void visitSuite(Suite*) override;
    void visitFamily(Family*) override;
    void visitNodeContainer(NodeContainer*) override;
    void visitTask(Task*) override;

private:
    void analyse(Node* n, std::set<Node*>& dependentNodes, bool dependent = false);
    void analyseExpressions(Node* node, std::set<Node*>& dependentNodes, bool trigger, bool dependent);

    std::set<Node*> analysedNodes_; // The node we  analysed
    std::string buffer_;
    ecf::stringstreambuf ss_{buffer_};
    ecf::FormatContext ctx_ = ecf::FormatContext::make_for(PrintStyle::DEFS);
};

} // namespace ecf

#endif /* ecflow_simulator_DefsAnalyserVisitor_HPP */
