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
#include <QMap>
#include <QModelIndex>

class ActionHandler;
class ExpandState;
class PropertyMapper;
class TreeNodeModel;
class GraphNodeViewItem;

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

protected Q_SLOTS:
   void rowsInserted(QModelIndex,int,int);

protected:
    void mousePressEvent(QMouseEvent* event);

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


#endif // GRAPHNODEVIEW_HPP

