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

#include <assert.h>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

//AstCollateVNodesVisitor::AstCollateVNodesVisitor( std::set<VItem*>& s) : theSet_(s) {}

AstCollateVNodesVisitor::AstCollateVNodesVisitor(std::vector<VItemTmp_ptr>& s) : items_(s) {}

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
            //theSet_.insert(n);
            items_.push_back(VItemTmp::create(n));
        }
    }
}

void AstCollateVNodesVisitor::visitVariable(AstVariable* astVar)
{

#if 0
    if(Node* referencedNode = astVar->referencedNode())
    {
        if(VNode* n=static_cast<VNode*>(referencedNode->graphic_ptr()))
        {
            QStringList types;
            types << "event" << "meter" << "var" << "genvar";
            Q_FOREACH(QString tName,types)
            {                               
                QList<VAttribute*> lst;
                VAttributeType::getSearchData(tName.toStdString(),n,lst);
                Q_FOREACH(VAttribute *a,lst)
                {
                    bool hasIt=false;
                    for(std::set<VItem*>::iterator it = theSet_.begin();
                        it != theSet_.end(); ++it)
                    {
                        if(a->sameContents(*it))
                        {
                            hasIt=true;
                            return;
                        }
                    }

                    if(!hasIt && a->strName() == astVar->name())
                    {
                        theSet_.insert(a);
                        return;
                    }
                    else
                        delete a;
                }
            }
        }
    }
#endif

    if(Node* referencedNode = astVar->referencedNode())
    {
        if(VNode* n=static_cast<VNode*>(referencedNode->graphic_ptr()))
        {
            QStringList types;
            types << "event" << "meter" << "var" << "genvar";
            Q_FOREACH(QString tName,types)
            {
                QList<VItemTmp_ptr> lst;
                VAttributeType::items(tName.toStdString(),n,lst);
                Q_FOREACH(VItemTmp_ptr aItem,lst)
                {
                    VAttribute *a=aItem->attribute();
                    assert(a);
                    for(std::vector<VItemTmp_ptr>::iterator it = items_.begin();it != items_.end(); ++it)
                    {
                        if(a->sameContents((*it)->item()))
                        {
                             return;
                        }
                    }

                    if(a->strName() == astVar->name())
                    {
                        items_.push_back(aItem);
                        return;
                    }
                }
            }
        }
    }

}

