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
#include "ServerHandler.hpp"
#include "TreeNodeModel.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "VNode.hpp"
#include "VTree.hpp"

#include <boost/algorithm/string.hpp>

//#define _UI_EXPANDSTATE_DEBUG

ExpandState::ExpandState(AbstractNodeView* view,TreeNodeModel* model) :
    view_(view), model_(model), root_(0)
{
}

ExpandState::~ExpandState()
{
    clear();
}

void ExpandState::init(const VNode *vnode)
{
    clear();

    ServerHandler* server=vnode->server();
    Q_ASSERT(server);
    QModelIndex idx=model_->serverToIndex(server);
    bool expanded=view_->isExpanded(idx);

    root_=new ExpandStateNode(server->vRoot(),expanded);

    save(server->vRoot());
}

bool ExpandState::isEmpty() const
{
    return  !root_ || root_->children_.size() == 0;
}

void ExpandState::clear()
{
    if(root_)
        delete root_;

    root_=0;
}


//Save the expand state for a whole subtree (it can be the whole VNode tree as well)
void ExpandState::save(const VNode *vnode)
{
    UI_FUNCTION_LOG
    UI_ASSERT(vnode,"node is null");
    UiLog().dbg() << " " << vnode->name();

    if(!root_)
    {
        init(vnode);
        Q_ASSERT(root_);
        return;
    }

    if(ExpandStateNode* es=find(vnode->absNodePath()))
    {
        Q_ASSERT(root_);
        QModelIndex idx=model_->nodeToIndex(vnode);

        //It can happen that the VTree is empty but the server is already
        //loaded. It is the situation when we load the same server in multiple
        //tabs!!! We return here because there is nothing to save (and we would
        //have a crash when trying to get index of the vnodes via the model!!)
        if(VTreeNode* vtn=model_->indexToServerOrNode(idx))
        {
            if(VTree* vt=vtn->root())
            {
                if(vt->totalNum() == 0)
                    return;
            }
        }

        //save all the children recursively
        save(vnode,es,idx);
     }
}

#if 0
//Save the expand state for a whole subtree (it can be the whole VNode tree as well)
void ExpandState::save(const VNode *root)
{    
    UI_FUNCTION_LOG
    UI_ASSERT(root,"root is null");
    UiLog().dbg() << " " << root->name();

    QModelIndex rootIdx=model_->nodeToIndex(root);
    bool expanded=view_->isExpanded(rootIdx);
    if(root_ == 0 || root_->name_ != root->strName())
    {
        clear();
        root_=new ExpandStateNode(root,expanded);
    }
    else
    {
        root_->expanded_=expanded;
    }

    //It can happen that the VTree is empty but the server is already
    //loaded. It is the situation when we load the same server in multiple
    //tabs!!! We return here because there is nothing to save (and we would
    //have a crash when trying to get index of the vnodes via the model!!)
    if(VTreeNode* vtn=model_->indexToServerOrNode(rootIdx))
    {
        if(VTree* vt=vtn->root())
        {
            if(vt->totalNum() == 0)
                return;
        }
    }

    //save all the children recursively
    save(root,root_,rootIdx);
}
#endif

//Save the expand state for the given node.
//  node: the vnode to save
//  expandNode: the expand node representing the vnode
//  idx: the modelindex of the vnode in the TreeNodeModel. It can be invalid
//       since the model can use a filter and its tree (the VTree) might represent only
//       the subset of all the vnodes of a given server
void ExpandState::save(const VNode *node,ExpandStateNode *expandNode,const QModelIndex& idx)
{
    std::size_t numExpandNode=expandNode->children_.size();
    std::size_t numNode=node->numOfChildren();

    //the node and the expand node does not match. We clear the whole
    //contents of the expand node
    //CAN IT HAPPEN AT ALL??
    if(numExpandNode != numNode || expandNode->name_ != node->strName())
    {
        UI_ASSERT(0,"numExpandNode=" << numExpandNode << " numNode=" << numNode <<
                  " expandNode->name_=" << expandNode->name_ << " node->strName()=" <<
                  node->strName());

        expandNode->reset(node,view_->isExpanded(idx));
        //At the this point the expand node children vector is
        //reserved, but contains null pointers
    }

    for(std::size_t i=0; i < numNode; i++)
    {
        VNode* chNode=node->childAt(i);
        QModelIndex chIdx=model_->nodeToIndex(chNode);
        ExpandStateNode* chExpandNode=expandNode->children_[i];

        //The expand node exists
        if(chExpandNode)
        {
            //We only set the expand state when the child node is in the current VTree (i.e. is filtered).
            //Otherwise we keep the original value
            if(chIdx.isValid())
                chExpandNode->setExpanded(view_->isExpanded(chIdx));
        }
        // ... create a new child expand node at the i-th place
        else
        {
            chExpandNode=expandNode->setChildAt(i,chNode,view_->isExpanded(chIdx));
        }

        //save all the children recursively
        save(chNode,chExpandNode,chIdx);
    }
}

//Collect the modelindexes of the expanded nodes in a VNode subtree.
//This is called after a server scan ended so the structure of the VNode tree and the
//expand tree might not match. In this case the expand tree has to be adjusted to the
//VNode tree.
void ExpandState::collectExpanded(const VNode* node,QSet<QPersistentModelIndex>& theSet)
{
#ifdef _UI_EXPANDSTATE_DEBUG
    UI_FUNCTION_LOG
#endif
    Q_ASSERT(node);

    if(!root_)
        return;

    QModelIndex nodeIdx=model_->nodeToIndex(node);
#ifdef _UI_EXPANDSTATE_DEBUG
    UiLog().dbg() << " root=" << root_->name_;
#endif
    ExpandStateNode *expand=find(node->absNodePath());
    if(expand)
        collectExpanded(expand,node,nodeIdx,theSet);
    else
    {
#ifdef _UI_EXPANDSTATE_DEBUG
        UiLog().dbg() << " Node not found in expand tree";
#endif
    }
}

