//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ExpandState.hpp"

ExpandNode::~ExpandNode()
{
	clear();
}

void ExpandNode::clear()
{
	name_.clear();
	for(unsigned int i=0; i < children_.size(); i++)
	{
		delete children_.at(i);
	}
	children_.clear();
}

ExpandNode* ExpandNode::add(const std::string& name)
{
	ExpandNode *n=new ExpandNode(name);
	children_.push_back(n);
	return n;
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

ExpandNode* ExpandState::setRoot(const std::string& name)
{
	if(root_)
		clear();

	root_=new ExpandNode(name);
	return root_;
}



