//============================================================================
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TRIGGERGRAPHVIEW_HPP
#define TRIGGERGRAPHVIEW_HPP

#include "TriggerCollector.hpp"
#include "VInfo.hpp"
#include "VProperty.hpp"

#include <QDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsTextItem>
#include <QGraphicsWidget>
#include <QPersistentModelIndex>
#include <QTextBrowser>

class ActionHandler;
class PropertyMapper;
class TriggerGraphDelegate;
class TriggerGraphLayout;
class TriggerGraphModel;
class TriggerGraphView;
class GraphLayoutBuilder;
class GraphLayoutNode;
class QToolButton;
class QModelIndex;


class TriggerGraphNodeItem: public QGraphicsItem
{
public:
    enum
    {
        Type = UserType + 1
    };

    TriggerGraphNodeItem(int index, VItem* item, TriggerGraphView*);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                    QWidget *widget) override;
    int type() const override { return Type; }

    int index() const {return index_;}
    VItem* item() const {return item_;}
    void addRelation(TriggerGraphNodeItem* o);
    void adjustSize();
    void adjustPos(int x, int y);
    void setCentral(bool c) {central_= c;}
    GraphLayoutNode* toGraphNode();

protected:
    QRectF bRect_;
    int index_;
    VItem* item_;
    TriggerGraphView* view_;
    std::vector<TriggerGraphNodeItem*> parents_;
    std::vector<TriggerGraphNodeItem*> children_;
    bool central_ {false};
};


class TriggerGraphEdgeItem: public QGraphicsPathItem
{
    friend class TriggerGraphEdgeInfoProxy;
    friend class TriggerGraphEdgeInfoDialog;
public:
    enum
    {
        Type = UserType + 2
    };

    TriggerGraphEdgeItem(TriggerGraphNodeItem* from, TriggerGraphNodeItem* to,
                         VItem* through, TriggerCollector::Mode mode,
                         VItem* trigger, TriggerGraphView* view);

    int type() const override { return Type; }
    bool sameAs(TriggerGraphNodeItem* from, TriggerGraphNodeItem* to) const {
        return (from == from_ && to_ == to);
    }
    void addTrigger(VItem* through, TriggerCollector::Mode mode,
                VItem* trigger);

    void adjust();
    TriggerCollector::Mode mode() const {return modes_[0];}

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    TriggerGraphNodeItem* from_;
    TriggerGraphNodeItem* to_;
    std::vector<VItem*> throughs_;
    std::vector<TriggerCollector::Mode> modes_;
    std::vector<VItem*> triggers_;
    QRectF bRect_;
    TriggerGraphView* view_;
    float arrowWidth_ {10.};
    float arrowHeight_  {8.};
};

class TriggerGraphEdgeInfoDialog : public QDialog
{
    Q_OBJECT
public:
     TriggerGraphEdgeInfoDialog(QWidget* parent=nullptr);
     void setInfo(TriggerGraphEdgeItem*);

Q_SIGNALS:
    void anchorClicked(const QUrl& link);

protected:
     QString makeHtml(TriggerGraphEdgeItem*) const;
     void makeRow(QString label, VItem* t, QString& s) const;
     void makeTrigger(VItem* trigger, VItem* through, TriggerCollector::Mode mode, QString& s) const;

     QTextBrowser* te_;
};

class TriggerGraphView : public QGraphicsView, public VPropertyObserver
{
    Q_OBJECT
    friend class TriggerRelationCollector;
    friend class TriggeredRelationCollector;

public:
    TriggerGraphView(QWidget* parent=nullptr);
    ~TriggerGraphView() override;

    void clear();
    void setInfo(VInfo_ptr);
    void notifyChange(VProperty* p) override;

    void adjustSceneRect();
    void nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect);

    TriggerGraphDelegate* delegate() const {return delegate_;}
    TriggerGraphModel* model() const {return model_;}
    bool dependency() const {return dependency_;}
    QPixmap makeLegendPixmap();
    int minZoomLevel() const {return minZoomLevel_;}
    int maxZoomLevel() const {return maxZoomLevel_;}
    int defaultZoomLevel() const {return defaultZoomLevel_;}
    int zoomLevel() const {return zoomLevel_;}

    void show(VNode*, bool dependency);
    void setEdgePen(TriggerGraphEdgeItem* e);
    void setTriggeredScanner(TriggeredScanner* scanner) {triggeredScanner_ = scanner;}
    void notifyEdgeSelected(TriggerGraphEdgeItem*);

public Q_SLOTS:
    //void slotSelectItem(const QModelIndex&);
    //void slotDoubleClickItem(const QModelIndex&);
    void slotContextMenu(const QPoint &position);
    void slotCommandShortcut();
    void slotViewCommand(VInfo_ptr,QString);
    void setZoomLevel(int);
    void slotEdgeInfo(const QUrl& link);
    //void slotRerender();
    //void slotSizeHintChangedGlobal();
    //void selectionChanged (const QItemSelection &selected, const QItemSelection &deselected) override;

Q_SIGNALS:
    void infoPanelCommand(VInfo_ptr,QString);
    void dashboardCommand(VInfo_ptr,QString);
    void linkSelected(VInfo_ptr);
    void linePenChanged();

protected:
    TriggerGraphNodeItem* nodeItemAt(QPointF scenePos) const;
    //QModelIndex indexAt(QPointF scenePos) const;
    //QModelIndex itemToIndex(QGraphicsItem* item) const;
    //TriggerGraphNodeItem* indexToItem(const QModelIndex& index) const;
    //QModelIndexList selectedIndexes();
    void adjustBackground(VProperty* p=nullptr);
    void adjustParentConnectColour(VProperty* p=nullptr);
    void adjustTriggerConnectColour(VProperty* p=nullptr);
    void adjustDepConnectColour(VProperty* p=nullptr);

    void show(VNode*);
    void showParent(VItem*);
    void scan(VNode*);
    void buildLayout();
    void addRelation(VItem* from, VItem* to,
                     VItem* through, TriggerCollector::Mode mode, VItem *trigger);
    TriggerGraphNodeItem* addNode(VItem* item);
    TriggerGraphEdgeItem* addEdge(
            TriggerGraphNodeItem* from, TriggerGraphNodeItem* to,
            VItem* through, TriggerCollector::Mode mode, VItem *trigger);

    void updateEdgePens();
    float currentScale() const;
    float scaleFromLevel(int level) const;

    QGraphicsScene* scene_;
    TriggerGraphDelegate* delegate_;
    TriggerGraphModel* model_;
    GraphLayoutBuilder* builder_;
    std::vector<TriggerGraphNodeItem*> nodes_;
    std::vector<TriggerGraphEdgeItem*> edges_;
    TriggerGraphEdgeInfoDialog* edgeInfo_ {nullptr};

    bool dependency_ {false};
    TriggeredScanner *triggeredScanner_ {nullptr};

    ActionHandler* actionHandler_;
    PropertyMapper* prop_;

    QPen parentConnectPen_ {QPen(Qt::red)};
    QPen triggerConnectPen_ {QPen(Qt::black)};
    QPen depConnectPen_ {QPen(Qt::blue)};

    int minZoomLevel_ {-10};
    int maxZoomLevel_ {10};
    int defaultZoomLevel_ {0};
    int zoomLevel_ {0};
    float zoomDelta_ {0.18};
};

#endif // TRIGGERGRAPHVIEW_HPP
