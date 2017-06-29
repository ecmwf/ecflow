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
#include "NodeViewBase.hpp"
#include "TreeNodeModel.hpp"
#include "VNode.hpp"
#include "VTree.hpp"

ExpandState::ExpandState(NodeViewBase* view,TreeNodeModel* model) :
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
    bool expanded=view_->isNodeExpanded(rootIdx);
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


void ExpandState::save(const VNode *parentVn,ExpandStateNode *parentExpand,const QModelIndex& parentIdx)
{
    std::size_t numExpand=parentExpand->children_.size();
    std::size_t numNode=parentVn->numOfChildren();

    //the node and the expand node does not match. We clear the whole
    //contents of the expand node
    if(numExpand != numNode || parentExpand->name_ != parentVn->strName())
    {
        parentExpand->clear();
        parentExpand->reserveChildren(numNode);
    }

    //At the this point the expand node children vector is
    //reserved, but might contain null pointers

    for(std::size_t i=0; i < numNode; i++)
    {
        VNode* vn=parentVn->childAt(i);
        QModelIndex chIdx=model_->nodeToIndex(vn);
        ExpandStateNode* chExpand=parentExpand->children_[i];

        //The expand node exists
        if(chExpand)
        {
            //We only set the expand state when the child node is in the current VTree (i.e. is filtered).
            //Otherwise we keep the original value
            if(chIdx.isValid())
                chExpand->setExpanded(view_->isNodeExpanded(chIdx));
        }
        // ... create a new child expand node at the i-th place
        else
        {
            chExpand=parentExpand->setChildAt(i,vn,view_->isNodeExpanded(chIdx));
        }

        //save all the children recursively
        save(vn,chExpand,chIdx);
    }
}

#if 0
template <typename View>
void ExpandState<View>::save(ExpandStateNode *parentExpand,const QModelIndex& parentIdx)
{
    for(int i=0; i < model_->rowCount(parentIdx); i++)
    {
        QModelIndex chIdx=model_->index(i, 0, parentIdx);

        if(!view_->isExpanded(chIdx))
            continue;
        else
        {
            ExpandStateNode* expand=parentExpand->add(chIdx.data(Qt::DisplayRole).toString().toStdString());
            save(expand,chIdx);
        }
    }
}
#endif

//Save the expand state for the given node (it can be a server as well)
void ExpandState::restore(const VTreeNode* node)
{
    if(!root_)
        return;

    if(node->vnode()->strName() != root_->name_)
    {
        clear();
        return;
    }

    restore(node->vnode(),root_);
    clear();
}

void ExpandState::restore(const VNode* node,ExpandStateNode *expand)
{
#if 0
    //Lookup the node in the model
    QModelIndex nodeIdx=model_->nodeToIndex(node);
    if(nodeIdx != QModelIndex())
    {
        view_->setExpanded(nodeIdx,true);
    }
    else
    {
        return;
    }

    for(int i=0; i < expand->children_.size(); i++)
    {
        ExpandStateNode *chExpand=expand->children_.at(i);
        std::string name=chExpand->name_;

        if(VTreeNode *chNode=node->findChild(name))
        {
            QModelIndex chIdx=model_->nodeToIndex(chNode);
            if(chIdx != QModelIndex())
            {
                //setExpanded(chIdx,true);
                restore(chExpand,chNode);
            }
        }
    }
#endif
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
    collectExpanded(root_,node,nodeIdx,theSet);
    clear();
}



void ExpandState::collectExpanded(ExpandStateNode *expand,const VTreeNode* node,const QModelIndex& nodeIdx,
                                QSet<QPersistentModelIndex>& theSet)
{
    //Lookup the node in the model
    //QModelIndex nodeIdx=model_->nodeToIndex(node);
    if(nodeIdx != QModelIndex())
    {
        theSet.insert(nodeIdx);
        //view_->setExpanded(nodeIdx,true);
    }
    else
    {
        return;
    }

    for(int i=0; i < expand->children_.size(); i++)
    {
        ExpandStateNode *chExpand=expand->children_[i];
        std::string name=chExpand->name_;

        if(VTreeNode *chNode=node->findChild(name))
        {
            QModelIndex chIdx=model_->nodeToIndex(chNode);
            if(chIdx != QModelIndex())
            {
                //setExpanded(chIdx,true);
                collectExpanded(chExpand,chNode,chIdx,theSet);
            }
        }
    }
}






#if 0

template <typename View>
ExpandState<View>::ExpandState(View * view,TreeNodeModel* model) :
    view_(view), model_(model), root_(0)
{
}

template <typename View>
ExpandState<View>::~ExpandState()
{
	clear();
}

template <typename View>
void ExpandState<View>::clear()
{
	if(root_)
		delete root_;

	root_=0;
}

template <typename View>
bool ExpandState<View>::rootSameAs(const std::string& name) const
{
    return (root_ && root_->name_ == name);
}

