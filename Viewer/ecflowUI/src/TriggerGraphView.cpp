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
#include "VItemPathParser.hpp"
#include "VNState.hpp"

#include <math.h>

#include <QFile>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsLinearLayout>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsProxyWidget>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QTextDocument>
#include <QToolButton>
#include <QVBoxLayout>

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

class TriggeredRelationCollector : public TriggerCollector
{
public:
    TriggeredRelationCollector(VItem* node, TriggerGraphView* view, bool e) :
       node_(node), view_(view), e_(e) {}

    bool scanParents() override { return e_; }
    bool scanKids() override { return e_; }

    bool add(VItem* n, VItem* p,Mode mode) override {
        view_->addRelation(node_, n, p, mode, n);
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
    setZValue(10.);
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

    int w=0, h=0;
    QModelIndex idx=view_->model()->index(index_, 0);
    if (idx.isValid()) {
        view_->delegate()->sizeHintCompute(idx,w,h, true);
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
     setFlags(QGraphicsItem::ItemIsSelectable);
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

    // bezier curve
    p.cubicTo(c1, c2, p1);

    // add arrow to mid-point of curve
    auto poly = p.toSubpathPolygons();
    QPolygonF tri;
    if (poly.count() == 1) {
        if (poly[0].count() > 3) {
            int n = poly[0].count()/2-1;
            double angle = atan2(poly[0].at(n+1).y() - poly[0].at(n).y(),
                    poly[0].at(n+1).x() - poly[0].at(n).x());

            //define triangle
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
        } else if (poly[0].count() == 2) {
            //define triangle
            tri << QPointF(0.,-arrowHeight_/2) <<
                QPointF(arrowWidth_, 0.) <<
                QPointF(0.,arrowHeight_/2);

            //translate triangle
            for(int i=0; i < tri.count(); i++) {
                tri[i] += (poly[0].at(0) + poly[0].at(1))/2.;
            }
        }
    }

    if (tri.count() > 0)
        p.addPolygon(tri);

    view_->setEdgePen(this);

    setPos(QPointF(srcRect.right() + gap, srcRect.center().y()));

    setPath(p);

    if (!scene())
        view_->scene()->addItem(this);
}

QVariant TriggerGraphEdgeItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        if (isSelected()) {
            view_->notifyEdgeSelected(this);
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

//=============================================================
//
// TriggerGraphEdgeInfoWidget
//
//=============================================================

TriggerGraphEdgeInfoWidget::TriggerGraphEdgeInfoWidget()
{
    setProperty("graphInfo", "1");

    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->setContentsMargins(2,2,2,2);
    vb->setSpacing(2);

    QHBoxLayout *hb = new QHBoxLayout();
    hb->addStretch(1);
    closeTb_ = new QToolButton(this);
    closeTb_->setAutoRaise(true);
    closeTb_->setIcon(QPixmap(":/viewer/favourite.svg"));
    closeTb_->setToolTip(tr("Close"));
    closeTb_->setProperty("graphInfo", "1");
    hb->addWidget(closeTb_);

    te_ = new QTextBrowser(this);
    te_->setOpenExternalLinks(false);
    te_->setOpenLinks(false);
    te_->setReadOnly(true);

    //Read css for the text formatting
    QString cssDoc;
    QFile f(":/viewer/trigger.css");
    //QTextStream in(&f);
    if(f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        cssDoc=QString(f.readAll());
    }
    f.close();
    te_->document()->setDefaultStyleSheet(cssDoc);


    vb->addLayout(hb);
    vb->addWidget(te_);
}

void TriggerGraphEdgeInfoWidget::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

//=============================================================
//
// TriggerGraphEdgeInfoProxy
//
//=============================================================

TriggerGraphEdgeInfoProxy::TriggerGraphEdgeInfoProxy(TriggerGraphView* view) :
    view_(view)
{
    w_ = new TriggerGraphEdgeInfoWidget();
    setWidget(w_);

    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    setOpacity(1.);
    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(this);
    effect->setXOffset(3.);
    effect->setYOffset(3.);
    setGraphicsEffect(effect);

    setZValue(100.);

    connect(w_->te_, SIGNAL(anchorClicked(const QUrl&)),
            this, SIGNAL(anchorClicked(const QUrl&)));

    connect(w_->closeTb_, SIGNAL(clicked()),
            this, SLOT(close()));
}

void TriggerGraphEdgeInfoProxy::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
     if (event->pos().y() < 20) {
        inDrag_ = true;
        dragDelta_ = mapToParent(event->pos()) - pos();
        return;
    } else {
        inDrag_ = false;
        QGraphicsProxyWidget::mousePressEvent(event);
    }

}

void TriggerGraphEdgeInfoProxy::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (inDrag_) {
        setPos(mapToParent(event->pos()) - dragDelta_);
    } else {
        QGraphicsProxyWidget::mouseMoveEvent(event);
    }
}

void TriggerGraphEdgeInfoProxy::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    inDrag_ = false;
    dragDelta_ = QPointF(0, 0);
    QGraphicsProxyWidget::mouseReleaseEvent(event);
}

