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
class NodeViewBase;
class QModelIndex;
class VNode;
class VTreeNode;
class ExpandStateNode;

//template <typename View> class ExpandState;

#if 0
class CompactViewExpandState
{
    friend class CompactNodeView;

public:
    explicit CompactViewExpandState(CompactNodeView*,TreeNodeModel*);
    ~CompactViewExpandState();

    bool rootSameAs(const std::string&) const;
    void save(const VTreeNode*);
    void restore(const VTreeNode*);

    void collectExpanded(const VTreeNode* node,QSet<QPersistentModelIndex>&);

protected:
    void clear();
    ExpandStateNode* setRoot(const std::string&);
    ExpandStateNode* root() const {return root_;}
    void save(ExpandStateNode*,const QModelIndex&);
    //void restore(ExpandStateNode*,const VTreeNode*);
    void collectExpanded(ExpandStateNode *expand,const VTreeNode* node,
                 const QModelIndex& nodeIdx,QSet<QPersistentModelIndex>& theSet);

    CompactNodeView* view_;
    TreeNodeModel* model_;
    ExpandStateNode* root_;
};

#endif

class ExpandState
{
    friend class TreeNodeView;
    friend class CompactNodeView;

public:
    ExpandState(NodeViewBase*,TreeNodeModel*);
    ~ExpandState();

    bool rootSameAs(const std::string&) const;
    void save(const VTreeNode*);
    void restore(const VTreeNode*);
    void collectExpanded(const VTreeNode* node,QSet<QPersistentModelIndex>&);


protected:
	void clear();
    ExpandStateNode* setRoot(VNode* root,bool expanded);
    ExpandStateNode* root() const {return root_;}
    void save(const VNode *,ExpandStateNode*,const QModelIndex&);
    void restore(const VNode *,ExpandStateNode*);
    void collectExpanded(ExpandStateNode *expand,const VTreeNode* node,
                 const QModelIndex& nodeIdx,QSet<QPersistentModelIndex>& theSet);

    NodeViewBase* view_;
    TreeNodeModel* model_;
    ExpandStateNode* root_;
};


#endif



