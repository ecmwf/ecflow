//============================================================================
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerGraphView.hpp"

#include "ActionHandler.hpp"
#include "AttributeEditor.hpp"
#include "PropertyMapper.hpp"
#include "TriggerGraphDelegate.hpp"
#include "TriggerGraphModel.hpp"
#include "TriggerGraphLayoutBuilder.hpp"
#include "UiLog.hpp"

#include <math.h>

#include <QScrollBar>

//=============================================================
//
// TriggerGraphCollectors
//
//=============================================================

class TriggerRelationCollector : public TriggerCollector
{
public:
    TriggerRelationCollector(VItem* node, TriggerGraphView* view, bool e) :
       node_(node), view_(view), e_(e) {}

    bool scanParents() override { return e_; }
    bool scanKids() override { return e_; }

    bool add(VItem* n, VItem* p,Mode mode) override {
        view_->addRelation(n, node_, p, mode, n);
        n_++;
        return true;
    }

private:
    int n_ {0};
    VItem* node_;
    TriggerGraphView* view_;
    bool e_;
};

//=============================================================
//
// TriggerGraphNodeItem
//
//=============================================================

TriggerGraphNodeItem::TriggerGraphNodeItem(int index, VItem* item,
                                           TriggerGraphView* view) :
    index_(index),
    item_(item),
    view_(view)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    //adjust();
}

QRectF TriggerGraphNodeItem::boundingRect() const
{
    return bRect_;
}

void TriggerGraphNodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                QWidget *widget)
{
    //Init style option
    QStyleOptionViewItem opt;

    //if(selectionModel_->isSelected(item->index))
    //    opt.state |= QStyle::State_Selected;

    //int optWidth=2000;
    //if(item->width > optWidth)
    //optWidth=item->width;
    opt.rect=bRect_.toRect(); //  QRect(item->x,yp,optWidth,item->height);

    QModelIndex idx=view_->model()->index(index_, 0);
    view_->delegate()->paint(painter,opt,idx);

    if (isSelected()) {
        painter->setPen(Qt::red);
        painter->drawRect(opt.rect.adjusted(1,1,-1,-1));
    }
}

void TriggerGraphNodeItem::adjustSize()
{
    prepareGeometryChange();

    int w, h;
    QModelIndex idx=view_->model()->index(index_, 0);
    if (idx.isValid()) {
        view_->delegate()->sizeHintCompute(idx,w,h);
        bRect_ = QRectF(QPointF(0, 0), QSize(w, h));
    }
}

void TriggerGraphNodeItem::addRelation(TriggerGraphNodeItem* o)
{
    for(auto ch: children_) {
        if (ch == o) {
            return;
        }
    }

    children_.push_back(o);
    o->parents_.push_back(this);
}

//void TriggerGraphNodeNode::addGrNode(const QModelIndex& idx, TriggerGraphDelegate* delegate)
//{
//    if (!grItem_) {
//        grItem_ = new TriggerGraphNodeItem(idx, delegate, -1, nullptr);
//    } else {
//        grItem_->setIndex(idx);
//    }
//}

void TriggerGraphNodeItem::adjustPos(int x, int y)
{
    setPos(x, y);
    if (!scene())
        view_->scene()->addItem(this);
}

GraphLayoutNode* TriggerGraphNodeItem::toGraphNode()
{
    GraphLayoutNode* n = new GraphLayoutNode(bRect_.width(), bRect_.height());
    for(auto p: parents_)
        n->parents_.emplace_back(p->index_);

    for(auto ch: children_)
        n->children_.emplace_back(ch->index_);

    return n;
}



//=============================================================
//
// TriggerGraphEdgeItem
//
//=============================================================

TriggerGraphEdgeItem::TriggerGraphEdgeItem(
        TriggerGraphNodeItem* from, TriggerGraphNodeItem* to,
        VItem* through, TriggerCollector::Mode mode,
        VItem* trigger, TriggerGraphView* view) :
    from_(from), to_(to), through_(through), mode_(mode),
    trigger_(trigger), view_(view)
{
    //adjust();
}

