//============================================================================
// Copyright 2009-2017 ECMWF.
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

ExpandStateNode::ExpandStateNode(VNode* node,unsigned int expanded) :
    expanded_(expanded)
{
    name_=node->strName();
    reserveChildren(node->numOfChildren());
}

ExpandStateNode::~ExpandStateNode()
{
    clear();
}

void ExpandStateNode::reset(const VNode* node,unsigned int expanded)
{
    clear();
    name_=node->strName();
    expanded_=expanded;
    reserveChildren(node->numOfChildren());
}

void ExpandStateNode::clear()
{
    name_.clear();
    expanded_=false;
    for(unsigned int i=0; i < children_.size(); i++)
    {
        delete children_.at(i);
    }
    children_.clear();
}

void ExpandStateNode::reserveChildren(std::size_t num)
{
    UI_ASSERT(children_.size() == 0,"children_.size()=" << children_.size());

    ExpandStateNode *exn=0;
    children_=std::vector<ExpandStateNode*>();
    children_.resize(num,exn);
}

ExpandStateNode* ExpandStateNode::setChildAt(std::size_t index,VNode* node,unsigned int expanded)
{
    ExpandStateNode *exn=new ExpandStateNode(node,expanded);
    children_[index]=exn;
    return exn;
}

void ExpandStateNode::setExpandedRecursively(bool expanded)
{
    expanded_=expanded;
    std::size_t num=children_.size();
    for(std::size_t i=0; i < num; i++)
    {
        children_[i]->setExpandedRecursively(expanded);
    }
}

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

    return n?n->find(rest):NULL;
}

ExpandStateNode* ExpandStateNode::findChild(const std::string& theName) const
{
    std::size_t num=children_.size();
    for(std::size_t i=0; i < num; i++)
    {
        if(children_[i]->name_ == theName)
            return children_[i];
    }
    return 0;
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

