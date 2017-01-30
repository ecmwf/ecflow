//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ExpandState.hpp"

#include <QTreeView>

#include "TreeNodeModel.hpp"
#include "VNode.hpp"
#include "VTree.hpp"

//-------------------------------------
// ExapandStateNode
//-------------------------------------

ExpandStateNode::~ExpandStateNode()
{
	clear();
}

void ExpandStateNode::clear()
{
	name_.clear();
	for(unsigned int i=0; i < children_.size(); i++)
	{
		delete children_.at(i);
	}
	children_.clear();
}

ExpandStateNode* ExpandStateNode::add(const std::string& name)
{
    ExpandStateNode *n=new ExpandStateNode(name);
	children_.push_back(n);
	return n;
}

//-------------------------------------
// ExapandStateTree
//-------------------------------------

ExpandStateTree::ExpandStateTree(QTreeView * view,TreeNodeModel* model) :
    view_(view), model_(model), root_(0)
{
}

ExpandStateTree::~ExpandStateTree()
{
	clear();
}

void ExpandStateTree::clear()
{
	if(root_)
		delete root_;

	root_=0;
}

bool ExpandStateTree::rootSameAs(const std::string& name) const
{
    return (root_ && root_->name_ == name);
}

ExpandStateNode* ExpandStateTree::setRoot(const std::string& name)
{
	if(root_)
		clear();

    root_=new ExpandStateNode(name);
	return root_;
}


//Save the expand state for the given node (it can be a server as well)
void ExpandStateTree::save(const VTreeNode *root)
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

void ExpandStateTree::save(ExpandStateNode *parentExpand,const QModelIndex& parentIdx)
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
void ExpandStateTree::restore(const VTreeNode* node)
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

void ExpandStateTree::restore(ExpandStateNode *expand,const VTreeNode* node)
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
//-------------------------------------
// ExapandState
//-------------------------------------

ExpandState::ExpandState(QTreeView* view,TreeNodeModel* model) : view_(view), model_(model)
{

}


ExpandState::~ExpandState()
{
    clear();
}

ExpandStateTree* ExpandState::add()
{
    ExpandStateTree* et=new ExpandStateTree(view_,model_);
    items_ << et;
    return et;
}

void ExpandState::remove(ExpandStateTree* es)
{
    items_.removeOne(es);
    delete es;
}

void ExpandState::clear()
{
    Q_FOREACH(ExpandStateTree* et,items_)
    {
        delete et;
    }
    items_.clear();
}
