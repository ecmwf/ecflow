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

class VNode;

//Stores the expanded state of a node between major updates, server clears,
//status filter changes etc...
class ExpandStateNode
{
    friend class ExpandState;

public:
    explicit ExpandStateNode(VNode* node,unsigned int expanded);
    ~ExpandStateNode();

    void reset(const VNode* node,unsigned int expanded);
    void clear();
    ExpandStateNode* setChildAt(std::size_t index,VNode* node,unsigned int expanded);
    void setExpanded(bool expanded) {expanded_=expanded;}

protected:
    void reserveChildren(std::size_t num);

    std::vector<ExpandStateNode*> children_;
    std::string name_;
    unsigned int expanded_ : 1;

};

#endif // EXPANDSTATENODE_HPP