void TriggerGraphEdgeItem::adjust()
{
    prepareGeometryChange();

    int gap = 3;
    QPainterPath p;
    QRectF srcRect = from_->mapRectToParent(from_->boundingRect());
    QRectF targetRect = to_->mapRectToParent(to_->boundingRect());

    QPointF p1(targetRect.left() - srcRect.right() - 2*gap,
               targetRect.center().y() - srcRect.center().y());
    QPointF c1(p1.x()/2, 0);
    QPointF c2(p1.x()/2, targetRect.center().y() - srcRect.center().y());

    //p.lineTo(p1);
    p.cubicTo(c1, c2, p1);

    // add arrow to mid-point of curve
    auto poly = p.toSubpathPolygons();
    if (poly.count() == 1) {
        if (poly[0].count() > 3) {
            int n = poly[0].count()/2-1;
            double angle = atan2(poly[0].at(n+1).y() - poly[0].at(n).y(),
                    poly[0].at(n+1).x() - poly[0].at(n).x());
            //define triangle
            QPolygonF tri;
            tri << QPointF(0.,-arrowHeight_/2) <<
                QPointF(arrowWidth_, 0.) <<
                QPointF(0.,arrowHeight_/2);

            //rotate + translate triangle
            for(int i=0; i < tri.count(); i++) {
                tri[i] = QPointF(
                            tri[i].x() * cos(angle) - tri[i].y() * sin(angle),
                            tri[i].x() * sin(angle) + tri[i].y() * cos(angle));

                tri[i] += poly[0].at(n);
            }

            p.addPolygon(tri);
        }
    }

    view_->setEdgePen(this);

    setPos(QPointF(srcRect.right() + gap, srcRect.center().y()));

    setPath(p);

    if (!scene())
        view_->scene()->addItem(this);
}

//QRectF TriggerGraphEdgeItem::boundingRect() const
//{
//    return bRect_;
//}


TriggerGraphScene::TriggerGraphScene(QWidget* parent) :
    QGraphicsScene(parent)
{
}

//===========================================================
//
// TriggerGraphView
//
//===========================================================

TriggerGraphView::TriggerGraphView(QWidget* parent) : QGraphicsView(parent)
{
    scene_ = new TriggerGraphScene(this);
    setScene(scene_);

    model_ = new TriggerGraphModel(TriggerGraphModel::TriggerMode,this);
    delegate_ = new TriggerGraphDelegate(this);
    builder_ = new SimpleGraphLayoutBuilder();

    actionHandler_=new ActionHandler(this,this);

    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setRenderHints(QPainter::Antialiasing);

    setContextMenuPolicy(Qt::CustomContextMenu);

    //Context menu
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(slotContextMenu(const QPoint&)));

    //Properties
    std::vector<std::string> propVec;
    propVec.emplace_back("view.trigger.background");
    propVec.emplace_back("view.trigger.parentConnectorColour");
    propVec.emplace_back("view.trigger.triggerConnectorColour");
    propVec.emplace_back("view.trigger.dependencyConnectorColour");

    prop_=new PropertyMapper(propVec,this);

    VProperty *prop=nullptr;
    std::string propName;

    //init
    adjustBackground(prop_->find("view.trigger.background", true));
    adjustParentConnectColour(prop_->find("view.trigger.parentConnectorColour", true));
    adjustTriggerConnectColour(prop_->find("view.trigger.triggerConnectorColour", true));
    adjustDepConnectColour(prop_->find("view.trigger.dependencyConnectorColour", true));
}

TriggerGraphView::~TriggerGraphView()
{
    delete actionHandler_;
    delete prop_;
    delete builder_;
    clear();
}

void TriggerGraphView::clear()
{
    model_->clearData();
    nodes_.clear();
    edges_.clear();
    scene()->clear();
}