void TriggerGraphEdgeInfoProxy::setInfo(TriggerGraphEdgeItem* e)
{
    prepareGeometryChange();
    w_->te_->setHtml(makeHtml((e)));
    setPos(e->pos());
    if(!scene()) {
        view_->scene()->addItem(this);
    }
    show();
}

QString TriggerGraphEdgeInfoProxy::makeHtml(TriggerGraphEdgeItem* e) const
{
    QString s = "<table width=\'100%\'>";

    VItem *t = e->from_->item();
    makeRow("from:", t, s);

    t = e->to_->item();
    makeRow("to:", t, s);

    s += "</table>";

    return s;
}

void TriggerGraphEdgeInfoProxy::makeRow(QString label, VItem* t, QString& s) const
{
    QString type=QString::fromStdString(t->typeName());
    QString path=QString::fromStdString(t->fullPath());
    QString anchor=QString::fromStdString(VItemPathParser::encode(t->fullPath(),t->typeName()));

    s += "<tr><td><b>" + label + "</b>  </td><td>" + type + "</td><td>";
    s += " <a href=\'" + anchor + "\'>" + path +"</a>";
    s += "</td></tr>";

}

//===========================================================
//
// TriggerGraphView
//
//===========================================================

TriggerGraphView::TriggerGraphView(QWidget* parent) : QGraphicsView(parent)
{
    // used by ActioHandler to identfy this view
    setProperty("view","graph");

    actionHandler_=new ActionHandler(this,this);

    scene_ = new QGraphicsScene(this);
    setScene(scene_);

    model_ = new TriggerGraphModel(TriggerGraphModel::TriggerMode,this);
    delegate_ = new TriggerGraphDelegate(this);
    builder_ = new SimpleGraphLayoutBuilder();

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
    scene()->clear();
}

