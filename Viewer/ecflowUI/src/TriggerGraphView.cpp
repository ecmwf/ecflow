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
#include "TriggerGraphLayout.hpp"
#include "TriggerGraphModel.hpp"
#include "UiLog.hpp"

#include <QScrollBar>

TriggerGraphNodeItem::TriggerGraphNodeItem(const QModelIndex& index,
                                           TriggerGraphDelegate* delegate, int col, QGraphicsItem* parent) :
    QGraphicsItem(parent),
    index_(index),
    delegate_(delegate),
    col_(col)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    adjust();
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

    delegate_->paint(painter,opt,index_);

    if (isSelected()) {
        painter->setPen(Qt::red);
        painter->drawRect(opt.rect.adjusted(1,1,-1,-1));
    }
}


void TriggerGraphNodeItem::adjust()
{
    prepareGeometryChange();

    int w, h;
    QModelIndex idx=index_;
    delegate_->sizeHintCompute(idx,w,h);

    QString name  = index_.data(Qt::DisplayRole).toString();
    bRect_        = QRectF(QPointF(0, 0), QSize(w, h));
    //QPoint refPos = index_.data(MvQFolderModel::PositionRole).toPoint();
    if (col_ == 0)
        setPos(600, 25);
    else
        setPos(150, 40 + index_.row()*50);
}


TriggerGraphEdgeItem::TriggerGraphEdgeItem(QGraphicsItem* srcItem, QGraphicsItem* targetItem,
                                           TriggerCollector::Mode mode, QGraphicsItem* parent) :
    QGraphicsPathItem(parent),
    srcItem_(srcItem),
    targetItem_(targetItem),
    mode_(mode)
{
    adjust();
}

void TriggerGraphEdgeItem::adjust()
{
    prepareGeometryChange();

    int gap = 3;
    QPainterPath p;
    QRectF srcRect = srcItem_->mapRectToParent(srcItem_->boundingRect());
    QRectF targetRect = targetItem_->mapRectToParent(targetItem_->boundingRect());

    QPointF p1(targetRect.left() - srcRect.right() - 2*gap,
               targetRect.center().y() - srcRect.center().y());
    QPointF c1(p1.x()/2, 0);
    QPointF c2(p1.x()/2, targetRect.center().y() - srcRect.center().y());

    //p.lineTo(p1);
    p.cubicTo(c1, c2, p1);

    //bRect_        = QRectF(QPointF(0, 0), QSize(300,50));
    //QPoint refPos = index_.data(MvQFolderModel::PositionRole).toPoint();
    setPos(QPointF(srcRect.right() + gap, srcRect.center().y()));


    setPath(p);
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
    layout_ = new TriggerGraphLayout(this);

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
}

void TriggerGraphView::clear()
{
    model_->clearData();
    layout_->clear();
}

void TriggerGraphView::setInfo(VInfo_ptr info)
{
    if (info) {

    }
}

QModelIndex TriggerGraphView::indexAt(QPointF scenePos) const
{
    QGraphicsItem* item = scene()->itemAt(scenePos, QTransform());
    return itemToIndex(item);
}

QModelIndex TriggerGraphView::itemToIndex(QGraphicsItem* item) const
{
    if (item && item->type() == TriggerGraphNodeItem::Type) {
        TriggerGraphNodeItem* nItem = static_cast<TriggerGraphNodeItem*>(item);
        return nItem->index();
    }
    return {};
}

TriggerGraphNodeItem* TriggerGraphView::indexToItem(const QModelIndex& index) const
{
    if (!index.isValid())
        return nullptr;

    Q_FOREACH(QGraphicsItem* item, items()) {
        if(item->type() == TriggerGraphNodeItem::Type) {
            TriggerGraphNodeItem* nItem = static_cast<TriggerGraphNodeItem*>(item);
            if (index == nItem->index()) {
                return nItem;
            }
       }
    }

    return nullptr;
}

QModelIndexList TriggerGraphView::selectedIndexes()
{
    Q_ASSERT(model_);
    QModelIndexList lst;

    Q_FOREACH(QGraphicsItem* item, items()) {
        if (item->isSelected()) {
            QModelIndex index = itemToIndex(item);
            if (index.isValid())
                lst << index;
        }
    }

    return lst;
}

void TriggerGraphView::slotContextMenu(const QPoint& position)
{
    QModelIndex indexClicked = indexAt(mapToScene(position));
    QPoint scrollOffset(horizontalScrollBar()->value(), verticalScrollBar()->value());
    QPoint globalPos = mapToGlobal(position); //, position + scrollOffset;

    //handleContextMenu(indexAt(position),lst,mapToGlobal(position),position+scrollOffset,this);
    //handleContextMenu(index, lst, mapToGlobal(position), position + scrollOffset, this);

    //Node actions
    if(indexClicked.isValid())   //indexLst[0].isValid() && indexLst[0].column() == 0)
    {
        QModelIndexList indexLst; //selectedIndexes();
        indexLst << indexClicked;
        std::vector<VInfo_ptr> nodeLst;
            for(int i=0; i < indexLst.count(); i++)
            {
                VInfo_ptr info=model_->nodeInfo(indexLst[i]);
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
    QModelIndex index = model_->nodeToIndex(node);
    if (TriggerGraphNodeItem *item = indexToItem(index)) {
        item->update();
    }

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

void TriggerGraphView::scan(VNode* node)
{
    Q_ASSERT(node);

    layout_->scan(node);
    model_->setItems(layout_->nodes());
    for(int i=0; i < model_->rowCount(); i++) {
        layout_->nodes()[i]->addGrNode(model_->index(i,0), delegate_);
    }

    layout_->build();
    //layout_->postBuild();s
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
