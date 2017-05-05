//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ExpandStateNode.hpp"


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
