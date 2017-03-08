//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef COMPACTNODEVIEW_HPP
#define COMPACTNODEVIEW_HPP

#include "CompactView.hpp"
#include "ExpandState.hpp"
#include "NodeViewBase.hpp"
#include "VInfo.hpp"
#include "VProperty.hpp"

#include <QMap>
#include <QModelIndex>

class ActionHandler;
class Animation;
class ExpandNode;
//class ExpandState;
class PropertyMapper;
class VTreeNode;
class QItemSelection;

class CompactNodeView : public CompactView, public NodeViewBase, public VPropertyObserver
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
    void selectFirstServer();

    void notifyChange(VProperty* p);

    void readSettings(VSettings* vs) {}
    void writeSettings(VSettings* vs) {}

public Q_SLOTS:
    //void slotDoubleClickItem(const QModelIndex&);
    void slotContextMenu(const QPoint &position);
    void slotViewCommand(VInfo_ptr,QString);
    void slotSaveExpand();
    void slotRestoreExpand();
    void slotSaveExpand(const VTreeNode* node);
    void slotRestoreExpand(const VTreeNode* node);
    void slotRepaint(Animation*);
    void slotRerender();
    void slotSizeHintChangedGlobal();

protected Q_SLOTS:
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
    void expandAll(const QModelIndex& idx);
    void collapseAll(const QModelIndex& idx);
    void expandTo(const QModelIndex& idxTo);
    void setCurrentSelectionFromExpand(VInfo_ptr info);
    void regainSelectionFromExpand();

    void adjustIndentation(int);
    void adjustBackground(QColor col,bool asjustStyleSheet=true);
    //void adjustBranchLines(bool,bool asjustStyleSheet=true);
    void adjustStyleSheet();
    void adjustServerToolTip(bool);
    void adjustNodeToolTip(bool);
    void adjustAttributeToolTip(bool);

    ActionHandler* actionHandler_;
    bool needItemsLayout_;
    int defaultIndentation_;
    PropertyMapper* prop_;
    QMap<QString,QString> styleSheet_;
    bool setCurrentIsRunning_;
    bool setCurrentFromExpand_;
    VInfo_ptr lastSelection_;

    typedef ExpandState<CompactNodeView> CompactViewExpandState;
    QVector<CompactViewExpandState*> expandStates_;
};


#endif // COMPACTNODEVIEW_HPP

