/*
 * Copyright 2009- ECMWF and INPE.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_TableNodeWidget_HPP
#define ecflow_viewer_TableNodeWidget_HPP

#include "NodeWidget.hpp"
#include "ui_TableNodeWidget.h"

class NodeStateFilter;
class TableNodeSortModel;
class VParamFilterMenu;
class VSettings;

class TableNodeWidget : public NodeWidget, protected Ui::TableNodeWidget {
    Q_OBJECT

public:
    TableNodeWidget(ServerFilter* servers, bool interactive, QWidget* parent = nullptr);
    ~TableNodeWidget() override;

    void populateDockTitleBar(DashboardDockTitleWidget* tw) override;
    void rerender() override;

    void writeSettings(VComboSettings*) override;
    void readSettings(VComboSettings*) override;

protected Q_SLOTS:
    void on_actionBreadcrumbs_triggered(bool b);
    void slotSelectionChangedInView(VInfo_ptr info);
    void slotSelectionAutoScrollChanged(bool b);

protected:
    void detachedChanged() override {}

private:
    TableNodeSortModel* sortModel_;
    VParamFilterMenu* stateFilterMenu_;
    QAction* acAutoScroll_;
};

#endif /* ecflow_viewer_TableNodeWidget_HPP */
