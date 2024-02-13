/*
 * Copyright 2009- ECMWF and INPE.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
