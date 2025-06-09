/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/simulator/DefsAnalyserVisitor.hpp"

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/ExprAst.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/simulator/AstAnalyserVisitor.hpp"

using namespace std;

namespace ecf {

///////////////////////////////////////////////////////////////////////////////
DefsAnalyserVisitor::DefsAnalyserVisitor() = default;

void DefsAnalyserVisitor::visitDefs(Defs* d) {
    for (suite_ptr s : d->suiteVec()) {
        s->acceptVisitTraversor(*this);
    }
}

void DefsAnalyserVisitor::visitSuite(Suite* s) {
    visitNodeContainer(s);
}
void DefsAnalyserVisitor::visitFamily(Family* f) {
    visitNodeContainer(f);
}

void DefsAnalyserVisitor::visitNodeContainer(NodeContainer* nc) {
    std::set<Node*> dependentNodes;
    analyse(nc, dependentNodes);

    for (node_ptr t : nc->nodeVec()) {
        t->acceptVisitTraversor(*this);
    }
}

void DefsAnalyserVisitor::visitTask(Task* t) {
    std::set<Node*> dependentNodes;
    analyse(t, dependentNodes);
}

void DefsAnalyserVisitor::analyse(Node* node, std::set<Node*>& dependentNodes, bool dependent) {
    // ***************************************************************
    // Do a depth first search to find the root cause of the blockage
    // ***************************************************************

    if (analysedNodes_.find(node) != analysedNodes_.end())
        return;
    analysedNodes_.insert(node);
    if (node->state() == NState::COMPLETE)
        return;

    if (node->state() == NState::QUEUED) {
        std::vector<std::string> theReasonWhy;
        node->why(theReasonWhy);
        for (const auto& i : theReasonWhy) {
            Indent l1(ctx_);
            ss_ << l1;
            ss_ << "Reason: ";
            ss_ << i;
            ss_ << "\n";
        }
    }

    /// Note a complete expression that does not evaluate, does *NOT* hold the node
    /// It merly sets node to complete.
    if (node->completeAst() && !node->evaluateComplete()) {
        // Follow nodes referenced in the complete expressions
        analyseExpressions(node, dependentNodes, false, dependent);

        // follow child nodes
        auto* nc = dynamic_cast<NodeContainer*>(node);
        if (nc) {
            for (node_ptr t : nc->nodeVec()) {
                t->acceptVisitTraversor(*this);
            }
        }
    }

    if (node->triggerAst() && !node->evaluateTrigger()) {
        // Follow nodes referenced in the trigger expressions
        analyseExpressions(node, dependentNodes, true, dependent);

        // follow child nodes
        auto* nc = dynamic_cast<NodeContainer*>(node);
        if (nc) {
            for (node_ptr t : nc->nodeVec()) {
                t->acceptVisitTraversor(*this);
            }
        }
    }
}

void DefsAnalyserVisitor::analyseExpressions(Node* node,
                                             std::set<Node*>& dependentNodes,
                                             bool trigger,
                                             bool dependent) {
    Indent l1(ctx_);

    ss_ << l1;
    if (dependent)
        ss_ << "DEPENDENT ";
    if (trigger) {
        ss_ << node->debugNodePath();
        ss_ << " holding on trigger expression '";
        ss_ << node->triggerExpression();
        ss_ << "'\n";
    }
    else {
        ss_ << node->debugNodePath();
        ss_ << " holding on complete expression '";
        ss_ << node->completeExpression();
        ss_ << "'\n";
    }

    AstAnalyserVisitor astVisitor;
    if (trigger) {
        node->triggerAst()->accept(astVisitor);
        ss_ << ecf::as_string(*node->triggerAst(), PrintStyle::DEFS);
    }
    else {
        node->completeAst()->accept(astVisitor);
        ss_ << ecf::as_string(*node->completeAst(), PrintStyle::DEFS);
    }

    // Warn about NULL node references in the trigger expressions
    for (const string& nodePath : astVisitor.dependentNodePaths()) {
        Indent l2(ctx_);

        ss_ << l2;
        ss_ << "'";
        ss_ << nodePath;
        ss_ << "' is not defined in the expression\n";
    }

    // **** NOTE: Currently for COMPLETE expression will only follow trigger expressions
    for (Node* triggerNode : astVisitor.dependentNodes()) {
        Indent l2(ctx_);

        ss_ << l2;
        ss_ << "EXPRESSION NODE ";
        ss_ << triggerNode->debugNodePath();
        ss_ << " state(";
        ss_ << NState::toString(triggerNode->state());
        ss_ << ")";
        if (triggerNode->triggerAst()) {
            ss_ << " trigger(evaluation = ";
            ss_ << triggerNode->evaluateTrigger();
            ss_ << "))";
        }
        if (analysedNodes_.find(triggerNode) != analysedNodes_.end()) {
            ss_ << " analysed ";
        }
        if (dependentNodes.find(triggerNode) != dependentNodes.end()) {
            ss_ << " ** ";
        }
        ss_ << "\n";

        if (dependentNodes.find(triggerNode) != dependentNodes.end()) {
            // possible deadlock make sure
            if (triggerNode->triggerAst()) {
                AstAnalyserVisitor visitor;
                triggerNode->triggerAst()->accept(visitor);

                if (visitor.dependentNodes().find(node) != visitor.dependentNodes().end()) {
                    Indent l3(ctx_);
                    ss_ << l3;
                    ss_ << "Deadlock detected between:\n";
                    Indent l4(ctx_);
                    ss_ << l4;
                    ss_ << node->debugNodePath();
                    ss_ << "\n";
                    ss_ << l4;
                    ss_ << triggerNode->debugNodePath();
                    ss_ << "\n";
                }
            }
            continue;
        }
        dependentNodes.insert(triggerNode);
        analyse(triggerNode, dependentNodes, true);
    }
}

} // namespace ecf
