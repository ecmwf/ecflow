//============================================================================
// Copyright 2009-2017 ECMWF.
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

#include <QList>

#include "VInfo.hpp"

class TreeNodeModel;
class QModelIndex;
class QTreeView;
class VTreeNode;

class ExpandStateNode
{
	friend class TreeNodeView;

public:
    explicit ExpandStateNode(const std::string& name) : name_(name) {}
    ExpandStateNode() : name_("") {}
    ~ExpandStateNode();

	void clear();
    ExpandStateNode* add(const std::string&);

    std::vector<ExpandStateNode*> children_;
	std::string name_;

};

class ExpandStateTree
{
	friend class TreeNodeView;

public:
    explicit ExpandStateTree(QTreeView*,TreeNodeModel*);
    ~ExpandStateTree();

    bool rootSameAs(const std::string&) const;
    void save(const VTreeNode*);
    void restore(const VTreeNode*);

protected:
	void clear();
    ExpandStateNode* setRoot(const std::string&);
    ExpandStateNode* root() const {return root_;}
    void save(ExpandStateNode*,const QModelIndex&);
    void restore(ExpandStateNode*,const VTreeNode*);

    QTreeView* view_;
    TreeNodeModel* model_;
    ExpandStateNode* root_;
};

class ExpandState
{
    friend class TreeNodeView;

public:
    ExpandState(QTreeView*,TreeNodeModel*);
    ~ExpandState();
    ExpandStateTree* add();
    void remove(ExpandStateTree*);
    void clear();
    QList<ExpandStateTree*> items() const {return items_;}

protected:
    QTreeView* view_;
    TreeNodeModel* model_;
    VInfo_ptr selection_;
    QList<ExpandStateTree*> items_;

};

#endif



