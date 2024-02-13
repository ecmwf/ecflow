/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_TriggerGraphView_HPP
#define ecflow_viewer_TriggerGraphView_HPP

#include <utility>

#include <QBasicTimer>
#include <QDialog>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QPersistentModelIndex>
#include <QTextBrowser>

#include "TriggerCollector.hpp"
#include "VInfo.hpp"
#include "VProperty.hpp"

class ActionHandler;
class PropertyMapper;
class TriggerViewDelegate;
class TriggerGraphLayout;
class TriggerGraphModel;
class TriggerGraphView;
class VComboSettings;
class GraphLayoutBuilder;
struct GraphLayoutNode;
class QShowEvent;
class QToolButton;

class QModelIndex;

class TriggerGraphNodeItem : public QGraphicsItem {
public:
    enum { Type = UserType + 1 };

    TriggerGraphNodeItem(int index, VItem* item, TriggerGraphView*);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    int type() const override { return Type; }

    int index() const { return index_; }
    VItem* item() const { return item_; }
    void addRelation(TriggerGraphNodeItem* o);
    void adjustSize();
    void adjustPos(int x, int y);
    bool detectSizeChange() const;
    bool detectSizeGrowth() const;
    void setExpanded(bool);
    GraphLayoutNode* toGraphNode();

protected:
    QRectF bRect_;
    int index_;
    VItem* item_;
    TriggerGraphView* view_;
    std::vector<TriggerGraphNodeItem*> parents_;
    std::vector<TriggerGraphNodeItem*> children_;
    bool expanded_{false};
};

class TriggerGraphEdgeItem : public QGraphicsPathItem {
    friend class TriggerGraphEdgeInfoProxy;
    friend class TriggerGraphEdgeInfoDialog;

public:
    enum { Type = UserType + 2 };

    TriggerGraphEdgeItem(TriggerGraphNodeItem* from,
                         TriggerGraphNodeItem* to,
                         VItem* through,
                         TriggerCollector::Mode mode,
                         VItem* trigger,
                         TriggerGraphView* view);

    int type() const override { return Type; }
    bool sameAs(TriggerGraphNodeItem* from, TriggerGraphNodeItem* to) const { return (from == from_ && to_ == to); }
    void addTrigger(VItem* through, TriggerCollector::Mode mode, VItem* trigger);

    void adjust();
    void setWayRects(const std::vector<int>& x, const std::vector<int>& y, const std::vector<int>& width);
    TriggerGraphNodeItem* from() const { return from_; }
    TriggerGraphNodeItem* to() const { return to_; }
    TriggerCollector::Mode mode() const { return modes_[0]; }
    QPainterPath shape() const override { return shapePath_; }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void addArrow(QPainterPath& pPath, double x1, double y1, double x2, double y2);
    void buildShape(QPolygonF pf);

    TriggerGraphNodeItem* from_;
    TriggerGraphNodeItem* to_;
    std::vector<VItem*> throughs_;
    std::vector<TriggerCollector::Mode> modes_;
    std::vector<VItem*> triggers_;
    QRectF bRect_;
    TriggerGraphView* view_;
    float arrowWidth_{10.};
    float arrowHeight_{8.};
    std::vector<QRectF> wayRects_;
    QPainterPath shapePath_;
    float shapeSpread_{6.};
};

class TriggerGraphEdgeInfoDialog : public QDialog {
    Q_OBJECT
public:
    explicit TriggerGraphEdgeInfoDialog(QWidget* parent = nullptr);
    void setInfo(TriggerGraphEdgeItem*);
    void readSettings(VComboSettings* vs);
    void writeSettings(VComboSettings* vs);

Q_SIGNALS:
    void anchorClicked(const QUrl& link);

protected:
    void closeEvent(QCloseEvent* event) override;
    QString makeHtml(TriggerGraphEdgeItem*) const;
    void makeRow(QString label, VItem* t, QString& s) const;
    void makeModeRow(TriggerCollector::Mode mode, QString& s) const;
    void makeTrigger(VItem* trigger, VItem* through, TriggerCollector::Mode mode, QString& s) const;

    QTextBrowser* te_;
    QSize lastSize_{QSize(350, 280)};
};

class TriggerGraphExpandStateItem;

class TriggerGraphExpandState {
public:
    enum Mode { ExpandNode = 0, ExpandParent = 1 };

    TriggerGraphExpandState() {}
    TriggerGraphExpandState(const TriggerGraphExpandState&);
    ~TriggerGraphExpandState() { clear(); }
    void add(VInfo_ptr, Mode);
    void remove(TriggerGraphExpandStateItem*);
    TriggerGraphExpandStateItem* find(VItem*) const;
    void clear();
    bool isEmpty() const { return items_.empty(); }

    std::vector<TriggerGraphExpandStateItem*> items_;
};