void TriggerGraphView::clear()
{
    model_->clearData();
    for(auto n: nodes_) {
        scene()->removeItem(n);
    }
    nodes_.clear();

    for(auto e: edges_) {
        scene()->removeItem(e);
    }
    edges_.clear();

    if (edgeInfo_)
        edgeInfo_->close();
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
    if(!itemClicked)
        return;

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
    if(cmd == "lookup") {
        Q_EMIT linkSelected(info);
    } else if(cmd == "expand") {
        if (info && info->node()) {
            show(info->node());
        }
    } else if(cmd == "expand_parent") {
        if (info && info->item()) {
            showParent(info->item());
        }
    } else if(cmd ==  "edit") {
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
            updateEdgePens();
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
            updateEdgePens();
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
            updateEdgePens();
            Q_EMIT linePenChanged();
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

// called by the graph widget
void TriggerGraphView::show(VNode* node, bool dependency)
{
    clear();
    dependency_ = dependency;
    show(node);
}

void TriggerGraphView::show(VNode* node)
{
    Q_ASSERT(node);

    scan(node);
    model_->setItems(nodes_);
    for(auto n: nodes_) {
        //UiLog().dbg() << n->item()->fullPath();
        n->adjustSize();
    }

    buildLayout();
    adjustSceneRect();
}

void TriggerGraphView::showParent(VItem* item)
{
    Q_ASSERT(item);
    if(VNode *p = item->parent()) {
        addRelation(p, item, nullptr, TriggerCollector::Hierarchy, nullptr);
        show(p);
    }
}

void TriggerGraphView::scan(VNode* node)
{
    Q_ASSERT(node);

    // add the node to the layout
    addNode(node);

    // performs the trigger collection - it will populate the layout
    TriggerRelationCollector tc(node, this, dependency_);
    node->triggers(&tc);

    TriggeredRelationCollector tc1(node, this, dependency_);
    node->triggered(&tc1);
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

    for(auto e: edges_) {
        e->adjust();
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

void TriggerGraphView::updateEdgePens()
{
    for(auto e: edges_) {
        setEdgePen(e);
    }
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

void TriggerGraphView::notifyEdgeSelected(TriggerGraphEdgeItem* e)
{
    if (!edgeInfo_) {
        edgeInfo_ = new TriggerGraphEdgeInfoProxy(this);
        connect(edgeInfo_, SIGNAL(anchorClicked(QUrl)),
                this, SLOT(slotEdgeInfo(QUrl)));
    }

    edgeInfo_->setInfo(e);
}

void TriggerGraphView::slotEdgeInfo(const QUrl& link)
{
    if (nodes_.size() > 0 && nodes_[0]->item()) {
        VInfo_ptr info=VInfo::createFromPath(
                    nodes_[0]->item()->server(),
                    link.toString().toStdString());
        if(info)
            Q_EMIT linkSelected(info);
    }
}

QPixmap TriggerGraphView::makeLegendPixmap()
{
    QFont f;
    QFontMetrics fm(f);
    int margin = 2;
    int gap = 6;
    int lineLen = 20;

    QMap<QString, QPen> data;
    data["trigger"] = triggerConnectPen_;
    data["dependency"] = depConnectPen_;
    data["parent"] = parentConnectPen_;

    int totalWidth = margin;
    Q_FOREACH(QString s, data.keys()) {
        totalWidth += lineLen + gap + fm.width(s) + gap;
    }
    totalWidth += margin;

    QPixmap pix(totalWidth,fm.height() + 2* margin);
    pix.fill(QColor(253, 253, 253));
    int lineY = pix.height()/2;
    QPainter p(&pix);
    p.setRenderHints(QPainter::Antialiasing);

    int xp = 0;
    QRect r(0,0,margin, pix.height());
    Q_FOREACH(QString s, data.keys()) {
        xp = r.right() + gap;
        QPen pen(data[s]);
        pen.setWidth(2);
        p.setPen(data[s]);
        p.drawLine(xp, lineY, xp + lineLen, lineY);
        p.drawLine(xp + lineLen/2 + 3, lineY, xp + lineLen/2 - 3, lineY -3);
        p.drawLine(xp + lineLen/2 + 3, lineY, xp + lineLen/2 - 3, lineY +3);
        //p.setPen(Qt::black);
        pen.setWidth(1);
        xp += lineLen + gap;
        r.setX(xp);
        r.setWidth(fm.width(s));
        p.drawText(r, Qt::AlignVCenter, s);
    }

    xp = r.right() + gap;

    return pix;
}

void TriggerGraphView::setZoomLevel(int level)
{
    if (level >= minZoomLevel_ && level <= maxZoomLevel_) {
        float actScale = scaleFromLevel(zoomLevel_);
        zoomLevel_     = level;
        float newScale = scaleFromLevel(level);
        float factor   = newScale / actScale;
        scale(factor, factor);
    }
}

//int TriggerGraphView::zoomLevelFromScale(float sc) const
//{
//    int level = static_cast<int>(round(log(sc) / log(1 + zoomDelta_)));
//    if (level < minZoomLevel_)
//        level = minZoomLevel_;
//    if (level > maxZoomLevel_)
//        level = maxZoomLevel_;
//    return level;
//}

float TriggerGraphView::currentScale() const
{
    return pow(1. + zoomDelta_, zoomLevel_);
}

float TriggerGraphView::scaleFromLevel(int level) const
{
    return pow(1. + zoomDelta_, level);
}

