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
#include "ServerHandler.hpp"
#include "SessionHandler.hpp"
#include "TriggerViewDelegate.hpp"
#include "TriggerGraphModel.hpp"
#include "TriggerGraphLayoutBuilder.hpp"
#include "UiLog.hpp"
#include "UiLogS.hpp"
#include "VAttribute.hpp"
#include "VItemPathParser.hpp"
#include "VSettings.hpp"
#include "VNState.hpp"

#include <algorithm>
#include <math.h>

#include "Spline.hpp"

#include <QDebug>
#include <QFile>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsLinearLayout>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsProxyWidget>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QShowEvent>
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
    opt.rect=bRect_.toRect();

    QModelIndex idx=view_->model()->index(index_, 0);
    view_->delegate()->paint(painter,opt,idx);

    if (!expanded_) {
        if (isSelected()) {
            painter->setPen(QPen(Qt::black, 1, Qt::DashLine));
            painter->drawRect(opt.rect.adjusted(1,1,-1,-1));
        }
    } else  {
        if (!isSelected())
            painter->setPen(QPen(Qt::black, 2));
        else
            painter->setPen(QPen(Qt::black, 2, Qt::DashLine));

        painter->drawRect(opt.rect.adjusted(1,1,-2,-2));
    }
}

void TriggerGraphNodeItem::adjustSize()
{
    prepareGeometryChange();

    int w=0, h=0;
    QModelIndex idx=view_->model()->index(index_, 0);
    if (idx.isValid()) {
        view_->delegate()->sizeHintCompute(idx,w,h, true);
        bRect_ = QRectF(QPointF(0, 0), QSize(w+1, h));
    }
}

bool TriggerGraphNodeItem::detectSizeChange() const
{
    int w=0, h=0;
    QModelIndex idx=view_->model()->index(index_, 0);
    if (idx.isValid()) {
        view_->delegate()->sizeHintCompute(idx,w,h, true);
    }

    return ((w==0 && bRect_.width() != 0) ||
            w+1 != bRect_.width() ||
            h != bRect_.height());
}

