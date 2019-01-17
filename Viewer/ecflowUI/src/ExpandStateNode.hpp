//============================================================================
// Copyright 2009-2019 ECMWF.
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

//Store the expanded state of a node between major updates, server clears,
//status filter changes etc... Each VNode is represented by an ExpandStateNode!
//Note: for memory efficiency we do not store the parent!!!
class ExpandStateNode
{
    friend class ExpandState;

public:
    explicit ExpandStateNode(const VNode* node,bool);
    ~ExpandStateNode();

    void reset(const VNode* node,bool expanded);
    void clear();
    ExpandStateNode* setChildAt(std::size_t index,VNode* node,bool
                                expanded);
    void setExpanded(bool expanded) {expanded_=expanded;}
    void setExpandedRecursively(unsigned int expanded);
    void setExpandedAll();
    void setCollapsedAll();

    ExpandStateNode* find(const std::vector<std::string>& pathVec);
    ExpandStateNode* findChild(const std::string& name) const;
    ExpandStateNode* findChild(const std::string& name,std::size_t& pos) const;
    bool adjustContents(const VNode* node);
    void print(std::string& indent,bool recursive) const;

protected:
    void reserveChildren(std::size_t num);

    std::vector<ExpandStateNode*> children_;
    std::string name_;

    //Set if the node is expanded
    unsigned int expanded_: 1;

    //Set if "Expand all children" was called on the node.
    //Cleared when "collaps all children" is called on this node or
    //on an ancestor
    unsigned int expandedAll_ : 1;

    //Set if "Collaps all children" was called on the node
    //Cleared when "expand all children" is called on this node or
    //on an ancestor
    unsigned int collapsedAll_ : 1;
};

#endif // EXPANDSTATENODE_HPP
