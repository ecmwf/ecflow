//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TreeNodeView_HPP_
#define TreeNodeView_HPP_

#include <QTreeView>

#include "NodeViewBase.hpp"

#include "VInfo.hpp"
#include "VProperty.hpp"

class AbstractNodeView;
class ActionHandler;
class Animation;
class ExpandNode;
class TableNodeSortModel;
class PropertyMapper;
class TreeNodeModel;
class StandardNodeViewDelegategate;
class VTreeNode;

class TreeNodeView : public QObject, public NodeViewBase, public VPropertyObserver
{
Q_OBJECT

public:
    TreeNodeView(AbstractNodeView* view,TreeNodeModel* model,NodeFilterDef* filterDef,QWidget *parent=0);
    ~TreeNodeView();

    void reload();
    void rerender();
    QWidget* realWidget();
    QObject* realObject();
    VInfo_ptr currentSelection();
    void setCurrentSelection(VInfo_ptr n);
    void selectFirstServer();

    void notifyChange(VProperty* p);

    void readSettings(VSettings* vs) {}
    void writeSettings(VSettings* vs) {}

public Q_SLOTS:
    void slotContextMenu(const QPoint &position);
    void slotViewCommand(VInfo_ptr,QString);
#if 0
    void slotSaveExpand();
    void slotRestoreExpand();
#endif
    void slotSaveExpand(const VTreeNode* node);
    void slotRestoreExpand(const VTreeNode* node);
    void slotRepaint(Animation*);
    void slotRerender();
    void slotSizeHintChangedGlobal();

protected Q_SLOTS:
    void slotDoubleClickItem(const QModelIndex&);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

Q_SIGNALS:
    void selectionChanged(VInfo_ptr);
    void infoPanelCommand(VInfo_ptr,QString);
    void dashboardCommand(VInfo_ptr,QString);

protected:
    QModelIndexList selectedList();
    void handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget);

    void saveExpand(ExpandNode *parentExpand,const QModelIndex& idx);
    void restoreExpand(ExpandNode *expand,const VNode* node);
    void expandTo(const QModelIndex& idxTo);
    void saveExpandAll(const QModelIndex& idx);
    void saveCollapseAll(const QModelIndex& idx);

    void setCurrentSelectionFromExpand(VInfo_ptr info);
    void regainSelectionFromExpand();

    void adjustBackground(QColor col);
    void adjustIndentation(int);
    void adjustAutoExpandLeafNode(bool b);
    void adjustDrawBranchLine(bool b);
    void adjustBranchLineColour(QColor col);
    void adjustServerToolTip(bool);
    void adjustNodeToolTip(bool);
    void adjustAttributeToolTip(bool);

    AbstractNodeView *view_;
    TreeNodeModel* model_;
    ActionHandler* actionHandler_;
    bool needItemsLayout_;
    PropertyMapper* prop_;
    QMap<QString,QString> styleSheet_;
    bool setCurrentIsRunning_;
    bool setCurrentFromExpandIsRunning_;
    bool canRegainCurrentFromExpand_;
    VInfo_ptr lastSelection_;
    bool inStartUp_;
};

#endif



