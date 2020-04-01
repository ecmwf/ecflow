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

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QPersistentModelIndex>

class ActionHandler;
class PropertyMapper;
class TriggerGraphDelegate;
class TriggerGraphLayout;
class TriggerGraphModel;
class TriggerGraphView;
class GraphLayoutBuilder;
class GraphLayoutNode;
class QModelIndex;


//class NodeRelation
//{
//public:
//    NodeRelation(VItem* trigger, VItem* through, int mode):
//        trigger_(trigger), through_(through), mode_(mode) {}

//    VItem* trigger_;
//    VItem* through_;
//    int	mode_;
//    NodeRelation* next_ {nullptr};
//};

//class TriggerGraphLayoutNode {
//public:
//    TriggerGraphLayoutNode(int index, VItem* item) : index_(index), item_(item) {}
//    void addRelation(TriggerGraphLayoutNode* o);

//    int index() const {return index_;}
//    VItem* item() const {return item_;}

//    TriggerGraphNodeItem* grNode() const {return grItem_;}
//    void addGrNode(const QModelIndex& idx, TriggerGraphDelegate* delegate);
//    void adjustGrPos(int x, int y, QGraphicsScene* scene);
//    GraphLayoutNode* cloneGraphNode();
//    int width() const;
//    int height() const;

//protected:
//    std::vector<TriggerGraphLayoutNode*> parents_;
//    std::vector<TriggerGraphLayoutNode*> children_;
//    int index_;
//    VItem* item_;
//    TriggerGraphNodeItem* grItem_ {nullptr};
//};


//class TriggerGraphLayoutEdge
//{
//public:
//    TriggerGraphLayoutEdge(
//        int from, int to, VItem* through, TriggerCollector::Mode mode, VItem* trigger) :
//        from_(from), to_(to), through_(through), mode_(mode), trigger_(trigger) {}

//    bool sameAs(int from, int to, VItem* through, TriggerCollector::Mode mode,
//                VItem* trigger) const {
//        return (from == from_ && to_ == to && through == through_ &&
//                mode == mode_ && trigger == trigger_);
//    }

//    int from_;
//    int to_;
//    VItem* through_;
//    TriggerCollector::Mode mode_;
//    VItem* trigger_;
//    TriggerGraphEdgeItem* grItem_;
//};



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

    //void setIndex(const QModelIndex& index) {index_ = index;}
    //const QModelIndex& index() {return index_;}

    int index() const {return index_;}
    VItem* item() const {return item_;}
    //void addGrNode(const QModelIndex& idx, TriggerGraphDelegate* delegate);
    void addRelation(TriggerGraphNodeItem* o);
    void adjustSize();
    void adjustPos(int x, int y);
    GraphLayoutNode* toGraphNode();

protected:
    QRectF bRect_;
    int index_;
    VItem* item_;
    TriggerGraphView* view_;
    std::vector<TriggerGraphNodeItem*> parents_;
    std::vector<TriggerGraphNodeItem*> children_;
};


class TriggerGraphEdgeItem: public QGraphicsPathItem
{
public:
    enum
    {
        Type = UserType + 2
    };

    TriggerGraphEdgeItem(TriggerGraphNodeItem* from, TriggerGraphNodeItem* to,
                         VItem* through, TriggerCollector::Mode mode,
                         VItem* trigger, TriggerGraphView* view);

    int type() const override { return Type; }
    //QRectF boundingRect() const override;
    //void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
    //                QWidget *widget) override;

//    TriggerGraphLayoutEdge(
//        int from, int to, VItem* through, TriggerCollector::Mode mode, VItem* trigger) :
//        from_(from), to_(to), through_(through), mode_(mode), trigger_(trigger) {}

    bool sameAs(TriggerGraphNodeItem* from, TriggerGraphNodeItem* to, VItem* through, TriggerCollector::Mode mode,
                VItem* trigger) const {
        return (from == from_ && to_ == to && through == through_ &&
                mode == mode_ && trigger == trigger_);
    }

    void adjust();
    TriggerCollector::Mode mode() const {return mode_;}

protected:
    TriggerGraphNodeItem* from_;
    TriggerGraphNodeItem* to_;
    VItem* through_;
    TriggerCollector::Mode mode_;
    VItem* trigger_;
    QRectF bRect_;
    TriggerGraphView* view_;
    float arrowWidth_ {10.};
    float arrowHeight_  {8.};
};


class TriggerGraphScene : public QGraphicsScene
{
public:
    TriggerGraphScene(QWidget* parent= nullptr);
};

class TriggerGraphView : public QGraphicsView, public VPropertyObserver
{
    Q_OBJECT
    friend class TriggerRelationCollector;

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

    void scan(VNode*, bool dependency);
    void setEdgePen(TriggerGraphEdgeItem* e);
    void setTriggeredScanner(TriggeredScanner* scanner) {triggeredScanner_ = scanner;}

public Q_SLOTS:
    //void slotSelectItem(const QModelIndex&);
    //void slotDoubleClickItem(const QModelIndex&);
    void slotContextMenu(const QPoint &position);
    void slotCommandShortcut();
    void slotViewCommand(VInfo_ptr,QString);
    //void slotRerender();
    //void slotSizeHintChangedGlobal();
    //void selectionChanged (const QItemSelection &selected, const QItemSelection &deselected) override;

Q_SIGNALS:
    void infoPanelCommand(VInfo_ptr,QString);
    void dashboardCommand(VInfo_ptr,QString);

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

    void scanOne(VNode*);
    void buildLayout();
    void addRelation(VItem* from, VItem* to,
                     VItem* through, TriggerCollector::Mode mode, VItem *trigger);
    TriggerGraphNodeItem* addNode(VItem* item);
    TriggerGraphEdgeItem* addEdge(
            TriggerGraphNodeItem* from, TriggerGraphNodeItem* to,
            VItem* through, TriggerCollector::Mode mode, VItem *trigger);


    TriggerGraphScene* scene_;
    TriggerGraphDelegate* delegate_;
    TriggerGraphModel* model_;
    GraphLayoutBuilder* builder_;
    std::vector<TriggerGraphNodeItem*> nodes_;
    std::vector<TriggerGraphEdgeItem*> edges_;

    bool dependency_ {false};
    TriggeredScanner *triggeredScanner_ {nullptr};

    ActionHandler* actionHandler_;
    PropertyMapper* prop_;

    QPen parentConnectPen_ {QPen(Qt::red)};
    QPen triggerConnectPen_ {QPen(Qt::black)};
    QPen depConnectPen_ {QPen(Qt::blue)};
};

#endif // TRIGGERGRAPHVIEW_HPP
