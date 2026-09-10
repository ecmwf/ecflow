/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TriggerTableView_HPP
#define ecflow_viewer_TriggerTableView_HPP

#include <cassert>

#include <QTreeView>

#include "VInfo.hpp"

class ActionHandler;
class NodeQueryResultModel;
class TriggerViewDelegate;
class TriggerTableItem;
class TriggerTableModel;

class TriggerTableView : public QTreeView {
    Q_OBJECT

public:
    explicit TriggerTableView(QWidget* parent = nullptr);
    ~TriggerTableView() override;

    void setTableModel(TriggerTableModel* model);

    void reload();
    void rerender();
    void setCurrentItem(TriggerTableItem*);
    void enableContextMenu(bool enable);
    void enableOneRowMode();

public Q_SLOTS:
    void slotSelectItem(const QModelIndex&);
    void slotDoubleClickItem(const QModelIndex&);
    void slotContextMenu(const QPoint& position);
    void slotCommandShortcut();
    void slotViewCommand(VInfo_ptr, QString);
    void slotRerender();
    void slotSizeHintChangedGlobal();
    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;

Q_SIGNALS:
    void selectionChanged(TriggerTableItem*);
    void clicked(TriggerTableItem*);
    void linkSelected(VInfo_ptr);
    void selectionChanged();
    void infoPanelCommand(VInfo_ptr, QString);
    void dashboardCommand(VInfo_ptr, QString);

protected:
    QModelIndexList selectedList();
    void handleContextMenu(QModelIndex indexClicked,
                           QModelIndexList indexLst,
                           QPoint globalPos,
                           QPoint widgetPos,
                           QWidget* widget);

    TriggerTableModel* model_{nullptr};
    ActionHandler* actionHandler_;
    bool needItemsLayout_{false};
    TriggerViewDelegate* delegate_;

private:
    // we enforce the usage of setTableModel()
    void setModel(QAbstractItemModel*) override { assert(false); }
};

#endif /* ecflow_viewer_TriggerTableView_HPP */