void TriggerGraphView::setInfo(VInfo_ptr info)
{
    if (info) {

    }
}

TriggerGraphNodeItem* TriggerGraphView::nodeItemAt(QPointF scenePos) const
{
    if (QGraphicsItem* item = scene()->itemAt(scenePos, QTransform())) {
        if (item->type() == TriggerGraphNodeItem::Type) {
            return static_cast<TriggerGraphNodeItem*>(item);
        }
    }
    return nullptr;
}

//QModelIndex TriggerGraphView::indexAt(QPointF scenePos) const
//{
//    QGraphicsItem* item = scene()->itemAt(scenePos, QTransform());
//    return itemToIndex(item);
//}

//QModelIndex TriggerGraphView::itemToIndex(QGraphicsItem* item) const
//{
//    if (item && item->type() == TriggerGraphNodeItem::Type) {
//        TriggerGraphNodeItem* nItem = static_cast<TriggerGraphNodeItem*>(item);
//        return nItem->index();
//    }
//    return {};
//}

//TriggerGraphNodeItem* TriggerGraphView::indexToItem(const QModelIndex& index) const
//{
//    if (!index.isValid())
//        return nullptr;

//    Q_FOREACH(QGraphicsItem* item, items()) {
//        if(item->type() == TriggerGraphNodeItem::Type) {
//            TriggerGraphNodeItem* nItem = static_cast<TriggerGraphNodeItem*>(item);
//            if (index == nItem->index()) {
//                return nItem;
//            }
//       }
//    }

//    return nullptr;
//}

//QModelIndexList TriggerGraphView::selectedIndexes()
//{
//    Q_ASSERT(model_);
//    QModelIndexList lst;

//    Q_FOREACH(QGraphicsItem* item, items()) {
//        if (item->isSelected()) {
//            QModelIndex index = itemToIndex(item);
//            if (index.isValid())
//                lst << index;
//        }
//    }

//    return lst;
//}

void TriggerGraphView::slotContextMenu(const QPoint& position)
{
    auto itemClicked = nodeItemAt(mapToScene(position));

    QPoint scrollOffset(horizontalScrollBar()->value(), verticalScrollBar()->value());
    QPoint globalPos = mapToGlobal(position); //, position + scrollOffset;

    //handleContextMenu(indexAt(position),lst,mapToGlobal(position),position+scrollOffset,this);
    //handleContextMenu(index, lst, mapToGlobal(position), position + scrollOffset, this);

    //Node actions
    if(itemClicked->item())   //indexLst[0].isValid() && indexLst[0].column() == 0)
    {
        std::vector<TriggerGraphNodeItem*> itemLst; //selectedIndexes();
        itemLst.push_back(itemClicked);

        std::vector<VInfo_ptr> nodeLst;
            for(auto n: itemLst) {
                VInfo_ptr info = VInfo::createFromItem(n->item());
                if(info && !info->isEmpty())
                    nodeLst.push_back(info);
            }

        actionHandler_->contextMenu(nodeLst,globalPos);
    }
}

void TriggerGraphView::slotCommandShortcut()
{

}

void TriggerGraphView::slotViewCommand(VInfo_ptr info,QString cmd)
{
    if(cmd == "lookup")
    {
        //Q_EMIT linkSelected(info);
    }

    else if(cmd ==  "edit")
    {
        if(info && info->isAttribute())
        {
            AttributeEditor::edit(info,this);
        }
    }
}

void TriggerGraphView::adjustBackground(VProperty *p)
{
    if (!p)
        p = prop_->find("view.trigger.background", true);

    if(p) {
        QColor col = p->value().value<QColor>();
        if (col.isValid()) {
            setBackgroundBrush(col);
        }
    }
}

