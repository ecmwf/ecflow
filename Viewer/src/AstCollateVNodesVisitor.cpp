//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "AstCollateVNodesVisitor.hpp"

#include <assert.h>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

AstCollateVNodesVisitor::AstCollateVNodesVisitor( std::set<VItem*>& s) : theSet_(s) {}

AstCollateVNodesVisitor::~AstCollateVNodesVisitor() {}

void AstCollateVNodesVisitor::visitEventState(AstEventState* astNode)
{
}

void AstCollateVNodesVisitor::visitNode(AstNode* astNode)
{
    if(Node* referencedNode = astNode->referencedNode())
    {
        if(VNode* n=static_cast<VNode*>(referencedNode->graphic_ptr()))
        {
            theSet_.insert(n);
        }
    }
}

void AstCollateVNodesVisitor::visitVariable(AstVariable* astVar)
{
    if(Node* referencedNode = astVar->referencedNode())
    {
        if(VNode* n=static_cast<VNode*>(referencedNode->graphic_ptr()))
        {
            QStringList types;
            types << "event" << "meter" << "var" << "genvar";
            Q_FOREACH(QString tName,types)
            {
                VAttributeType *t=VAttributeType::find(tName.toStdString());
                assert(t);

                QList<VAttribute*> lst;
                t->getSearchData(n,lst);
                Q_FOREACH(VAttribute *a,lst)
                {
                    if(a->strName() == astVar->name())
                        theSet_.insert(a);
                    else
                        delete a;
                }
            }
        }
   }
}

