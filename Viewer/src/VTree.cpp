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


VTreeNode::VTreeNode(VNode* n,VTreeNode* parent) :
    vnode_(n),
    parent_(parent),
    attrNum_(-1)
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

#if 0
int VTreeNode::attrRow(int row,AttributeFilter *filter) const
{
    return VAttributeType::getRow(vnode_,row,filter);
}
#endif

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

#if 0
void VTreeNode::updateAttrNum(AttributeFilter *filter,bool showOneMore)
{
    attrNum_=vnode_->attrNum(filter);
    if(showOneMore)
        attrNum_++;
}
#endif

void VTreeNode::resetAttrNum()
{
    attrNum_=-1;
    for(unsigned int i=0; i < children_.size(); i++)
    {
        children_[i]->resetAttrNum();
    }
}

void VTreeNode::countChildren() const
{
}

void VTreeNode::countChildren(int& num) const
{
    for(unsigned int i=0; i < children_.size(); i++)
    {
        num++;
        children_[i]->countChildren(num);
    }
}

int VTreeNode::totalNumOfChildren() const
{
    int num=0;
    countChildren(num);
    return num;
}



VTreeSuiteNode::VTreeSuiteNode(VNode* n,VTreeNode* parent) : VTreeNode(n,parent), num_(0)
{
}

void VTreeSuiteNode::countChildren() const
{
    num_=0;
    VTreeNode::countChildren(num_);
}

int VTreeSuiteNode::totalNumOfChildren() const
{
    return num_;
}

//========================================================
//
// VTree
//
//========================================================

VTree::VTree(VTreeServer* server) :
    VTreeNode(server->realServer()->vRoot(),0),
    server_(server),
    forceShowNode_(0),
    forceShowAttr_(0)
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

    return this;
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
    assert(idx >=0 && idx < children_.size());

    return children_[idx]->totalNumOfChildren();
}

int VTree::indexOfTopLevel(VTreeNode* node) const
{
    return indexOfChild(node);
}

int VTree::indexOfTopLevelToInsert(VNode* suite) const
{
    VServer* s=server_->realServer()->vRoot();
    int suiteIdx=s->indexOfChild(suite);
    assert(suiteIdx >=0);

    for(unsigned int i=0; i < numOfChildren();i++)
    {
        int idx=s->indexOfChild(children_[i]->vnode_);
        assert(idx >=0);
        if(suiteIdx < idx)
            return i;
    }

    return numOfChildren();
}

void VTree::removeChildren(VTreeNode* node)
{
    for(std::vector<VTreeNode*>::iterator it=node->children_.begin(); it != node->children_.end();++it)
    {
        nodeVec_[(*it)->vnode()->index()]=NULL;
        removeChildren(*it);
        delete *it;
        totalNum_--;
    }

    node->children_.clear();
}

void VTree::remove(VTreeNode* node)
{
    VTreeNode* p=node->parent();
    assert(p);
    std::vector<VTreeNode*>::iterator it=std::find(p->children_.begin(),p->children_.end(),node);
    assert(it != p->children_.end());
    if(it != p->children_.end())
    {
        removeChildren(node);
        p->children_.erase(it);
        delete node;
        nodeVec_[node->vnode()->index()]=NULL;
        totalNum_--;
        assert(totalNum_ >=0);
    }
}


//----------------------------------------------------------------
// Build/insert a branch fro the given parentNode
//----------------------------------------------------------------

VTreeNode* VTree::makeBranch(const std::vector<VNode*>& filter,VTreeNode* parentNode)
{
    VNode* vnode=parentNode->vnode();
    VTreeNode *branch=new VTreeNode(vnode,0);

    assert(filter[vnode->index()] != NULL);

    for(unsigned int i=0; i < vnode->numOfChildren();i++)
    {
        build(branch,vnode->childAt(i),filter);
    }

    return branch;
}

void VTree::replaceWithBranch(VTreeNode* node,VTreeNode* branch)
{
    assert(node->vnode() == branch->vnode());
    assert(node->numOfChildren() == 0);

    for(unsigned int i=0; i < branch->numOfChildren();i++)
    {
        branch->childAt(i)->parent_=node;
        node->addChild(branch->childAt(i));
    }

    //Update the children count in the toplevel node (suite)
    VNode* s=node->vnode()->suite();
    assert(s);
    VTreeNode *sn=nodeVec_[s->index()];
    assert(sn);
    sn->countChildren();

    totalNum_+=branch->totalNumOfChildren();
    branch->children_.clear();
}

