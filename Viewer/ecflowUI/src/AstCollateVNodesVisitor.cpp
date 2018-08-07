//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "AstCollateVNodesVisitor.hpp"

#include <cassert>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

static std::vector<VAttributeType*> attrTypes;

AstCollateVNodesVisitor::AstCollateVNodesVisitor(std::vector<VItem*>& s) : items_(s)
{
    if(attrTypes.empty())
    {
        QStringList types;
        types << "event" << "meter" << "var" << "genvar";
        Q_FOREACH(QString name,types)
        {
            VAttributeType *t=VAttributeType::find(name.toStdString());
            Q_ASSERT(t);
            attrTypes.push_back(t);

        }
   }
}

AstCollateVNodesVisitor::~AstCollateVNodesVisitor() {}

void AstCollateVNodesVisitor::visitEventState(AstEventState* astNode)
{
}

void AstCollateVNodesVisitor::visitNode(AstNode* astNode)
{
    if(Node* referencedNode = astNode->referencedNode())
    {
        if(auto* n=static_cast<VNode*>(referencedNode->graphic_ptr()))
        {           
            items_.push_back(n);
        }
    }
}

void AstCollateVNodesVisitor::visitVariable(AstVariable* astVar)
{
    if(Node* referencedNode = astVar->referencedNode())
    {
        if(auto* n=static_cast<VNode*>(referencedNode->graphic_ptr()))
        {
            std::size_t nType=attrTypes.size();
            std::size_t nItem=items_.size();
            for(std::size_t i=0; i < nType; i++)
            {
                if(VAttribute *a=n->findAttribute(attrTypes[i],astVar->name()))
                {
                    for(std::size_t k=0; k < nItem; k++)
                    {
                        if(a == items_[k])
                            return;
                    }

                    items_.push_back(a);
                        return;
                }
            }
        }
    }
}

void AstCollateVNodesVisitor::visitParentVariable(AstParentVariable* astVar)
{
    if(Node* referencedNode = astVar->referencedNode())
    {
        if(auto* n=static_cast<VNode*>(referencedNode->graphic_ptr()))
        {
            std::size_t nType=attrTypes.size();
            std::size_t nItem=items_.size();
            for(std::size_t i=0; i < nType; i++)
            {
                if(VAttribute *a=n->findAttribute(attrTypes[i],astVar->name()))
                {
                    for(std::size_t k=0; k < nItem; k++)
                    {
                        if(a == items_[k])
                            return;
                    }

                    items_.push_back(a);
                        return;
                }
            }
        }
    }
}

void AstCollateVNodesVisitor::visitFlag(AstFlag* astVar)
{
   // ???
}

