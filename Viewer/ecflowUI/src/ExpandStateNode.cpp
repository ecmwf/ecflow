//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ExpandStateNode.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "VNode.hpp"

ExpandStateNode::ExpandStateNode(const VNode* node,bool expanded) :
    expanded_(expanded),
    expandedAll_(0),
    collapsedAll_(0)
{
    name_=node->strName();
    reserveChildren(node->numOfChildren());
}

ExpandStateNode::~ExpandStateNode()
{
    clear();
}

//Clear the contents and create a new children vector
//with NULLs!
void ExpandStateNode::reset(const VNode* node,bool expanded)
{
    clear();
    name_=node->strName();
    expanded_=expanded;
    expandedAll_=0;
    collapsedAll_=0;
    reserveChildren(node->numOfChildren());
}

void ExpandStateNode::clear()
{
    name_.clear();
    expanded_=0;
    expandedAll_=0;
    collapsedAll_=0;
    for(auto & i : children_)
    {
        delete i;
    }
    children_=std::vector<ExpandStateNode*>();
}

//Create new children vector filled with NULLs
void ExpandStateNode::reserveChildren(std::size_t num)
{
    UI_ASSERT(children_.size() == 0,"children_.size()=" << children_.size());

    ExpandStateNode *exn=nullptr;
    children_=std::vector<ExpandStateNode*>();
    children_.resize(num,exn);
}

//Allocate a new child and place it into then children vector at the specified position
ExpandStateNode* ExpandStateNode::setChildAt(std::size_t index,VNode* node,bool expanded)
{
    auto *exn=new ExpandStateNode(node,expanded);
    children_[index]=exn;
    return exn;
}

//"Expand all children" was called on the node
void ExpandStateNode::setExpandedAll()
{
    expandedAll_=1;
    collapsedAll_=0;
    //Set the expand state on all the descendants
    std::size_t num=children_.size();
    for(std::size_t i=0; i < num; i++)
        children_[i]->setExpandedRecursively(1);
}

//"Collapse all children" was called on the node
void ExpandStateNode::setCollapsedAll()
{
    expandedAll_=0;
    collapsedAll_=1;
    //Set the expand state on all the descendants
    std::size_t num=children_.size();
    for(std::size_t i=0; i < num; i++)
        children_[i]->setExpandedRecursively(0);
}

void ExpandStateNode::setExpandedRecursively(unsigned int expanded)
{   
    expanded_=expanded;
    expandedAll_=0;
    collapsedAll_=0;
    std::size_t num=children_.size();
    for(std::size_t i=0; i < num; i++)
    {       
        children_[i]->setExpandedRecursively(expanded);
    }
}

//Find a descendant
ExpandStateNode* ExpandStateNode::find(const std::vector<std::string>& pathVec)
{
    if(pathVec.size() == 0)
        return this;

    if(pathVec.size() == 1)
    {
        return findChild(pathVec.at(0));
    }

    std::vector<std::string> rest(pathVec.begin()+1,pathVec.end());
    ExpandStateNode*n = findChild(pathVec.at(0));

    return n?n->find(rest):nullptr;
}

//Find a child with the given name
ExpandStateNode* ExpandStateNode::findChild(const std::string& theName) const
{
    std::size_t num=children_.size();
    for(std::size_t i=0; i < num; i++)
    {
        //A child can be NULL temporarily
        if(children_[i] && children_[i]->name_ == theName)
            return children_[i];
    }
    return nullptr;
}

//Find a child with the given name. Returns its position as well.
ExpandStateNode* ExpandStateNode::findChild(const std::string& theName,std::size_t& pos) const
{
    pos=-1;
    std::size_t num=children_.size();
    for(std::size_t i=0; i < num; i++)
    {
        //A child can be NULL temporarily
        if(children_[i] && children_[i]->name_ == theName)
        {
            pos=i;
            return children_[i];
        }
    }
    return nullptr;
}

void ExpandStateNode::print(std::string& indent,bool recursive) const
{
    UiLog().dbg() << indent <<  name_ << " " << expanded_;
    if(recursive)
    {
        indent+="  ";
        std::size_t num=children_.size();
        for(std::size_t i=0; i < num; i++)
            children_[i]->print(indent,true);

        indent=indent.substr(0,indent.size()-2);
    }
}

//Adjust the contents of the given expand node to the contents of the vnode it
//represents. Return true if an adjustment was actually needed.
bool ExpandStateNode::adjustContents(const VNode* node)
{
    std::size_t numExpand=children_.size();
    std::size_t numNode=node->numOfChildren();

    //Check if the children of the expand node and the vnode are the same.
    //They migh differ when:
    //  -the order of the nodes changed
    //  -nodes were added/removed
    bool same=false;
    if(numExpand == numNode)
    {
        same=true;
        for(std::size_t i=0; i < numNode; i++)
        {          
            //items in children can be null pointers
            if(!children_[i] || children_[i]->name_ != node->childAt(i)->strName())
            {
                same=false;
                break;
            }
        }
    }

    //the children of the expand node and the vnode are not the same!!!
    if(!same)
    {
        //create a new children vector for the expand node and
        //copy all the expand children into it whose name appear in the
        //vnode children
        std::vector<ExpandStateNode*> chVec;
        for(std::size_t i=0; i < numNode; i++)
        {
            std::size_t chPos=-1;
            if(ExpandStateNode* chExpand=findChild(node->childAt(i)->strName(),chPos))
            {
                chVec.push_back(chExpand);
                children_[chPos]=nullptr;
            }
            else
                chVec.push_back(nullptr);
        }

        //Delete the children vector of the expand node. It eaither contains NULLs or
        //children objects which do not exist anymore!
        for(std::size_t i=0; i < numExpand; i++)
        {
            if(children_[i] != nullptr)
                delete children_[i];
        }

        children_.clear();

        //reassign the children vector to the expand node
        children_=chVec;
     }

    return !same;
}




