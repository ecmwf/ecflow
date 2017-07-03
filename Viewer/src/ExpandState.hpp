//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef EXPANDSTATE_HPP_
#define EXPANDSTATE_HPP_

#include <string>
#include <vector>

#include <QPersistentModelIndex>
#include <QSet>

class TreeNodeModel;
class AbstractNodeView;
class QModelIndex;
class VNode;
class VTreeNode;
class ExpandStateNode;

class ExpandState
{
    friend class TreeNodeView;
    friend class CompactNodeView;

public:
    ExpandState(AbstractNodeView*,TreeNodeModel*);
    ~ExpandState();

    bool rootSameAs(const std::string&) const;
    void save(const VTreeNode*);
    void collectExpanded(const VTreeNode* node,QSet<QPersistentModelIndex>&);


protected:
	void clear();
    ExpandStateNode* setRoot(VNode* root,bool expanded);
    ExpandStateNode* root() const {return root_;}
    void save(const VNode *,ExpandStateNode*,const QModelIndex&);
    void collectExpanded(ExpandStateNode *expand,const VTreeNode* node,
                 const QModelIndex& nodeIdx,QSet<QPersistentModelIndex>& theSet);

    AbstractNodeView* view_;
    TreeNodeModel* model_;
    ExpandStateNode* root_;
};

#endif