template <typename View>
ExpandStateNode* ExpandState<View>::setRoot(VNode* r,bool expanded)
{
	if(root_)
		clear();

    root_=new ExpandStateNode(r,expanded);
	return root_;
}


//Save the expand state for the given node (it can be a server as well)
template <typename View>
void ExpandState<View>::save(const VTreeNode *root)
{
    assert(root);

    VNode* rootVn=root->vnode();

    QModelIndex rootIdx=model_->nodeToIndex(root);
    bool expanded=view_->isExpanded(rootIdx);
    if(root_ == 0 || root_->name_ != root->vnode()->strName())
    {
        clear();
        root_=new ExpandStateNode(r,expanded);
    }
    else
        root_->expanded_=expanded;

    //save all the children recursively
    save(rootVn,root_,rootIdx);
}


template <typename View>
void ExpandState<View>::save(const VNode *parentVn,ExpandStateNode *parentExpand,const QModelIndex& parentIdx)
{
    std::size_t numExpand=parentExpand->children_.size();
    std::size_t numNode=parentVn->children_.size();

    //the node and the expand node does not match. We clear the whole
    //contents of the expand node
    if(numExpand != numNode || parentExpand->name_ != parentVn->strName())
    {
        parentExpand->clear();
        parentExpand->reserveChildren(numNode);
    }

    //At the this point the expand node children vector is
    //reserved, but might contain null pointers

    for(std::size_t i=0; i < numNode; i++)
    {
        VNode* vn=parentVn->childAt(i);
        QModelIndex chIdx=model_->nodeToIndex(vn);
        ExpandStateNode* chExpand=parentExpand->children_[i];

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
            chExpand=parentExpand->setChildAt(i,vn,view_->isExpanded(chIdx));
        }

        //save all the children recursively
        save(vn,chExpand,chIdx);
    }
}

#if 0
template <typename View>
void ExpandState<View>::save(ExpandStateNode *parentExpand,const QModelIndex& parentIdx)
{
    for(int i=0; i < model_->rowCount(parentIdx); i++)
    {
        QModelIndex chIdx=model_->index(i, 0, parentIdx);

        if(!view_->isExpanded(chIdx))
            continue;
        else
        {
            ExpandStateNode* expand=parentExpand->add(chIdx.data(Qt::DisplayRole).toString().toStdString());
            save(expand,chIdx);
        }
    }
}
#endif

//Save the expand state for the given node (it can be a server as well)
template <typename View>
void ExpandState<View>::restore(const VTreeNode* node)
{
    if(!root_)
        return;

    if(node->vnode()->strName() != root_->name_)
    {
        clear();
        return;
    }

    restore(node->vnode(),root_);
    clear();
}

template <typename View>
void ExpandState<View>::restore(const VNode* node,ExpandStateNode *expand)
{
    //Lookup the node in the model
    QModelIndex nodeIdx=model_->nodeToIndex(node);
    if(nodeIdx != QModelIndex())
    {
        view_->setExpanded(nodeIdx,true);
    }
    else
    {
        return;
    }

    for(int i=0; i < expand->children_.size(); i++)
    {
        ExpandStateNode *chExpand=expand->children_.at(i);
        std::string name=chExpand->name_;

        if(VTreeNode *chNode=node->findChild(name))
        {
            QModelIndex chIdx=model_->nodeToIndex(chNode);
            if(chIdx != QModelIndex())
            {
                //setExpanded(chIdx,true);
                restore(chExpand,chNode);
            }
        }
    }
}

template <typename View>
void ExpandState<View>::collectExpanded(const VTreeNode* node,QSet<QPersistentModelIndex>& theSet)
{
    if(!root_)
        return;

    if(node->vnode()->strName() != root_->name_)
    {
        clear();
        return;
    }

    QModelIndex nodeIdx=model_->nodeToIndex(node);
    collectExpanded(root_,node,nodeIdx,theSet);
    clear();
}


template <typename View>
void ExpandState<View>::collectExpanded(ExpandStateNode *expand,const VTreeNode* node,const QModelIndex& nodeIdx,
                                QSet<QPersistentModelIndex>& theSet)
{
    //Lookup the node in the model
    //QModelIndex nodeIdx=model_->nodeToIndex(node);
    if(nodeIdx != QModelIndex())
    {
        theSet.insert(nodeIdx);
        //view_->setExpanded(nodeIdx,true);
    }
    else
    {
        return;
    }

    for(int i=0; i < expand->children_.size(); i++)
    {
        ExpandStateNode *chExpand=expand->children_[i];
        std::string name=chExpand->name_;

        if(VTreeNode *chNode=node->findChild(name))
        {
            QModelIndex chIdx=model_->nodeToIndex(chNode);
            if(chIdx != QModelIndex())
            {
                //setExpanded(chIdx,true);
                collectExpanded(chExpand,chNode,chIdx,theSet);
            }
        }
    }
}

#endif
