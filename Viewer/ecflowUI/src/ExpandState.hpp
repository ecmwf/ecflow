/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ExpandState_HPP
#define ecflow_viewer_ExpandState_HPP

#include <string>
#include <vector>

#include <QPersistentModelIndex>
#include <QSet>

// Represents the expand state of a full/part VNode tree. This tree has the same structure
// as a VNode tree and each VNode object is represented by a ExpandStateNode object.

class TreeNodeModel;
class AbstractNodeView;
class QModelIndex;
class VNode;
class ExpandStateNode;

class ExpandState {
    friend class TreeNodeView;
    friend class CompactNodeView;

public:
    ExpandState(AbstractNodeView*, TreeNodeModel*);
    ~ExpandState();

    void save(const VNode*);
    void collectExpanded(const VNode* node, QSet<QPersistentModelIndex>&);
    void saveExpandAll(const VNode* node);
    void saveCollapseAll(const VNode* node);
    void print() const;
    bool isEmpty() const;
    void updateServerName(const std::string& serverName);

protected:
    void init(const VNode* vnode);
    void clear();
    ExpandStateNode* root() const { return root_; }
    void save(const VNode*, ExpandStateNode*, const QModelIndex&);
    void collectExpanded(ExpandStateNode* expand,
                         const VNode* node,
                         const QModelIndex& nodeIdx,
                         QSet<QPersistentModelIndex>& theSet);

    bool needToExpandNewChild(ExpandStateNode* expandNode, const std::string&) const;
    void collectParents(const std::string& fullPath, std::vector<ExpandStateNode*>& parents) const;

    ExpandStateNode* find(const std::string& fullPath);

    AbstractNodeView* view_;
    TreeNodeModel* model_;
    ExpandStateNode* root_;
};

#endif /* ecflow_viewer_ExpandState_HPP */
