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

class TriggerGraphNodeItem: public QGraphicsItem
{
public:
    enum
    {
        Type = UserType + 1
    };

    TriggerGraphNodeItem(const QModelIndex& index, TriggerGraphDelegate*, int, QGraphicsItem* parent);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                    QWidget *widget) override;
    int type() const override { return Type; }

    void setIndex(const QModelIndex& index) {index_ = index;}
    const QModelIndex& index() {return index_;}

protected:
    void adjust();

    QPersistentModelIndex index_;
    TriggerGraphDelegate* delegate_;
    int col_;
    QRectF bRect_;
};


class TriggerGraphEdgeItem: public QGraphicsPathItem
{
public:
    enum
    {
        Type = UserType + 2
    };

    TriggerGraphEdgeItem(QGraphicsItem* srcItem, QGraphicsItem* targetItem,
                         TriggerCollector::Mode mode, QGraphicsItem* parent);
    int type() const override { return Type; }
    //QRectF boundingRect() const override;
    //void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
    //                QWidget *widget) override;

    TriggerCollector::Mode mode() const {return mode_;}

protected:
    void adjust();

    QGraphicsItem* srcItem_;
    QGraphicsItem* targetItem_;
    TriggerCollector::Mode mode_;
    QRectF bRect_;
};


class TriggerGraphScene : public QGraphicsScene
{
public:
    TriggerGraphScene(QWidget* parent= nullptr);
};

class TriggerGraphView : public QGraphicsView, public VPropertyObserver
{
    Q_OBJECT
public:
    TriggerGraphView(QWidget* parent=nullptr);
    ~TriggerGraphView() override;

    void clear();
    void setInfo(VInfo_ptr);
    void notifyChange(VProperty* p) override;
    void adjustBackground(VProperty* p=nullptr);
    void adjustSceneRect();
    void nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect);
    void scan(VNode*);
    void setEdgePen(TriggerGraphEdgeItem* e);

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
    QModelIndex indexAt(QPointF scenePos) const;
    QModelIndex itemToIndex(QGraphicsItem* item) const;
    TriggerGraphNodeItem* indexToItem(const QModelIndex& index) const;
    QModelIndexList selectedIndexes();
    void adjustParentConnectColour(VProperty* p=nullptr);
    void adjustTriggerConnectColour(VProperty* p=nullptr);
    void adjustDepConnectColour(VProperty* p=nullptr);

    TriggerGraphScene* scene_;
    TriggerGraphDelegate* delegate_;
    TriggerGraphModel* model_;
    ActionHandler* actionHandler_;
    PropertyMapper* prop_;

    QPen parentConnectPen_ {QPen(Qt::red)};
    QPen triggerConnectPen_ {QPen(Qt::black)};
    QPen depConnectPen_ {QPen(Qt::blue)};

    TriggerGraphLayout* layout_;
};

#endif // TRIGGERGRAPHVIEW_HPP
