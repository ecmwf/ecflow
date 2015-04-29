//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef EXPANDNODE_HPP_
#define EXPANDNODE_HPP_

#include <string>
#include <vector>

#include "VInfo.hpp"

class ExpandNode
{
	friend class TreeNodeView;

public:
	ExpandNode(const std::string& name) : name_(name) {}
	ExpandNode() : name_("") {}
	~ExpandNode();

	void clear();
	ExpandNode* add(const std::string&);

	std::vector<ExpandNode*> children_;
	std::string name_;

};

class ExpandState
{
	friend class TreeNodeView;

public:
	ExpandState() : root_(0) {}
	~ExpandState();

	void clear();
	ExpandNode* setRoot(const std::string&);
	ExpandNode* root() const {return root_;}

	ExpandNode* root_;
};

#endif