bool TriggerGraphNodeItem::detectSizeGrowth() const
{
    int w=0, h=0;
    QModelIndex idx=view_->model()->index(index_, 0);
    if (idx.isValid()) {
        view_->delegate()->sizeHintCompute(idx,w,h, true);
    }

    if (w == 0)
        return false;

    return (w+1 > bRect_.width() || h > bRect_.height());
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

void TriggerGraphNodeItem::setExpanded(bool e)
{
    expanded_= e;
    update();
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
    from_(from), to_(to), view_(view)
{
    triggers_.push_back(trigger);
    modes_.push_back(mode);
    throughs_.push_back(through);

    setFlags(QGraphicsItem::ItemIsSelectable);
}

void TriggerGraphEdgeItem::adjust()
{
    prepareGeometryChange();

    int gap = 3;
    QPainterPath p;
    QRectF srcRect = from_->mapRectToParent(from_->boundingRect());
    QRectF targetRect = to_->mapRectToParent(to_->boundingRect());

    QPointF pOffset(srcRect.right() + gap, srcRect.center().y());
    QPointF p1(targetRect.left() - srcRect.right() - 2*gap,
               targetRect.center().y() - srcRect.center().y());

    bool splineUsed = false;

    // gry to draw spline if there are waypoints
    if (!wayRects_.empty()) {
        std::vector<double> xp, yp;

        xp.push_back(0.);
        yp.push_back(0.);
        for(auto v: wayRects_) {
            xp.push_back(v.center().x()-pOffset.x());
            yp.push_back(v.y()-pOffset.y() + srcRect.height()/2);
            //xp.push_back(v.right()-pOffset.x());
            //yp.push_back(v.y()-pOffset.y() + srcRect.height()/2);
        }
        xp.push_back(p1.x());
        yp.push_back(p1.y());

        Q_ASSERT(xp.size() >= 3);
        Spline spline(xp, yp);
        if (spline.status())  {
            splineUsed = true;
            double start=xp[0];
            double end=xp[xp.size()-1];
            size_t stepNum=100;
            double step = (end-start)/(stepNum-1);
            QPolygonF pf;
            for (size_t i=0; i < stepNum; i++) {
                double xp = start + static_cast<double>(i)*step;
                pf << QPointF(xp, spline.eval(xp));
            }

            p.addPolygon(pf);

            double xmid1 = (start + end)/2. - 5;
            double xmid2 = (start + end)/2. + 5;
            addArrow(p, xmid1, spline.eval(xmid1),
                        xmid2, spline.eval(xmid2));

            buildShape(pf);
        }
    }

    // bezier curve
    if (!splineUsed) {
        QPointF c1(p1.x()/2, 0);
        QPointF c2(p1.x()/2, targetRect.center().y() - srcRect.center().y());

        p.cubicTo(c1, c2, p1);

        // add arrow to mid-point of curve
        auto poly = p.toSubpathPolygons();
        if (poly.count() == 1) {
            int n = -1;
            if (poly[0].count() >= 3) {
                n = poly[0].count()/2-1;
            // staright line
            } else if (poly[0].count() == 2) {
                n = 0;
            }
            if (n >= 0) {
                addArrow(p,
                    poly[0].at(n).x(), poly[0].at(n).y(),
                    poly[0].at(n+1).x(), poly[0].at(n+1).y());
            }

            buildShape(poly[0]);
       }
    }

    view_->setEdgePen(this);

    setPos(QPointF(srcRect.right() + gap, srcRect.center().y()));

    setPath(p);

    if (!scene())
        view_->scene()->addItem(this);
}

void TriggerGraphEdgeItem::addArrow(QPainterPath& pPath, double x1, double y1, double x2, double y2)
{
    double angle = atan2(y2 - y1, x2 - x1);

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

        tri[i] += QPointF((x2+x1)/2., (y2+y1)/2.);
    }

    pPath.addPolygon(tri);
}

void TriggerGraphEdgeItem::buildShape(QPolygonF pf)
{
    QPointF delta(0, shapeSpread_);
    QPolygonF shapePoly;
    if (pf.count() < 10) {
        for(int i=0; i < pf.count(); i++) {
            shapePoly << pf[i] + delta;
        }
        for(int i=pf.count()-1; i >=0; i--) {
            shapePoly << pf[i] - delta;
        }
    } else {
        int n = pf.count()/10;
        for(int i=0; i < pf.count(); i+=n) {
            shapePoly << pf[i] + delta;
        }
        shapePoly << pf.last() + delta;
        for(int i=pf.count()-1; i >=0; i-=n) {
            shapePoly << pf[i] - delta;
        }
        shapePoly << pf.first() - delta;
    }

    shapePath_ = QPainterPath();
    shapePath_.addPolygon(shapePoly);
}


void TriggerGraphEdgeItem::setWayRects(const std::vector<int>& x,
                                        const std::vector<int>& y,
                                        const std::vector<int>& width)
{
    wayRects_.clear();
    if (x.size() != y.size() || x.size() != width.size()) {
        return;
    }
    for (size_t i=0; i < x.size(); i++) {
        wayRects_.emplace_back(QRectF(x[i], y[i], width[0], 0));
    }
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

void TriggerGraphEdgeItem::addTrigger(VItem* through, TriggerCollector::Mode mode,
            VItem* trigger)
{
    for(size_t i=0; i < throughs_.size(); i++) {
        if (through == throughs_[i] &&
                mode == modes_[i] && trigger == triggers_[i]) {
            return;
        }
    }

    triggers_.push_back(trigger);
    modes_.push_back(mode);
    throughs_.push_back(through);
}

//=============================================================
//
// TriggerGraphEdgeInfoWidget
//
//=============================================================

TriggerGraphEdgeInfoDialog::TriggerGraphEdgeInfoDialog(QWidget* parent) :
    QDialog(parent)
{
    setWindowTitle(tr("Trigger details"));
    setModal(false);

    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->setContentsMargins(0,0,0,0);
    vb->setSpacing(2);

    te_ = new QTextBrowser(this);
    te_->setOpenExternalLinks(false);
    te_->setOpenLinks(false);
    te_->setReadOnly(true);

    //Read css for the text formatting
    QString cssDoc;
    QFile f(":/viewer/trigger.css");
    if(f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        cssDoc=QString(f.readAll());
    }
    f.close();
    te_->document()->setDefaultStyleSheet(cssDoc);

    vb->addWidget(te_);

    connect(te_, SIGNAL(anchorClicked(const QUrl&)),
            this, SIGNAL(anchorClicked(const QUrl&)));

    hide();
    resize(lastSize_);
}

void TriggerGraphEdgeInfoDialog::closeEvent(QCloseEvent * event)
{
    event->accept();
    lastSize_ = size();
}

void TriggerGraphEdgeInfoDialog::setInfo(TriggerGraphEdgeItem* e)
{
    te_->setHtml(makeHtml((e)));
    //setPos(e->pos());

    if (!isVisible()) {
        resize(lastSize_);
    }

    show();
}

QString TriggerGraphEdgeInfoDialog::makeHtml(TriggerGraphEdgeItem* e) const
{
    QString s = "<table width=\'100%\'>";

    VItem *t = e->from_->item();
    makeRow("from", t, s);

    t = e->to_->item();
    makeRow("to", t, s);

    if (e->triggers_.size() > 0) {
        makeModeRow(e->modes_[0], s);

        if (e->triggers_[0] || e->triggers_.size() > 1) {
            s += "<tr><td></td><td></td></tr>";
            s += "<tr><td class=\'grtitle\' colspan=\'2\'> Triggers </td></tr>";
        }
    }

    for(size_t i=0; i < e->triggers_.size(); i++) {
        if (e->triggers_[i]) {
            makeTrigger(e->triggers_[i], e->throughs_[i], e->modes_[i], s);
        }
    }

    s += "</table>";

    return s;
}

void TriggerGraphEdgeInfoDialog::makeRow(QString label, VItem* t, QString& s) const
{
    QString type=QString::fromStdString(t->typeName());
    QString path=QString::fromStdString(t->fullPath());
    QString anchor=QString::fromStdString(VItemPathParser::encode(t->fullPath(),t->typeName()));

    s += "<tr><td class=\'grfirst\'>" + label + "</td>";
    s += "<td>[" + type + "] <a href=\'" + anchor + "\'>" + path +"</a>";
    s += "</td></tr>";

}
void TriggerGraphEdgeInfoDialog::makeModeRow(TriggerCollector::Mode mode, QString &s) const
{
    QString modeTxt = "direct trigger";
    if (mode == TriggerCollector::Parent)
        modeTxt = "dependency via parent";
    else if (mode ==  TriggerCollector::Child)
        modeTxt = "dependency via child";
    else if (mode == TriggerCollector::Hierarchy)
        modeTxt = "parent-child relationship";

    s += "<tr><td class=\'grfirst\'>type</td><td>" + modeTxt + "</td></tr>";
}

void TriggerGraphEdgeInfoDialog::makeTrigger(VItem* trigger, VItem* through, TriggerCollector::Mode mode, QString& s) const
{
    Q_ASSERT(trigger);
    QString type=QString::fromStdString(trigger->typeName());
    QString path=QString::fromStdString(trigger->fullPath());
    QString anchor=QString::fromStdString(
                VItemPathParser::encode(trigger->fullPath(),trigger->typeName()));

    s += "<tr><td class=\'grfirst\'>trigger</td>";
    s += "<td>[" + type + "] <a href=\'" + anchor + "\'>" + path +"</a>";
    s += "</td></tr>";

    if (through)  {
        QString modeStr;
        if (mode == TriggerCollector::Parent) {
            modeStr = "via parent";
        } else if (mode == TriggerCollector::Child) {
            modeStr = "via child";
        }
        type=QString::fromStdString(through->typeName());
        path=QString::fromStdString(through->fullPath());
        anchor=QString::fromStdString(
                    VItemPathParser::encode(through->fullPath(),through->typeName()));

        s += "<tr><td class=\'grfirst\'>" + modeStr + "</td>";
        s += "<td> [" + type + "] <a href=\'" + anchor + "\'>" + path +"</a>";
        s += "</td></tr>";
    }

    s += "<tr><td></td></tr>";
}


void TriggerGraphEdgeInfoDialog::writeSettings(VComboSettings* vs)
{
    vs->beginGroup("infoDialog");
    vs->putQs("size",size());
    vs->endGroup();
}

void TriggerGraphEdgeInfoDialog::readSettings(VComboSettings* vs)
{
    vs->beginGroup("infoDialog");
    if(vs->containsQs("size"))
    {
        resize(vs->getQs("size").toSize());
    }
    vs->endGroup();
}

//===========================================================
//
// TriggerGraphExpandState
//
//===========================================================

TriggerGraphExpandState::TriggerGraphExpandState(const TriggerGraphExpandState &o)
{
    for(auto v: o.items_) {
        add(v->info_, v->mode_);
    }
}

void TriggerGraphExpandState::add(VInfo_ptr info, Mode mode)
{
    items_.push_back(new TriggerGraphExpandStateItem(info, mode));
}

TriggerGraphExpandStateItem* TriggerGraphExpandState::find(VItem* item) const
{
    for(auto v: items_) {
        if (v->info_->item() == item)
            return v;
    }
    return nullptr;
}

void TriggerGraphExpandState::remove(TriggerGraphExpandStateItem* item)
{
    if (item) {
        auto it = std::find(items_.begin(), items_.end(), item);
        if (it != items_.end()) {
            items_.erase(it);
        }
        delete item;
    }
}

void TriggerGraphExpandState::clear()
{
    for(auto v: items_) {
        delete v;
    }
    items_.clear();
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
    actionHandler_->setallowShortcutsForHiddenItems(true);

    scene_ = new QGraphicsScene(this);
    setScene(scene_);

    model_ = new TriggerGraphModel(TriggerGraphModel::TriggerMode,this);
    delegate_ = new TriggerViewDelegate(this);
    delegate_->setMaxLimitItems(6);
//    connect(delegate_,SIGNAL(sizeHintChangedGlobal()),
//            this,SLOT(slotSizeHintChangedGlobal()));

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

    // create the info dialogie, stays hidden
    edgeInfo_ = new TriggerGraphEdgeInfoDialog(this);
    connect(edgeInfo_, SIGNAL(anchorClicked(QUrl)),
            this, SLOT(slotEdgeInfo(QUrl)));
}

TriggerGraphView::~TriggerGraphView()
{
    delete actionHandler_;
    delete prop_;
    delete builder_;
    clear();
}

void TriggerGraphView::clear(bool keepConfig)
{
    info_.reset();    
    clearGraph(keepConfig);
}

void TriggerGraphView::clearGraph(bool keepConfig)
{
    model_->clearData();
    scene_->clear();
    nodes_.clear();
    edges_.clear();
    edgeInfo_->close();
    if (!keepConfig) {
        expandState_.clear();
    }
    focus_ = nullptr;
    cancelDelayedLayout();
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

TriggerGraphNodeItem* TriggerGraphView::currentNodeItem() const
{
    Q_FOREACH(QGraphicsItem* item, items()) {
        if (item->isSelected() && item->type() == TriggerGraphNodeItem::Type) {
            return static_cast<TriggerGraphNodeItem*>(item);
        }
    }
    return nullptr;
}

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
    if (QShortcut* sc = static_cast<QShortcut*>(QObject::sender())) {
        TriggerGraphNodeItem* item = currentNodeItem();
        if (item) {
            std::vector<TriggerGraphNodeItem*> itemLst;
            itemLst.push_back(item);

            std::vector<VInfo_ptr> nodeLst;
            for(auto n: itemLst) {
                VInfo_ptr info = VInfo::createFromItem(n->item());
                if(info && !info->isEmpty())
                    nodeLst.push_back(info);
            }
            actionHandler_->runCommand(nodeLst, sc->property("id").toInt());
        }
    }
}

void TriggerGraphView::slotViewCommand(VInfo_ptr info,QString cmd)
{
    if(cmd == "lookup") {
        Q_EMIT linkSelected(info);
    } else if(cmd == "expand") {
        if (info && info->node()) {
            expandItem(info, false);
        }
    } else if(cmd == "collapse") {
        if (info && info->node()) {
            collapseItem(info);
        }
    } else if(cmd == "toggle_expand") {
        if (info && info->node()) {
            toggleExpandItem(info);
        }
    } else if(cmd == "expand_parent") {
        if (info && info->item()) {
            expandParent(info, false);
        }
    } else if(cmd ==  "edit") {
        if(info && info->isAttribute())
        {
            AttributeEditor::edit(info,this);
        }
    }
}

void TriggerGraphView::rerender()
{
    // check if the first nodes's size changed
    // it is a good indicator if we need to recompute the layout
    bool sizeChanged = false;
    for(auto n: nodes_) {
        if (n->item()->isNode()) {
            sizeChanged = n->detectSizeChange();
            break;
        }
    }

    if (sizeChanged) {
        doDelayedLayout();
    } else {
        scene()->update();
    }
}

//void TriggerGraphView::slotSizeHintChangedGlobal()
//{
//    needItemsLayout_=true;
//}

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
    ServerHandler* server =  node->server();
    if (!server)
        return;

    //NOTE: the re-layout, if needed, is delayed by 0.1 s. It might be
    //too short and if there are a lot of changes at a synch and a lot of nodes
    //in the graph view it can be too demanding.

    // if the refresh period is too short we do not attemt a
    // re-layout just udpate the relevant graphical nodes
    bool longRefresh = (server->currentRefreshPeriod() == -1 ||
                        server->currentRefreshPeriod() > 5);

    // redraw the item - if its size grew we schedule a re-layout
    bool hasNode = false;
    for (auto n: nodes_) {
        if (VItem* item = n->item()) {
            if(VNode* vn=item->isNode()) {
                if (vn == node) {
                    n->update();
                    if (longRefresh && n->detectSizeGrowth()) {
                        doDelayedLayout();
                    }
                    hasNode = true;
                    break;
                }
            }
        }
    }

    if (hasNode) {
        bool attrChange = false;
        for(auto it : aspect)
        {
            if(it == ecf::Aspect::NODE_VARIABLE || it == ecf::Aspect::METER || it == ecf::Aspect::LIMIT ||
               it == ecf::Aspect::EVENT) {
                 attrChange = true;
                 break;
            }
        }

        // redraw the attributes belongoing to the node -
        // if any of their sizes grew we schedule a re-layout
        if (attrChange) {
            for(auto n: nodes_) {
                if (VItem* item = n->item()) {
                    if (VAttribute *a = item->isAttribute()) {
                        if(a->parent() == node) {
                            n->update();
                            if (longRefresh && !delayedLayoutTimer_.isActive() &&
                                n->detectSizeGrowth()) {
                                doDelayedLayout();
                            }
                        }
                    }
                }
            }
         }
    }
}

void TriggerGraphView::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == delayedLayoutTimer_.timerId()) {
        if (info_ && info_->server()) {
            UiLogS(info_->server()).dbg() << "TriggerGraphView delayed layout";
        }
        updateLayout();
        delayedLayoutTimer_.stop();
    }
}

