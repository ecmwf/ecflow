//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef GRAPHNODEVIEW_HPP
#define GRAPHNODEVIEW_HPP

#include "NodeViewBase.hpp"
#include "VInfo.hpp"
#include "VProperty.hpp"

#include <QGraphicsView>
#include <QAbstractScrollArea>
#include <QMap>
#include <QModelIndex>

class ActionHandler;
class ExpandState;
class PropertyMapper;
class TreeNodeModel;
class GraphNodeViewItem;
class TreeNodeViewDelegate;

struct CompactViewItem
{
    CompactViewItem() : parentItem(-1), expanded(true), hasChildren(false),
                      hasMoreSiblings(false), total(0),  height(0), x(0) {}

    CompactViewItem(const QModelIndex idx,int parentId,int xp, bool leaf);

    QModelIndex index; // we remove items whenever the indexes are invalidated
    int parentItem; // parent item index in viewItems
    uint expanded : 1;
    //uint spanning : 1;
    uint hasChildren : 1; // if the item has visible children (even if collapsed)
    uint hasMoreSiblings : 1;
    uint total : 8; // total number of children visible
    //uint level : 16; // indentation
    int height : 14; // row height
    uint x: 12;
    uint width: 10;
    int y;

    int right() const {return x+width;}
    int width1() const {return x+width;}
    int width12() const {return x+width;}

    int computeWidth();
    void paintItem(QPainter *p,int yp,TreeNodeViewDelegate*);
};


#if 0

class GraphNodeView : public QGraphicsView, public NodeViewBase, public VPropertyObserver
{
Q_OBJECT

public:
    explicit GraphNodeView(TreeNodeModel* model,NodeFilterDef* filterDef,QWidget *parent=0);
    ~GraphNodeView();

    void reload();
    void rerender();
    QWidget* realWidget();
    VInfo_ptr currentSelection();
    void setCurrentSelection(VInfo_ptr n);
    void selectFirstServer() {}

    void notifyChange(VProperty* p) {}

    void readSettings(VSettings* vs) {}
    void writeSettings(VSettings* vs) {}

public Q_SLOTS:
    void reset();

//protected Q_SLOTS:
//   void rowsInserted(QModelIndex,int,int);

protected:
    //void mousePressEvent(QMouseEvent* event);

    void attachModel();
    void insertItems(const QModelIndex& parent,GraphNodeViewItem *p=0);

    TreeNodeModel* model_;
    ActionHandler* actionHandler_;
    ExpandState *expandState_;
    bool needItemsLayout_;
    int defaultIndentation_;
    //TreeNodeViewDelegate* delegate_;
    PropertyMapper* prop_;
    QMap<QString,QString> styleSheet_;
    bool setCurrentIsRunning_;
    bool setCurrentFromExpand_;
};

#endif


class CompactNodeView : public QAbstractScrollArea, public NodeViewBase, public VPropertyObserver
{
Q_OBJECT

public:
    explicit CompactNodeView(TreeNodeModel* model,NodeFilterDef* filterDef,QWidget *parent=0);
    ~CompactNodeView();

    void reload();
    void rerender();
    QWidget* realWidget();
    VInfo_ptr currentSelection();
    void setCurrentSelection(VInfo_ptr n);
    void selectFirstServer() {}

    void notifyChange(VProperty* p);

    void readSettings(VSettings* vs) {}
    void writeSettings(VSettings* vs) {}

public Q_SLOTS:
    void reset();

protected Q_SLOTS:
    void rowsInserted(QModelIndex,int,int);

protected:
    //void mousePressEvent(QMouseEvent* event);
    void paintEvent(QPaintEvent *event);

    void attachModel();
    void insertItems(const QModelIndex& parent, int);
    void paint(QPainter *painter,const QRegion& region);

    void adjustIndentation(int);
    void adjustBackground(QColor col,bool asjustStyleSheet=true);
    //void adjustBranchLines(bool,bool asjustStyleSheet=true);
    void adjustStyleSheet();
    void adjustServerToolTip(bool);
    void adjustNodeToolTip(bool);
    void adjustAttributeToolTip(bool);

    TreeNodeModel* model_;
    ActionHandler* actionHandler_;
    ExpandState *expandState_;
    bool needItemsLayout_;
    int defaultIndentation_;
    //TreeNodeViewDelegate* delegate_;
    PropertyMapper* prop_;
    QMap<QString,QString> styleSheet_;
    bool setCurrentIsRunning_;
    bool setCurrentFromExpand_;

    TreeNodeViewDelegate* delegate_;
    mutable QVector<CompactViewItem*> viewItems_;
};



#endif // GRAPHNODEVIEW_HPP

