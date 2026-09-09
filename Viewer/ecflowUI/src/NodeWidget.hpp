/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_NodeWidget_HPP
#define ecflow_viewer_NodeWidget_HPP

#include <QString>
#include <QWidget>

#include "DashboardWidget.hpp"
#include "VInfo.hpp"
#include "Viewer.hpp"

class QStackedLayout;
class QWidget;

class AbstractNodeModel;
class AttributeFilter;
class IconFilter;
class NodeFilterDef;
class NodeStateFilter;
class VModelData;
class NodePathWidget;
class NodeViewBase;
class ServerFilter;

class NodeWidget : public DashboardWidget {
    Q_OBJECT

public:
    void active(bool);
    bool active() const;
    NodeViewBase* view() const { return view_; }
    QWidget* widget();
    VInfo_ptr currentSelection() override;
    // void currentSelection(VInfo_ptr info);
    void reload() override;
    void populateDialog() override {}
    QList<QAction*> dockTitleActions() override { return dockActions_; }

public Q_SLOTS:
    void setCurrentSelection(VInfo_ptr) override;

protected Q_SLOTS:
    void slotInfoPanelAction();
    void slotSelectionChangedInBc(VInfo_ptr info);

protected:
    explicit NodeWidget(const std::string& type, ServerFilter* serverFilter, QWidget* parent = nullptr);
    ~NodeWidget() override;

    void updateActionState(VInfo_ptr);
    bool broadcastSelection() const { return broadcastSelection_; }

    ServerFilter* serverFilter_;

    AbstractNodeModel* model_;
    NodeViewBase* view_;

    IconFilter* icons_;
    AttributeFilter* atts_;

    NodeFilterDef* filterDef_;
    NodeStateFilter* states_;

private:
    void createActions();
    QList<QAction*> dockActions_;
    QMap<QString, QAction*> dockActionMap_;
    QList<QAction*> infoPanelActions_;
    bool broadcastSelection_;
};

#endif /* ecflow_viewer_NodeWidget_HPP */