void TriggerGraphView::doDelayedLayout()
{
    if(!delayedLayoutTimer_.isActive())
    {
        delayedLayoutTimer_.start(100,this);
    }
}

void TriggerGraphView::cancelDelayedLayout()
{
    delayedLayoutTimer_.stop();
}

//------------------------------------------
// Contents handling
//------------------------------------------

// called by the graph widget
void TriggerGraphView::show(VInfo_ptr info, bool dependency)
{
    //TODO: save and reload expand state when server is reset
    //bool sameInfo = (info_ && info == info_);
    //clear(sameInfo);
    dependency_ = dependency;
    info_ = info;
    expandState_.clear();
    expandItem(info_, false);

//    if(!sameInfo || !expandState_.find(info_->node())) {
//        expandState_.clear();
//        expandItem(info_, false);
//    } else {
//        rebuild();
//    }
}

void TriggerGraphView::expandItem(VInfo_ptr info, bool scanOnly)
{
    Q_ASSERT(info);
    if (info->isNode() && info->node() && !expandState_.find(info->node())) {
        expandState_.add(info, TriggerGraphExpandState::ExpandNode);
        scan(info->node());
        if(!scanOnly) {
            updateAfterScan();
        }
    }
}

void TriggerGraphView::toggleExpandItem(VInfo_ptr info)
{
    Q_ASSERT(info);
    if (info->isNode() && info->node()) {
        if (expandState_.find(info->node())) {
            collapseItem(info);
        } else {
            expandItem(info, false);
        }
    }
}

