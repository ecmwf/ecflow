/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
