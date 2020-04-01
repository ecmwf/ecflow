//============================================================================
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TRIGGERGRAPHLAYOUT_HPP
#define TRIGGERGRAPHLAYOUT_HPP

#include "TriggerCollector.hpp"

#include <vector>

class GraphLayoutBuilder;
class GraphLayoutNode;
class TriggerGraphDelegate;
class TriggerGraphNodeItem;
class TriggerGraphEdgeItem;
class TriggerGraphView;
class QModelIndex;
class QGraphicsScene;


class TriggerGraphLayoutNode {
public:
    TriggerGraphLayoutNode(int index, VItem* item) : index_(index), item_(item) {}

    bool hasParents() const {return parents_.size() > 0;}
    bool hasChildren() const {return children_.size() > 0;}
    bool hasManagedParent() const;
    bool hasManagedChild() const;
    void addRelation(TriggerGraphLayoutNode* o);

    int index() const {return index_;}
    VItem* item() const {return item_;}

    TriggerGraphNodeItem* grNode() const {return grItem_;}
    void addGrNode(const QModelIndex& idx, TriggerGraphDelegate* delegate);
    void adjustGrPos(int x, int y, QGraphicsScene* scene);
    GraphLayoutNode* cloneGraphNode();
    int width() const;
    int height() const;

protected:
    std::vector<TriggerGraphLayoutNode*> parents_;
    std::vector<TriggerGraphLayoutNode*> children_;
    int index_;
    VItem* item_;
    TriggerGraphNodeItem* grItem_ {nullptr};
};


class TriggerGraphLayoutEdge
{
public:
    TriggerGraphLayoutEdge(
        int from, int to, VItem* through, TriggerCollector::Mode mode, VItem* trigger) :
        from_(from), to_(to), through_(through), mode_(mode), trigger_(trigger) {}

    bool sameAs(int from, int to, VItem* through, TriggerCollector::Mode mode,
                VItem* trigger) const {
        return (from == from_ && to_ == to && through == through_ &&
                mode == mode_ && trigger == trigger_);
    }

    int from_;
    int to_;
    VItem* through_;
    TriggerCollector::Mode mode_;
    VItem* trigger_;
    TriggerGraphEdgeItem* grItem_;
};

class TriggerGraphLayout
{
    friend class TriggerRelationCollector;

public:
    TriggerGraphLayout(TriggerGraphView* view);
    ~TriggerGraphLayout();

    void clear();
    void scan(VNode* node, bool dependency);
    void build();
    const std::vector<TriggerGraphLayoutNode*> nodes() const {return nodes_;}
    bool dependency() const {return dependency_;}
    void setTriggeredScanner(TriggeredScanner* scanner) {triggeredScanner_=scanner;}

protected:
    void scanOne(VNode* node);
    void addRelation(VItem* from, VItem* to,
                     VItem* through, TriggerCollector::Mode mode, VItem *trigger);
    TriggerGraphLayoutNode* addNode(VItem* item);
    TriggerGraphLayoutEdge* addEdge(
            int from, int to,
            VItem* through, TriggerCollector::Mode mode, VItem *trigger);

    GraphLayoutBuilder* builder_;
    std::vector<TriggerGraphLayoutNode*> nodes_;
    std::vector<TriggerGraphLayoutEdge*> edges_;
    TriggerGraphView* view_;

    bool dependency_ {false};
    TriggeredScanner *triggeredScanner_ {nullptr};

};

#endif // TRIGGERGRAPHLAYOUT_HPP
