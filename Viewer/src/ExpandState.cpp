//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ExpandState.hpp"
#include "ExpandStateNode.hpp"
#include "AbstractNodeView.hpp"
#include "TreeNodeModel.hpp"
#include "UiLog.hpp"
#include "VNode.hpp"
#include "VTree.hpp"

#define _UI_EXPANDSTATE_DEBUG

ExpandState::ExpandState(AbstractNodeView* view,TreeNodeModel* model) :
    view_(view), model_(model), root_(0)
{
}

ExpandState::~ExpandState()
{
    clear();
}

void ExpandState::clear()
{
    if(root_)
        delete root_;

    root_=0;
}

bool ExpandState::rootSameAs(const std::string& name) const
{
    return (root_ && root_->name_ == name);
}

ExpandStateNode* ExpandState::setRoot(VNode* r,bool expanded)
{
    if(root_)
        clear();

    root_=new ExpandStateNode(r,expanded);
    return root_;
}


//Save the expand state for the given node (it can be a server as well)
void ExpandState::save(const VTreeNode *root)
{
    assert(root);

    VNode* rootVn=root->vnode();

    QModelIndex rootIdx=model_->nodeToIndex(root);
    bool expanded=view_->isExpanded(rootIdx);
    if(root_ == 0 || root_->name_ != root->vnode()->strName())
    {
        clear();
        root_=new ExpandStateNode(rootVn,expanded);        
    }
    else
        root_->expanded_=expanded;

    //save all the children recursively
    save(rootVn,root_,rootIdx);
}


void ExpandState::save(const VNode *node,ExpandStateNode *expand,const QModelIndex& idx)
{
    std::size_t numExpand=expand->children_.size();
    std::size_t numNode=node->numOfChildren();

    //the node and the expand node does not match. We clear the whole
    //contents of the expand node
    if(numExpand != numNode || expand->name_ != node->strName())
    {
        expand->reset(node,view_->isExpanded(idx));
    }

    //At the this point the expand node children vector is
    //reserved, but might contain null pointers

    for(std::size_t i=0; i < numNode; i++)
    {
        VNode* chNode=node->childAt(i);
        QModelIndex chIdx=model_->nodeToIndex(chNode);
        ExpandStateNode* chExpand=expand->children_[i];

        //The expand node exists
        if(chExpand)
        {
            //We only set the expand state when the child node is in the current VTree (i.e. is filtered).
            //Otherwise we keep the original value
            if(chIdx.isValid())
                chExpand->setExpanded(view_->isExpanded(chIdx));
        }
        // ... create a new child expand node at the i-th place
        else
        {
            chExpand=expand->setChildAt(i,chNode,view_->isExpanded(chIdx));
        }

        //save all the children recursively
        save(chNode,chExpand,chIdx);
    }
}

void ExpandState::collectExpanded(const VTreeNode* node,QSet<QPersistentModelIndex>& theSet)
{
    if(!root_)
        return;

    if(node->vnode()->strName() != root_->name_)
    {
        clear();
        return;
    }

    QModelIndex nodeIdx=model_->nodeToIndex(node);
#ifdef _UI_EXPANDSTATE_DEBUG
    UiLog().dbg() << "ExpandState::collectExpanded --> " << root_->name_;
#endif
    collectExpanded(root_,node,nodeIdx,theSet);
#ifdef _UI_EXPANDSTATE_DEBUG
    UiLog().dbg() << "<-- ExpandState::collectExpanded";
#endif
    //clear();
}

void ExpandState::collectExpanded(ExpandStateNode *expand,const VTreeNode* node,const QModelIndex& nodeIdx,
                                QSet<QPersistentModelIndex>& theSet)
{
    //Lookup the node in the model
    //QModelIndex nodeIdx=model_->nodeToIndex(node);
    if(expand->expanded_)
    {
        theSet.insert(nodeIdx);

#ifdef _UI_EXPANDSTATE_DEBUG
        UiLog().dbg() << "  " << expand->name_;
#endif
    }

    for(int i=0; i < expand->children_.size(); i++)
    {
        ExpandStateNode *chExpand=expand->children_[i];      
        if(VTreeNode *chNode=node->findChild(chExpand->name_))
        {
            QModelIndex chIdx=model_->nodeToIndex(chNode);
            //if(chExpand->expanded_)
                collectExpanded(chExpand,chNode,chIdx,theSet);
            //}
        }
    }
}
