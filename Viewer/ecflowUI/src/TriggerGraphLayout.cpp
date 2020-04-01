//============================================================================
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TriggerGraphLayout.hpp"

#include <algorithm>

#include "TriggerGraphView.hpp"
#include "TriggerGraphDelegate.hpp"
#include "TriggerGraphLayoutBuilder.hpp"
#include "VNode.hpp"
#include "VSettings.hpp"

#include <QModelIndex>

//=============================================================
//
// TriggerGraphCollectors
//
//=============================================================

class TriggerRelationCollector : public TriggerCollector
{
public:
    TriggerRelationCollector(VItem* node, TriggerGraphLayout* layout, bool e) :
       node_(node), layout_(layout), e_(e) {}

    bool scanParents() override { return e_; }
    bool scanKids() override { return e_; }

    bool add(VItem* n, VItem* p,Mode mode) override {
        layout_->addRelation(n, node_, p, mode, n);
        n_++;
        return true;
    }

private:
    int n_ {0};
    VItem* node_;
    TriggerGraphLayout* layout_;
    bool e_;
};

int TriggerGraphLayoutNode::width() const
{
    return (grItem_)?grItem_->boundingRect().width():0;
}

int TriggerGraphLayoutNode::height() const
{
    return (grItem_)?grItem_->boundingRect().height():0;
}

GraphLayoutNode* TriggerGraphLayoutNode::cloneGraphNode()
{
    GraphLayoutNode* n = new GraphLayoutNode(width(), height());
    for(auto p: parents_)
        n->parents_.emplace_back(p->index_);

    for(auto ch: children_)
        n->children_.emplace_back(ch->index_);

    return n;
}

void TriggerGraphLayoutNode::addRelation(TriggerGraphLayoutNode* o)
{
    for(auto ch: children_) {
        if (ch == o) {
            return;
        }
    }

    children_.push_back(o);
    o->parents_.push_back(this);
}

void TriggerGraphLayoutNode::addGrNode(const QModelIndex& idx, TriggerGraphDelegate* delegate)
{
    if (!grItem_) {
        grItem_ = new TriggerGraphNodeItem(idx, delegate, -1, nullptr);
    } else {
        grItem_->setIndex(idx);
    }
}

void TriggerGraphLayoutNode::adjustGrPos(int x, int y, QGraphicsScene* scene)
{
    assert(grItem_);
    grItem_->setPos(x, y);
    if (!grItem_->scene())
        scene->addItem(grItem_);
}

//=============================================================
//
// TriggerGraphLayout
//
//=============================================================

TriggerGraphLayout::TriggerGraphLayout(TriggerGraphView* view) :
    view_(view)
{
    builder_ = new SimpleGraphLayoutBuilder();
}

TriggerGraphLayout::~TriggerGraphLayout()
{
    delete builder_;
}

void TriggerGraphLayout::clear()
{
    view_->scene()->clear();
    for(auto n: nodes_) {
        delete n;
    }
    nodes_.clear();

    for(auto n: edges_) {
        delete n;
    }
    edges_.clear();
}

void TriggerGraphLayout::scan(VNode* node, bool dependency)
{
    Q_ASSERT(node);
    dependency_ = dependency;s
    scanOne(node);

//    if(VNode *p = node->parent()) {
//        addRelation(p, node, nullptr, TriggerCollector::Hierarchy, nullptr);
//        scanOne(p);
//    }
}

void TriggerGraphLayout::scanOne(VNode* node)
{
    Q_ASSERT(node);

    // add the node to the layout
    addNode(node);

    // performs the trigger collection - it will populate the layout
    TriggerRelationCollector tc(node, this, dependency_);
    node->triggers(&tc);
}

void TriggerGraphLayout::build()
{
    std::vector<GraphLayoutNode*> lnodes;
    for (auto n: nodes_) {
        lnodes.emplace_back(n->cloneGraphNode());
    }

    builder_->build(lnodes);

    for (size_t i=0; i < nodes_.size(); i++) {
        nodes_[i]->adjustGrPos(lnodes[i]->x_, lnodes[i]->y_, view_->scene());
        delete lnodes[i];
    }
    lnodes.clear();

//    for(auto e: edges_) {
//        view_->scene()->removeItem(e->grItem_);
//        delete e->grItem_;
//    }
    //edges_.clear();

    for(auto e: edges_) {
        TriggerGraphEdgeItem* itemE =
                new TriggerGraphEdgeItem(nodes_[e->from_]->grNode(), nodes_[e->to_]->grNode(), e->mode_, nullptr);

        e->grItem_ = itemE;
        view_->setEdgePen(itemE);
        //edges_.push_back(itemE);
        view_->scene()->addItem(itemE);
    }
}


TriggerGraphLayoutNode* TriggerGraphLayout::addNode(VItem* item)
{
    if (!item)
        return nullptr;

    for(auto n: nodes_) {
        if (n->item() == item) {
            return n;
        }
    }

    TriggerGraphLayoutNode* n =new TriggerGraphLayoutNode(nodes_.size(), item);
    nodes_.push_back(n);
    return n;
}

TriggerGraphLayoutEdge* TriggerGraphLayout::addEdge(
        int from, int to,
        VItem* through, TriggerCollector::Mode mode, VItem *trigger)
{
    for(auto e: edges_) {
        if (e->sameAs(from, to, through, mode, trigger))
            return e;
    }

    TriggerGraphLayoutEdge* e =new TriggerGraphLayoutEdge(from, to, through, mode, trigger);
    edges_.push_back(e);
    return e;
}

void TriggerGraphLayout::addRelation(VItem* from, VItem* to,
                VItem* through, TriggerCollector::Mode mode, VItem *trigger)
{
    TriggerGraphLayoutNode* from_n = addNode(from);
    TriggerGraphLayoutNode* to_n = addNode(to);
    Q_ASSERT(from_n);
    Q_ASSERT(to_n);

    from_n->addRelation(to_n);

    //edge
    TriggerGraphLayoutEdge* edge = addEdge(from_n->index(), to_n->index(), through, mode, trigger);
}