void TriggerGraphView::expandParent(VInfo_ptr info, bool scanOnly)
{
    Q_ASSERT(info);
    focus_=nullptr;
    if (VNode* n = info->node()) {
        VNode *p = n->parent();

        if (p)
            addRelation(p, n, nullptr, TriggerCollector::Hierarchy, nullptr);

        focus_ = n;
        auto exItem = expandState_.find(n);
        if (!exItem) {
            expandState_.add(info, TriggerGraphExpandState::ExpandParent);
            scan(n);
        } else if (exItem->mode_ == TriggerGraphExpandState::ExpandNode) {
            exItem->mode_ = TriggerGraphExpandState::ExpandParent;
        }

        if (p && !expandState_.find(p)) {
            VInfo_ptr pInfo = VInfo::createFromItem(p);
            expandState_.add(pInfo, TriggerGraphExpandState::ExpandNode);
            scan(p);
        }

        if(!scanOnly) {
            updateAfterScan();
        }
    }
}

void TriggerGraphView::updateAfterScan()
{
    model_->setItems(nodes_);

    if (!info_ || !info_->node()) {
        clear();
        return;
    }

    for(auto n: nodes_) {
        //UiLog().dbg() << n->item()->fullPath();
        n->adjustSize();
    }

    buildLayout();
    for (auto n: nodes_) {
        if (expandState_.find(n->item())) {
            n->setExpanded(true);
        }
    }
    adjustSceneRect();
}

