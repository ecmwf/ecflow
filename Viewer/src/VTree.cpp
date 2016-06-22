//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VTree.hpp"

#include "ServerHandler.hpp"
#include "UserMessage.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"
#include "VFilter.hpp"
#include "VModelData.hpp"

//==========================================
//
// VTreeNode
//
//==========================================


VTreeNode::VTreeNode(VNode* n,VTreeNode* parent) : vnode_(n), parent_(parent), attrNum_(-1)
{
    if(parent_)
        parent_->addChild(this);
}

VTreeNode::~VTreeNode()
{
    for(std::vector<VTreeNode*>::iterator it=children_.begin(); it != children_.end();++it)
        delete *it;
}

VTree* VTreeNode::root() const
{
    return (parent_)?parent_->root():NULL;
}

VTreeServer* VTreeNode::server() const
{
    return (parent_)?parent_->server():NULL;
}

void VTreeNode::addChild(VTreeNode* ch)
{
    children_.push_back(ch);
}

VTreeNode* VTreeNode::findChild(const std::string& name) const
{
    for(unsigned int i=0; i < children_.size(); i++)
    {
        if(children_[i]->vnode_->strName() == name)
            return children_[i];
    }

    return NULL;
}


int VTreeNode::indexOfChild(const VTreeNode* vn) const
{
    for(unsigned int i=0; i < children_.size(); i++)
    {
        if(children_[i] == vn)
            return i;
    }

    return -1;
}

int VTreeNode::indexInParent() const
{
    if(parent_)
    {
        return parent_->indexOfChild(this);
    }
    return -1;
}

bool VTreeNode::isAttrInitialised() const
{
    return (attrNum_ != -1);
}

int VTreeNode::attrRow(int row,AttributeFilter *filter) const
{
    return VAttributeType::getRow(vnode_,row,filter);
}

int VTreeNode::attrNum(AttributeFilter *filter) const
{
    if(!isAttrInitialised())
        attrNum_=vnode_->attrNum(filter);

    return attrNum_;
}

void VTreeNode::updateAttrNum(AttributeFilter *filter)
{
    attrNum_=vnode_->attrNum(filter);
}

void VTreeNode::resetAttrNum()
{
    attrNum_=-1;
    for(unsigned int i=0; i < children_.size(); i++)
    {
        children_[i]->resetAttrNum();
    }
}

//========================================================
//
// VTree
//
//========================================================

VTree::VTree(VTreeServer* server) : VTreeNode(server->realServer()->vRoot(),0), server_(server)
{

}

VTree::~VTree()
{
    clear();
}

VTree* VTree::root() const
{
    return const_cast<VTree*>(this);
}

VNode* VTree::vnodeAt(int index) const
{
    return (nodeVec_[index])?(nodeVec_[index]->vnode()):NULL;
}

VTreeNode* VTree::find(const VNode* vn) const
{
    Q_ASSERT(vn->index()  < nodeVec_.size());
    return nodeVec_[vn->index()];
}

VTreeNode *VTree::findAncestor(const VNode* vn)
{
    VNode* p=vn->parent();
    while(p)
    {
        if(VTreeNode* n=find(p))
            return n;

        p=p->parent();
    }

    return NULL;
}

int VTree::totalNumOfTopLevel(VTreeNode* n) const
{
    if(!n->isTopLevel())
        return -1;

    int idx=indexOfChild(n);
    if(idx != -1)
        return totalNumOfTopLevel(idx);

    return -1;
}

int VTree::totalNumOfTopLevel(int idx) const
{
    assert(totalNumInChild_.size() == children_.size());

    if(idx >=0 && idx < totalNumInChild_.size())
    {
        return totalNumInChild_.at(idx);
    }

    return -1;
}



#if 0
VTreeNode* VTree::topLevelNode(int row) const
{
    return children_[row];
}

int VTree::indexOfTopLevelNode(const VTreeNode* node) const
{
    return indexOfChild(node);
}

int VTree::topLevelNodeNum() const
{
    return numOfChildren();
}
#endif

void VTree::removeChildren(VTreeNode* node)
{
    for(std::vector<VTreeNode*>::iterator it=node->children_.begin(); it != node->children_.end();++it)
    {
        nodeVec_[(*it)->vnode()->index()]=NULL;
        removeChildren(*it);
        delete *it;
    }

    node->children_.clear();
}

void VTree::buildBranch(const std::vector<VNode*>& filter,VTreeNode* node,VTreeNode* branch)
{
    VNode* vnode=node->vnode();
    assert(filter[vnode->index()] != NULL);

    for(unsigned int i=0; i < vnode->numOfChildren();i++)
    {
        build(branch,vnode->childAt(i),filter);
    }
}

void VTree::addBranch(VTreeNode* node,VTreeNode* branch)
{
    assert(node->vnode() == branch->vnode());

    for(unsigned int i=0; i < branch->numOfChildren();i++)
    {
        branch->childAt(i)->parent_=node;
        node->addChild(branch->childAt(i));
    }

    branch->children_.clear();
}


void VTree::clear()
{
    for(std::vector<VTreeNode*>::iterator it=children_.begin(); it != children_.end();++it)
        delete *it;

    children_.clear();
    nodeVec_.clear();
    attrNum_=-1;
    totalNum_=0;
    totalNumInChild_.clear();
}

void VTree::build(const std::vector<VNode*>& filter)
{
    clear();
    VServer* s=server_->realServer()->vRoot();
    nodeVec_.resize(s->totalNum());
    VTreeNode *nptr=0;
    std::fill(nodeVec_.begin(), nodeVec_.end(), nptr);

    int prevTotalNum=0;
    for(unsigned int j=0; j < s->numOfChildren();j++)
    {
        build(this,s->childAt(j),filter);
        totalNumInChild_.push_back(totalNum_-prevTotalNum-1);
        prevTotalNum=totalNum_;
    }
}

void VTree::build(VTreeNode* parent,VNode* node,const std::vector<VNode*>& filter)
{
     if(filter[node->index()])
     {
         VTreeNode *n=new VTreeNode(node,parent);
         totalNum_++;
         nodeVec_[node->index()]=n;

         for(unsigned int j=0; j < node->numOfChildren();j++)
         {
             build(n,node->childAt(j),filter);
         }
     }
}

void VTree::build()
{
    clear();
    VServer* s=server_->realServer()->vRoot();
    nodeVec_.resize(s->totalNum());
    VTreeNode *nptr=0;
    std::fill(nodeVec_.begin(), nodeVec_.end(), nptr);

    int prevTotalNum=0;
    for(unsigned int j=0; j < s->numOfChildren();j++)
    {     
        build(this,s->childAt(j));
        totalNumInChild_.push_back(totalNum_-prevTotalNum-1);
        prevTotalNum=totalNum_;
    }
}

void VTree::build(VTreeNode* parent,VNode* vnode)
{
    VTreeNode *n=new VTreeNode(vnode,parent);
    nodeVec_[vnode->index()]=n;
    totalNum_++;

    //Preallocates the children. With this we will only use the memory we really need.
    if(vnode->numOfChildren() > 0)
        n->children_.reserve(vnode->numOfChildren());

    for(unsigned int j=0; j < vnode->numOfChildren();j++)
    {
        build(n,vnode->childAt(j));
    }
}

