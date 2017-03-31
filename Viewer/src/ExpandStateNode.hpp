//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef EXPANDSTATENODE_HPP
#define EXPANDSTATENODE_HPP

#include <string>
#include <vector>

class ExpandStateNode
{
    friend class TreeNodeView;
    friend class CompactNodeView;

public:
    explicit ExpandStateNode(const std::string& name) : name_(name) {}
    ExpandStateNode() : name_("") {}
    ~ExpandStateNode();

    void clear();
    ExpandStateNode* add(const std::string&);

    std::vector<ExpandStateNode*> children_;
    std::string name_;

};

#endif // EXPANDSTATENODE_HPP