void TriggerGraphView::collapseItem(VInfo_ptr info)
{
    if(info && info->node() && info != info_) {
        auto exItem = expandState_.find(info->node());
        if (exItem) {
            expandState_.remove(exItem);
            rebuild();
        }
    }
}

void TriggerGraphView::rebuild()
{
    auto expandCopy = expandState_;
    clearGraph();

    // make sure info is always added upfront
    if (info_ && info_->node() && !expandCopy.find(info_->node())) {
        addNode(info_->node());
    }

    for(auto it: expandCopy.items_) {
        if (it->mode_ == TriggerGraphExpandState::ExpandNode) {
            if (it->info_ && it->info_->node()) {
                expandItem(it->info_, true);
            }
        } else {
            if (it->info_ && it->info_->node()) {
                expandParent(it->info_, true);
            }
        }
    }

    updateAfterScan();
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
    node->triggered(&tc1, triggeredScanner_);
}

void TriggerGraphView::buildLayout()
{
    std::vector<GraphLayoutNode*> lnodes;
    for (auto n: nodes_) {
        lnodes.emplace_back(n->toGraphNode());
    }

    std::vector<GraphLayoutEdge*> enodes;

    int focus = 0;
    if (focus_) {
        for (auto n: nodes_) {
            if (n->item() == focus_) {
                focus = n->index();
                break;
            }
        }
    }

    focus = 0; //TODO: refine it
    builder_->build(lnodes, enodes, focus);

    for (size_t i=0; i < nodes_.size(); i++) {
        nodes_[i]->adjustPos(lnodes[i]->x_, lnodes[i]->y_);
        delete lnodes[i];
    }

    lnodes.clear();

    for(auto en: enodes) {
        for(auto e: edges_) {
            if (e->from()->index() == en->from_ &&
                    e->to()->index() == en->to_) {
                e->setWayRects(en->x_, en->y_, en->width_);
                break;
            }
        }
    }

    for(auto e: edges_) {
        e->adjust();
    }

    for(auto en: enodes) {
        delete en;
    }
}