class TriggerGraphExpandStateItem {
public:
    TriggerGraphExpandStateItem(VInfo_ptr info, TriggerGraphExpandState::Mode mode) : info_(info), mode_(mode) {}

    VInfo_ptr info_;
    TriggerGraphExpandState::Mode mode_;
};

class TriggerGraphView : public QGraphicsView, public VPropertyObserver {
    Q_OBJECT
    friend class TriggerRelationCollector;
    friend class TriggeredRelationCollector;

public:
    explicit TriggerGraphView(QWidget* parent = nullptr);
    ~TriggerGraphView() override;

    void clear(bool keepConfig = false);
    void setInfo(VInfo_ptr);
    void notifyChange(VProperty* p) override;

    void adjustSceneRect();
    void nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect);

    TriggerViewDelegate* delegate() const { return delegate_; }
    TriggerGraphModel* model() const { return model_; }
    bool dependency() const { return dependency_; }
    int minZoomLevel() const { return minZoomLevel_; }
    int maxZoomLevel() const { return maxZoomLevel_; }
    int defaultZoomLevel() const { return defaultZoomLevel_; }
    int zoomLevel() const { return zoomLevel_; }
    void rerender();

    void show(VInfo_ptr, bool dependency);
    void setEdgePen(TriggerGraphEdgeItem* e);
    void setTriggeredScanner(TriggeredScanner* scanner) { triggeredScanner_ = scanner; }
    void notifyEdgeSelected(TriggerGraphEdgeItem*);

    void becameInactive();

    void readSettings(VComboSettings* vs);
    void writeSettings(VComboSettings* vs);

public Q_SLOTS:
    void slotContextMenu(const QPoint& position);
    void slotCommandShortcut();
    void slotViewCommand(VInfo_ptr, QString);
    void setZoomLevel(int);
    void slotEdgeInfo(const QUrl& link);

Q_SIGNALS:
    void infoPanelCommand(VInfo_ptr, QString);
    void dashboardCommand(VInfo_ptr, QString);
    void linkSelected(VInfo_ptr);
    void linePenChanged();

protected:
    void clearGraph(bool keepConfig = false);
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void showEvent(QShowEvent*) override;
    void timerEvent(QTimerEvent* event) override;
    TriggerGraphNodeItem* nodeItemAt(QPointF scenePos) const;
    TriggerGraphNodeItem* currentNodeItem() const;
    void adjustBackground(VProperty* p = nullptr);
    void adjustParentConnectColour(VProperty* p = nullptr);
    void adjustTriggerConnectColour(VProperty* p = nullptr);
    void adjustDepConnectColour(VProperty* p = nullptr);

    void expand(VNode*);
    void expandItem(VInfo_ptr, bool scanOnly);
    void toggleExpandItem(VInfo_ptr);
    void expandParent(VInfo_ptr, bool scanOnly);
    void collapseItem(VInfo_ptr);
    void scan(VNode*);
    void updateAfterScan();
    void rebuild();
    void buildLayout();
    void updateLayout();
    void initVisibleRegion();
    void doDelayedLayout();
    void cancelDelayedLayout();
    void addRelation(VItem* from, VItem* to, VItem* through, TriggerCollector::Mode mode, VItem* trigger);
    TriggerGraphNodeItem* addNode(VItem* item);
    TriggerGraphEdgeItem* addEdge(TriggerGraphNodeItem* from,
                                  TriggerGraphNodeItem* to,
                                  VItem* through,
                                  TriggerCollector::Mode mode,
                                  VItem* trigger);

    void updateEdgePens();
    float currentScale() const;
    float scaleFromLevel(int level) const;

    QGraphicsScene* scene_;
    TriggerViewDelegate* delegate_;
    TriggerGraphModel* model_;
    GraphLayoutBuilder* builder_;
    std::vector<TriggerGraphNodeItem*> nodes_;
    std::vector<TriggerGraphEdgeItem*> edges_;
    TriggerGraphEdgeInfoDialog* edgeInfo_;
    VInfo_ptr info_;
    VInfo_ptr lastExpandSelected_;
    bool needItemsLayout_{false};

    bool dependency_{false};
    TriggeredScanner* triggeredScanner_{nullptr};

    ActionHandler* actionHandler_;
    PropertyMapper* prop_;

    QPen parentConnectPen_{QPen(Qt::red)};
    QPen triggerConnectPen_{QPen(Qt::black)};
    QPen depConnectPen_{QPen(Qt::blue)};

    int minZoomLevel_{-10};
    int maxZoomLevel_{10};
    int defaultZoomLevel_{0};
    int zoomLevel_{0};
    float zoomDelta_{0.18};

    TriggerGraphExpandState expandState_;
    VNode* focus_{nullptr};

    int layoutDurationInMs_{0};
    QBasicTimer delayedLayoutTimer_;
};

#endif /* ecflow_viewer_TriggerGraphView_HPP */