void ExpandState::collectExpanded(ExpandStateNode *expand,const VNode* node,const QModelIndex& nodeIdx,
                                QSet<QPersistentModelIndex>& theSet)
{
#ifdef _UI_EXPANDSTATE_DEBUG
    UI_FUNCTION_LOG
#endif
    //The contents of expand node and the vnode might differ. We try to
    //adjust the expand node to the vnode with this call.
    bool adjusted=expand->adjustContents(node);

    std::size_t numExpand=expand->children_.size();
    std::size_t numNode=node->numOfChildren();

    UI_ASSERT(numExpand==numNode,"node=" << node->strName() << " numExpand=" << numExpand <<
              " numNode=" << numNode);

    //Lookup the node in the model
    //QModelIndex nodeIdx=model_->nodeToIndex(node);
    if(expand->expanded_ && nodeIdx.isValid())
    {
        theSet.insert(nodeIdx);

#ifdef _UI_EXPANDSTATE_DEBUG
        UiLog().dbg() << "  " << expand->name_;
#endif
    }

    //We need to see what to do with newly added nodes. We either expand
    //all its children or leave it unexpanded. It depends on the expandedAll_
    //flag set in any of the parents.
    bool parentExpandedAll=false;
    if(adjusted)
    {
        parentExpandedAll=needToExpandNewChild(expand,node->absNodePath());
    }

    for(std::size_t i=0; i < numExpand; i++)
    {
        VNode *chNode=node->childAt(i);
        UI_ASSERT(chNode,"chNode is null");
        QModelIndex chIdx=model_->nodeToIndex(chNode);
        ExpandStateNode *chExpand=expand->children_[i];

        //An existing expand node
        if(chExpand)
        {
            UI_ASSERT(chExpand->name_ == chNode->strName()," chExpand->name_=" << chExpand->name_ << " chNode->strName()=" << chNode->strName());
        }

        //A not-yet-allocated expand node. The corresponding vnode has just been
        //added to the tree. We have to decide what to do with its expand state!!!
        else
        {
            //create an expand node. Collapsed by default
            chExpand=expand->setChildAt(i,chNode,0);

            //save all the children recursively
            save(chNode,chExpand,chIdx);

            //expand recursively the new expand node if needed
            if(parentExpandedAll)
                chExpand->setExpandedRecursively(1);
            else
            {
                UiLog().dbg() << " newly added node not expanded!!";
            }

        }

        collectExpanded(chExpand,chNode,chIdx,theSet);
    }
}


void ExpandState::saveExpandAll(const VNode* node)
{
    UI_ASSERT(node,"node is null");
    if(ExpandStateNode* expand=find(node->absNodePath()))
    {       
        expand->setExpandedAll();
    }
}

void ExpandState::saveCollapseAll(const VNode* node)
{
    UI_ASSERT(node,"node is null");
    if(ExpandStateNode* expand=find(node->absNodePath()))
    {        
        expand->setCollapsedAll();
    }
}

//Find an expand node using its ful path
ExpandStateNode* ExpandState::find(const std::string& fullPath)
{
    if(!root_)
        return NULL;

    if(fullPath.empty())
        return NULL;

    if(fullPath == "/")
        return root_;

    std::vector<std::string> pathVec;
    boost::split(pathVec,fullPath,boost::is_any_of("/"));

    if(pathVec.size() > 0 && pathVec[0].empty())
    {
        pathVec.erase(pathVec.begin());
    }

    return root_->find(pathVec);
}

bool ExpandState::needToExpandNewChild(ExpandStateNode* expandNode,const std::string& expandNodePath) const
{

    if(expandNode->expandedAll_ ==  1)
        return true;

    if(expandNode->collapsedAll_ ==  1)
        return false;

    //Checks if any of the parents have expandedAll set. In this case we
    //expand recursively all the new expand node
    std::vector<ExpandStateNode*> parents;
    collectParents(expandNodePath,parents);
    for(auto it=parents.rbegin();
        it != parents.rend(); ++it)
    {
        if((*it)->expandedAll_ ==  1)
        {
            return true;
        }
        else if((*it)->collapsedAll_ ==  1)
        {
            return false;
        }
     }
     return false;
}

//We need to do it this way because we do not store the parents in the nodes for memory efficiency.
void ExpandState::collectParents(const std::string& fullPath,std::vector<ExpandStateNode*>& parents) const
{
    if(!root_)
        return;

    //how split works:
    //  str="/"  -> "",""
    //  str="/a" -> "","a"
    //  str=" "  -> " "
    //  str=""   -> ""

    std::vector<std::string> pathVec;
    boost::split(pathVec,fullPath,boost::is_any_of("/"));
    if(pathVec.size() > 0 && pathVec[0].empty())
    {
        pathVec.erase(pathVec.begin());
    }

    ExpandStateNode *p=root_;
    std::size_t num=pathVec.size();
    for(std::size_t i=0; i < num && p != 0; i++)
    {
        parents.push_back(p);
        p=p->findChild(pathVec[i]);
    }
}


void ExpandState::print() const
{
    if(!root_)
        return;

    std::string indent="";
    root_->print(indent,true);
}
