//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ExpandStateNode.hpp"
#include "TreeNodeModel.hpp"
#include "VNode.hpp"
#include "VTree.hpp"

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
ExpandStateNode* ExpandState<View>::setRoot(const std::string& name)
{
	if(root_)
		clear();

    root_=new ExpandStateNode(name);
	return root_;
}


//Save the expand state for the given node (it can be a server as well)
template <typename View>
void ExpandState<View>::save(const VTreeNode *root)
{
    assert(root);

    clear();

    QModelIndex rootIdx=model_->nodeToIndex(root);
    if(view_->isExpanded(rootIdx))
    {
        setRoot(root->vnode()->strName());
        save(root_,rootIdx);
    }
}

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

    restore(root_,node);
    clear();
}

template <typename View>
void ExpandState<View>::restore(ExpandStateNode *expand,const VTreeNode* node)
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

