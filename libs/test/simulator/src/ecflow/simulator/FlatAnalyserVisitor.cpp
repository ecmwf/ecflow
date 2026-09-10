/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/simulator/FlatAnalyserVisitor.hpp"

#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/ExprAst.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/simulator/AstAnalyserVisitor.hpp"

namespace ecf {

///////////////////////////////////////////////////////////////////////////////
FlatAnalyserVisitor::FlatAnalyserVisitor() = default;

void FlatAnalyserVisitor::visitDefs(Defs* d) {
    for (auto suite : d->suites()) {
        suite->acceptVisitTraversor(*this);
    }
}

void FlatAnalyserVisitor::visitSuite(Suite* s) {
    visitNodeContainer(s);
}
void FlatAnalyserVisitor::visitFamily(Family* f) {
    visitNodeContainer(f);
}

void FlatAnalyserVisitor::visitNodeContainer(NodeContainer* nc) {
    if (nc->state() == NState::COMPLETE) {
        return;
    }

    Indent l1(ctx_);

    bool traverseChildren = analyse(nc);

    // Do not traverse children if the parent is holding on trigger/complete expression
    if (traverseChildren) {
        for (auto node : nc->children()) {
            node->acceptVisitTraversor(*this);
        }
    }
}

void FlatAnalyserVisitor::visitTask(Task* t) {
    analyse(t);
}

bool FlatAnalyserVisitor::analyse(Node* node) {
    bool traverseChildren = true;

    Indent l1(ctx_);

    ss_ << l1;
    ss_ << node->debugType();
    ss_ << ecf::string_constants::colon;
    ss_ << node->name();
    ss_ << " state(";
    ss_ << NState::toString(node->state());
    ss_ << ")";
    if (node->state() != NState::COMPLETE) {

        if (node->repeat().isInfinite()) {
            ss_ << " may **NEVER** complete due to ";
            ss_ << node->repeat().toString();
        }
        ss_ << "\n";

        if (node->state() == NState::QUEUED) {
            std::vector<std::string> theReasonWhy;
            node->why(theReasonWhy);
            for (const auto& i : theReasonWhy) {
                ss_ << l1;
                ss_ << "Reason: ";
                ss_ << i;
                ss_ << "\n";
            }
        }

        /// Note a complete expression that does not evaluate, does *NOT* hold the node
        /// It merely sets node to complete.
        if (node->completeAst() && !node->evaluateComplete()) {
            ss_ << l1;
            ss_ << "holding on complete expression '";
            ss_ << node->completeExpression();
            ss_ << "'\n";

            AstAnalyserVisitor astVisitor;
            node->completeAst()->accept(astVisitor);
            for (const std::string& nodePath : astVisitor.dependentNodePaths()) {
                Indent l2(ctx_);
                ss_ << l2;
                ss_ << "'";
                ss_ << nodePath;
                ss_ << "' is not defined in the expression\n";
            }
            ss_ << ecf::as_string(*node->completeAst(), PrintStyle::DEFS);

            traverseChildren = false;
        }

        if (node->triggerAst() && !node->evaluateTrigger()) {
            ss_ << l1;
            ss_ << "holding on trigger expression '";
            ss_ << node->triggerExpression();
            ss_ << "'\n";

            AstAnalyserVisitor astVisitor;
            node->triggerAst()->accept(astVisitor);
            for (const std::string& nodePath : astVisitor.dependentNodePaths()) {
                Indent l2(ctx_);
                ss_ << l2;
                ss_ << "'";
                ss_ << nodePath;
                ss_ << "' is not defined in the expression\n";
            }
            ss_ << ecf::as_string(*node->triggerAst(), PrintStyle::DEFS);

            traverseChildren = false;
        }
    }
    ss_ << "\n";
    return traverseChildren;
}

} // namespace ecf