void TriggerGraphView::updateLayout()
{
    for(auto n: nodes_) {
        n->adjustSize();
    }

    buildLayout();
    for (auto n: nodes_) {
        if (expandState_.find(n->item())) {
            n->setExpanded(true);
        }
    }
    adjustSceneRect();
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
        if (e->sameAs(from, to)) {
            e->addTrigger(through, mode, trigger);
            return e;
        }
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
    Q_ASSERT(edgeInfo_);
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

float TriggerGraphView::currentScale() const
{
    return pow(1. + zoomDelta_, zoomLevel_);
}

float TriggerGraphView::scaleFromLevel(int level) const
{
    return pow(1. + zoomDelta_, level);
}

void TriggerGraphView::becameInactive()
{
    edgeInfo_->close();
}

void TriggerGraphView::showEvent(QShowEvent* e)
{
    if(!e->spontaneous()) {
        edgeInfo_->close();
    }
    QGraphicsView::showEvent(e);
}

void TriggerGraphView::writeSettings(VComboSettings* vs)
{
    Q_ASSERT(edgeInfo_);
    vs->beginGroup("view");
    edgeInfo_->writeSettings(vs);
    vs->endGroup();
}

void TriggerGraphView::readSettings(VComboSettings* vs)
{
    Q_ASSERT(edgeInfo_);
    vs->beginGroup("view");
    edgeInfo_->readSettings(vs);
    vs->endGroup();
}