//----------------------------------------------------------------
// Build/insert a toplevel node (aka suite)
//----------------------------------------------------------------

VTreeNode* VTree::makeTopLevelBranch(const std::vector<VNode*>& filter,VNode* suite)
{
    assert(suite);
    assert(suite->isSuite());
    VTreeSuiteNode *branch=new VTreeSuiteNode(suite,0);

    for(unsigned int i=0; i < suite->numOfChildren();i++)
    {
        build(branch,suite->childAt(i),filter);
    }

    branch->countChildren();
    return branch;
}

void VTree::insertTopLevelBranch(VTreeNode* branch,int index)
{
    VNode* suite=branch->vnode();
    assert(suite);
    assert(suite->isSuite());
    assert(index >=0 && index <=numOfChildren());
    assert(suite->index() >=0);

    if(index < numOfChildren())
    {
        std::vector<VTreeNode*>::iterator it=children_.begin();
        children_.insert(it+index,branch);
    }
    else
    {
       children_.push_back(branch);
    }

    nodeVec_[suite->index()]=branch;
    branch->parent_=this;
    totalNum_+=branch->totalNumOfChildren()+1;
}

void VTree::clear()
{
    for(std::vector<VTreeNode*>::iterator it=children_.begin(); it != children_.end();++it)
        delete *it;

    children_.clear();
    nodeVec_.clear();
    attrNum_=-1;
    totalNum_=0;
}

//====================================================
// Build the tree by using a filter
//====================================================

void VTree::build(const std::vector<VNode*>& filter)
{
    clear();
    VServer* s=server_->realServer()->vRoot();
    nodeVec_.resize(s->totalNum());
    VTreeNode *nptr=0;
    std::fill(nodeVec_.begin(), nodeVec_.end(), nptr);

    if(filter.empty())
        return;

    assert(filter.size() == nodeVec_.size());

    //int prevTotalNum=0;
    for(unsigned int i=0; i < s->numOfChildren();i++)
    {
        build(this,s->childAt(i),filter);
#if 0
        if(build(this,s->childAt(i),filter))
        {
            //totalNumInChild_.push_back(totalNum_-prevTotalNum-1);
            //prevTotalNum=totalNum_;
        }
#endif
    }

    for(unsigned int i=0; i < numOfChildren();i++)
    {
        children_[i]->countChildren();
        totalNum_+=children_[i]->totalNumOfChildren()+1;
    }
}

bool VTree::build(VTreeNode* parent,VNode* node,const std::vector<VNode*>& filter)
{
     if(filter[node->index()])
     {
         VTreeNode *n=0;
         if(node->isSuite())
             n=new VTreeSuiteNode(node,parent);
         else
             n=new VTreeNode(node,parent);

         assert(n);

         //if(count)
         //{
         //    totalNum_++;
         //}

         nodeVec_[node->index()]=n;

         for(unsigned int j=0; j < node->numOfChildren();j++)
         {
             build(n,node->childAt(j),filter);
         }

         return true;
     }

     return false;
}

//====================================================
// Build the tree by adding all the nodes to it
// (no filter is defined)
//====================================================

void VTree::build()
{
    clear();
    VServer* s=server_->realServer()->vRoot();
    nodeVec_.resize(s->totalNum());
    VTreeNode *nptr=0;
    std::fill(nodeVec_.begin(), nodeVec_.end(), nptr);

    //int prevTotalNum=0;
    for(unsigned int i=0; i < s->numOfChildren();i++)
    {     
        build(this,s->childAt(i));
        children_[i]->countChildren();
        totalNum_+=children_[i]->totalNumOfChildren()+1;
        //totalNumInChild_.push_back(totalNum_-prevTotalNum-1);
        //prevTotalNum=totalNum_;
    }
}

void VTree::build(VTreeNode* parent,VNode* vnode)
{
    VTreeNode *n=0;
    if(vnode->isSuite())
        n=new VTreeSuiteNode(vnode,parent);
    else
        n=new VTreeNode(vnode,parent);

    assert(n);
    nodeVec_[vnode->index()]=n;
    //totalNum_++;

    //Preallocates the children. With this we will only use the memory we really need.
    if(vnode->numOfChildren() > 0)
        n->children_.reserve(vnode->numOfChildren());

    for(unsigned int j=0; j < vnode->numOfChildren();j++)
    {
        build(n,vnode->childAt(j));
    }
}