void TriggerGraphView::adjustParentConnectColour(VProperty *p)
{
    if (!p)
        p = prop_->find("view.trigger.parentConnectorColour", true);

    if(p) {
        QColor col = p->value().value<QColor>();
        if (col.isValid()) {
            parentConnectPen_ = QPen(col);
        }
    }
}

void TriggerGraphView::adjustTriggerConnectColour(VProperty *p)
{
    if (!p)
        p = prop_->find("view.trigger.triggerConnectorColour", true);

    if(p) {
        QColor col = p->value().value<QColor>();
        if (col.isValid()) {
            triggerConnectPen_ = QPen(col);
        }
    }
}

void TriggerGraphView::adjustDepConnectColour(VProperty *p)
{
    if (!p)
        p = prop_->find("view.trigger.dependencyConnectorColour", true);

    if(p) {
        QColor col = p->value().value<QColor>();
        if (col.isValid())  {
            depConnectPen_ = QPen(col);
        }
    }
}

void TriggerGraphView::adjustSceneRect()
{
    QRectF r = scene()->itemsBoundingRect();
    r.setTopLeft(QPointF(0, 0));
    r.setBottom(r.bottom() + 10);
    scene()->setSceneRect(r);
}

void TriggerGraphView::notifyChange(VProperty* p)
{
    if(p->path() == "view.trigger.background") {
        adjustBackground(p);
    } else if(p->path() == "view.trigger.parentConnectorColour") {
        adjustParentConnectColour(p);
    } else if(p->path() == "view.trigger.triggerConnectorColour") {
        adjustTriggerConnectColour(p);
    } else if(p->path() == "view.trigger.depConnectorColour") {
        adjustDepConnectColour(p);
    }
}

void TriggerGraphView::nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect)
{
//    QModelIndex index = model_->nodeToIndex(node);
//    if (TriggerGraphNodeItem *item = indexToItem(index)) {
//        item->update();
//    }

}

//------------------------------------------
// Contents handling
//------------------------------------------

//void TriggerGraphView::scan()
//{
//    if (!info_ || !info_->node())
//        return;

//    VNode *node = info_->node();
//    Q_ASSERT(node);
//    scan(node);

//    UiLog().dbg() << model_->rowCount() << layout_->nodes_.size();

//    if(VNode *p = node->parent()) {
//        layout_->addRelation(p, node, nullptr, TriggerCollector::Hierarchy, nullptr);
//        scan(p);
//    }
//}

void TriggerGraphView::scan(VNode* node, bool dependency)
{
    Q_ASSERT(node);

    dependency_ = dependency;

    scanOne(node);
    model_->setItems(nodes_);
    for(auto n: nodes_) {
        n->adjustSize();
    }

    buildLayout();
    adjustSceneRect();

#if 0
    // add the node to the layout
    layout_->addNode(node);

    // performs the trigger collection - it will populate the layout
    TriggerRelationCollector tc(node, layout_, false);
    node->triggers(&tc);

//    std::vector<VItem*> items;
//    for(size_t i=oriNum; i < layout_->nodes_.size(); i++) {
//        items.push_back(layout_->nodes_[i]->item_);
//    }

    model_->setItems(layout_->nodes());

    for(int i=0; i < model_->rowCount(); i++) {
        layout_->nodes()[i]->addGrItem(model_->index(i,0), delegate_);
    }

    layout_->build();
    //layout_->postBuild();

//    for(size_t i=0; i < nodes_.size(); i++) {
//        nodes_[i]->setPos(QPoint(layout_->nodes_[i]->tmpX_, layout_->nodes_[i]->tmpY_));
//        if (i >= oriNum)
//            scene_->addItem(nodes_[i]);
//    }

//    for(size_t i=0; i < edges_.size(); i++) {
//        scene_->removeItem(edges_[i]);
//        delete edges_[i];
//    }
//    edges_.clear();

//    for(auto e: layout_->edges_) {
//        TriggerGraphEdgeItem* itemE =new TriggerGraphEdgeItem(nodes_[e->from_], nodes_[e->to_], nullptr);
//        if (e->mode_ == TriggerCollector::Normal) {
//            itemE->setPen(ui_->view->triggerConnectPen_);
//        } else if(e->mode_ == TriggerCollector::Hierarchy) {
//            itemE->setPen(ui_->view->parentConnectPen_);
//        } else {
//            itemE->setPen(ui_->view->depConnectPen_);
//        }
//        edges_.push_back(itemE);
//        scene_->addItem(itemE);
//    }

    adjustSceneRect();
#endif
}


