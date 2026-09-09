/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TreeNodeWidget_HPP
#define ecflow_viewer_TreeNodeWidget_HPP

#include "NodeWidget.hpp"
#include "VProperty.hpp"
#include "ui_TreeNodeWidget.h"

class AttributeFilter;
class NodeStateFilter;
class ServerFilter;
class VParamFilterMenu;
class VSettings;
class VTreeServer;
class TreeNodeWidget : public NodeWidget, public VPropertyObserver, protected Ui::TreeNodeWidget {
    Q_OBJECT

public:
    explicit TreeNodeWidget(ServerFilter*, QWidget* parent = nullptr);
    ~TreeNodeWidget() override;

    void populateDockTitleBar(DashboardDockTitleWidget* tw) override;

    void rerender() override;
    bool initialSelectionInView() override;
    void writeSettings(VComboSettings*) override;
    void readSettings(VComboSettings*) override;

    void notifyChange(VProperty*) override;

protected Q_SLOTS:
    void on_actionBreadcrumbs_triggered(bool b);
    void slotSelectionChangedInView(VInfo_ptr info);
    void slotAttsChanged();
    void firstScanEnded(const VTreeServer*);

protected:
    enum ViewLayoutMode { StandardLayoutMode, CompactLayoutMode };

    void initAtts();
    void detachedChanged() override {}
    void setViewLayoutMode(ViewLayoutMode);

    VParamFilterMenu* stateFilterMenu_;
    VParamFilterMenu* attrFilterMenu_;
    VParamFilterMenu* iconFilterMenu_;

    ViewLayoutMode viewLayoutMode_;
    VProperty* layoutProp_;

    static AttributeFilter* lastAtts_;

    std::string firstSelectionPath_;
};

#endif /* ecflow_viewer_TreeNodeWidget_HPP */
