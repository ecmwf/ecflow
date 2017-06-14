//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TRIGGERTABLEVIEW_HPP
#define TRIGGERTABLEVIEW_HPP

#include <QTreeView>

#include "VInfo.hpp"

class ActionHandler;
class NodeQueryResultModel;
class TriggerViewDelegate;
class TriggerTableItem;
class TriggerTableModel;

class TriggerTableView : public QTreeView
{
Q_OBJECT

public:
    explicit TriggerTableView(QWidget *parent=0);
    ~TriggerTableView();

    void setModel(TriggerTableModel* model);

    void reload();
    void rerender();
    VInfo_ptr currentSelection();
    void currentSelection(VInfo_ptr n);
    void selectFirstServer();
    void enableContextMenu(bool enable);
    //void getListOfSelectedNodes(std::vector<VInfo_ptr> &nodeList);

    //void readSettings(VSettings* vs) {};

public Q_SLOTS:
    void slotSelectItem(const QModelIndex&);
    void slotDoubleClickItem(const QModelIndex&);
    void slotContextMenu(const QPoint &position);
    void slotViewCommand(VInfo_ptr,QString);
    void slotSetCurrent(VInfo_ptr);
    void slotRerender();
    void slotSizeHintChangedGlobal();
    void selectionChanged (const QItemSelection &selected, const QItemSelection &deselected);

Q_SIGNALS:
    void selectionChanged(TriggerTableItem*);
    void linkSelected(VInfo_ptr);
    void selectionChanged();
    void infoPanelCommand(VInfo_ptr,QString);
    void dashboardCommand(VInfo_ptr,QString);

protected:
    QModelIndexList selectedList();
    void handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget);

    TriggerTableModel* model_;
    ActionHandler* actionHandler_;
    bool needItemsLayout_;
    TriggerViewDelegate* delegate_;
};

#endif // TRIGGERTABLEVIEW_HPP
