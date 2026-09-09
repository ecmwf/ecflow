/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TableFilterWidget_HPP
#define ecflow_viewer_TableFilterWidget_HPP

#include <QWidget>

#include "ui_TableFilterWidget.h"

class NodeFilterDef;
class ServerFilter;

class TableFilterWidget : public QWidget, private Ui::TableFilterWidget {
    Q_OBJECT

public:
    explicit TableFilterWidget(QWidget* parent = nullptr);
    ~TableFilterWidget() override = default;

    void build(NodeFilterDef*, ServerFilter*);

    /**
     * Setup the Table Filter Widget, using an interactive a Node Filter dialog to define the which nodes
     * are displayed.
     *
     * This implies showing a Node Filter dialog to the user and allow the user defining the new filter
     * (creating the widget) or cancelling the filter/widget creation.
     *
     * @return true if a new filter is created ("Apply" button selected), false otherwise ("Cancel" button selected).
     */
    bool setupFilterInteractive();

public Q_SLOTS:
    void slotEdit();
    void slotDefChanged();
    void slotHeaderFilter(QString column, QPoint globalPos);
    void slotTotalNumChanged(int);

private:
    NodeFilterDef* filterDef_{nullptr};
    ServerFilter* serverFilter_{nullptr};
};

#endif /* ecflow_viewer_TableFilterWidget_HPP */
