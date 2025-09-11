/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/ExprAstVisitor.hpp"

#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/ExprAst.hpp"
#include "ecflow/node/Node.hpp"

namespace ecf {

//======================================================================================

ExprAstVisitor::~ExprAstVisitor() = default;

//======================================================================================
AstResolveVisitor::AstResolveVisitor(const Node* node) : triggerNode_(node) {
}
AstResolveVisitor::~AstResolveVisitor() = default;

void AstResolveVisitor::visitNode(AstNode* astNode) {
    // std::cout << "AstResolveVisitor::visitNode errorMsg = " << errorMsg_ << "\n";
    if (errorMsg_.empty()) {

        astNode->setParentNode(const_cast<Node*>(triggerNode_));
        Node* node = astNode->referencedNode(errorMsg_);
        if (!node) {
            // A node can be NULL when its a extern path. In this case errorMsg should be empty
            return;
        }
        LOG_ASSERT(errorMsg_.empty(), ""); // found Node, make sure errorMsg is empty
    }
}

void AstResolveVisitor::visitVariable(AstVariable* astVar) {
    if (errorMsg_.empty()) {

        astVar->setParentNode(const_cast<Node*>(triggerNode_));

        /// Use VariableHelper to populate errorMsg_
        VariableHelper varHelper(astVar, errorMsg_);
    }
}

void AstResolveVisitor::visitParentVariable(AstParentVariable* astvar) {
    if (errorMsg_.empty()) {

        astvar->setParentNode(const_cast<Node*>(triggerNode_));

        if (!astvar->find_node_which_references_variable()) {

            // Check externs if possible
            Defs* defs = triggerNode_->defs();
            if (defs) {
                if (defs->find_extern(triggerNode_->absNodePath(), astvar->name())) {
                    return;
                }
            }

            std::stringstream ss;
            ss << " Could not find variable " << astvar->name() << " on node " << triggerNode_->debugNodePath()
               << " OR any of its parent nodes";
            errorMsg_ += ss.str();
        }
    }
}

void AstResolveVisitor::visitFlag(AstFlag* ast) {
    if (errorMsg_.empty()) {

        ast->setParentNode(const_cast<Node*>(triggerNode_));
        Node* node = ast->referencedNode(errorMsg_);
        if (!node) {
            // A node can be NULL when its a extern path. In this case errorMsg should be empty
            return;
        }
        LOG_ASSERT(errorMsg_.empty(), ""); // found Node, make sure errorMsg is empty
    }
}

//===========================================================================================================

AstCollateNodesVisitor::AstCollateNodesVisitor(std::set<Node*>& s) : theSet_(s) {
}
AstCollateNodesVisitor::~AstCollateNodesVisitor() = default;

void AstCollateNodesVisitor::visitNode(AstNode* astNode) {
    Node* referencedNode = astNode->referencedNode(); // could be expensive, hence don't call twice
    if (referencedNode) {
        theSet_.insert(referencedNode);
    }
}

void AstCollateNodesVisitor::visitVariable(AstVariable* astVar) {
    Node* referencedNode = astVar->referencedNode(); // could be expensive, hence don't call twice
    if (referencedNode) {
        theSet_.insert(referencedNode);
    }
}

void AstCollateNodesVisitor::visitParentVariable(AstParentVariable* astvar) {
    Node* referencedNode = astvar->referencedNode(); // could be expensive, hence don't call twice
    if (referencedNode) {
        theSet_.insert(referencedNode);
    }
}

void AstCollateNodesVisitor::visitFlag(AstFlag* ast) {
    Node* referencedNode = ast->referencedNode(); // could be expensive, hence don't call twice
    if (referencedNode) {
        theSet_.insert(referencedNode);
    }
}

} // namespace ecf
