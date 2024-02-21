/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/ResolveExternsVisitor.hpp"

#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/ExprAst.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"

using namespace std;

namespace ecf {

///////////////////////////////////////////////////////////////////////////////
// ResolveExternsVisitor: Will traverse node tree hierarchy
ResolveExternsVisitor::ResolveExternsVisitor(Defs* defs) : defs_(defs) {
}

void ResolveExternsVisitor::visitDefs(Defs* d) {
    for (suite_ptr s : d->suiteVec()) {
        s->acceptVisitTraversor(*this);
    }
}

void ResolveExternsVisitor::visitSuite(Suite* s) {
    visitNodeContainer(s);
}
void ResolveExternsVisitor::visitFamily(Family* f) {
    visitNodeContainer(f);
}

void ResolveExternsVisitor::visitNodeContainer(NodeContainer* nc) {

    setup(nc);

    for (node_ptr t : nc->nodeVec()) {
        t->acceptVisitTraversor(*this);
    }
}

void ResolveExternsVisitor::visitTask(Task* t) {
    setup(t);
}

void ResolveExternsVisitor::setup(Node* n) {
    // Defs passed in to avoid, traversing up the node tree
    n->auto_add_inlimit_externs(defs_);
    doSetup(n, n->completeAst());
    doSetup(n, n->triggerAst());
}

void ResolveExternsVisitor::doSetup(Node* node, Ast* ast) {
    if (ast) {
        // The complete expression have been parsed and we have created the abstract syntax tree
        AstResolveExternVisitor astVisitor(node, defs_);
        ast->accept(astVisitor);
    }
}

//======================================================================================
// AstResolveExternVisitor: Will traverse ASR tree hierarchy
AstResolveExternVisitor::AstResolveExternVisitor(Node* node, Defs* defs) : triggerNode_(node), defs_(defs) {
}

AstResolveExternVisitor::~AstResolveExternVisitor() = default;

void AstResolveExternVisitor::visitNode(AstNode* astNode) {
    // std::cout << " AstResolveExternVisitor::visitNode " << triggerNode_->debugNodePath() << "\n";

    astNode->setParentNode(triggerNode_);

    // See if can reference the path, on the AstNode, if we can't, it should be added as an extern
    std::string errorMsg;
    Node* node = astNode->referencedNode(errorMsg);
    if (!node) {
        /// Add this path to the extern's. Avoid adding duplicates
        addExtern(astNode->nodePath());
    }
}

void AstResolveExternVisitor::visitVariable(AstVariable* astVar) {
    // std::cout << " AstResolveExternVisitor::visitNode " << triggerNode_->debugNodePath() << "\n";

    astVar->setParentNode(triggerNode_);

    // See if can reference the path, on the AstVariable, if we can't, it should be added as an extern
    std::string errorMsg;
    Node* theReferencedNode = astVar->referencedNode(errorMsg);
    if (!theReferencedNode) {
        addExtern(astVar->nodePath(), astVar->name());
        return;
    }
    LOG_ASSERT(errorMsg.empty(), "");

    // Ok,we found the referenced, node, now see if we can reference the attribute name
    // Find in order, event, meter, user variable, repeat, generated variable
    if (theReferencedNode->findExprVariable(astVar->name())) {
        return;
    }

    // Can't find name, in event, meter, user variable, repeat, generated variable, add as extern
    addExtern(astVar->nodePath(), astVar->name());
}

void AstResolveExternVisitor::visitParentVariable(AstParentVariable* astvar) {
    // std::cout << " AstResolveExternVisitor::visitParentVariable  " << triggerNode_->debugNodePath() << "\n";

    astvar->setParentNode(const_cast<Node*>(triggerNode_));

    if (!astvar->find_node_which_references_variable()) {

        addExtern(triggerNode_->absNodePath(), astvar->name()); // absolute var
    }
}

void AstResolveExternVisitor::visitFlag(AstFlag* astVar) {
    // std::cout << " AstResolveExternVisitor::visitFlag " << triggerNode_->debugNodePath() << "\n";

    astVar->setParentNode(triggerNode_);

    // See if can reference the path, on the AstFlag, if we can't, it should be added as an extern
    std::string errorMsg;
    Node* theReferencedNode = astVar->referencedNode(errorMsg);
    if (!theReferencedNode) {
        addExtern(astVar->nodePath(), astVar->name()); // return flag:late
        return;
    }
}

void AstResolveExternVisitor::addExtern(const std::string& absNodePath, const std::string& var) {
    string ext = absNodePath;
    if (!var.empty()) {
        ext += Str::COLON();
        ext += var;
    }
    // cout << " AstResolveExternVisitor::addExtern " << ext << "\n";
    defs_->add_extern(ext); // stored in a set:
}
} // namespace ecf