//void TriggerGraphView::scanOne(VNode* node)
//{
//    Q_ASSERT(node);
//    dependency_ = dependency;
//    scanOne(node);

////    if(VNode *p = node->parent()) {
////        addRelation(p, node, nullptr, TriggerCollector::Hierarchy, nullptr);
////        scanOne(p);
////    }
//}

void TriggerGraphView::scanOne(VNode* node)
{
    Q_ASSERT(node);

    // add the node to the layout
    addNode(node);

    // performs the trigger collection - it will populate the layout
    TriggerRelationCollector tc(node, this, dependency_);
    node->triggers(&tc);
}

void TriggerGraphView::buildLayout()
{
    std::vector<GraphLayoutNode*> lnodes;
    for (auto n: nodes_) {
        lnodes.emplace_back(n->toGraphNode());
    }

    builder_->build(lnodes);

    for (size_t i=0; i < nodes_.size(); i++) {
        nodes_[i]->adjustPos(lnodes[i]->x_, lnodes[i]->y_);
        delete lnodes[i];
    }
    lnodes.clear();

//    for(auto e: edges_) {
//        view_->scene()->removeItem(e->grItem_);
//        delete e->grItem_;
//    }
    //edges_.clear();

    for(auto e: edges_) {
        e->adjust();
        //TriggerGraphEdgeItem* itemE =
        //        new TriggerGraphEdgeItem(nodes_[e->from_]->grNode(), nodes_[e->to_]->grNode(), e->mode_, nullptr);

//        e->grItem_ = itemE;
//        view_->setEdgePen(itemE);
//        //edges_.push_back(itemE);
//        view_->scene()->addItem(itemE);
    }
}


TriggerGraphNodeItem* TriggerGraphView::addNode(VItem* item)
{
    if (!item)
        return nullptr;

    for(auto n: nodes_) {
        if (n->item() == item) {
            return n;
        }
    }

    auto n =new TriggerGraphNodeItem(nodes_.size(), item, this);
    nodes_.push_back(n);
    return n;
}

TriggerGraphEdgeItem* TriggerGraphView::addEdge(
        TriggerGraphNodeItem* from, TriggerGraphNodeItem* to,
        VItem* through, TriggerCollector::Mode mode, VItem *trigger)
{
    for(auto e: edges_) {
        if (e->sameAs(from, to, through, mode, trigger))
            return e;
    }

    auto e =new TriggerGraphEdgeItem(from, to, through, mode, trigger, this);
    edges_.push_back(e);
    return e;
}

void TriggerGraphView::addRelation(VItem* from, VItem* to,
                VItem* through, TriggerCollector::Mode mode, VItem *trigger)
{
    TriggerGraphNodeItem* from_n = addNode(from);
    TriggerGraphNodeItem* to_n = addNode(to);
    Q_ASSERT(from_n);
    Q_ASSERT(to_n);

    from_n->addRelation(to_n);

    //edge
    auto edge = addEdge(from_n, to_n, through, mode, trigger);
}

void TriggerGraphView::setEdgePen(TriggerGraphEdgeItem* e)
{
    Q_ASSERT(e);
    switch (e->mode()) {
        case TriggerCollector::Normal:
            e->setPen(triggerConnectPen_);
            break;
        case TriggerCollector::Hierarchy:
            e->setPen(parentConnectPen_);
            break;
        default:
            e->setPen(depConnectPen_);
    }
}
