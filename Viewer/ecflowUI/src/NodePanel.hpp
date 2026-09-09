/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_NodePanel_HPP
#define ecflow_viewer_NodePanel_HPP

#include <QIcon>

#include "TabWidget.hpp"
#include "VInfo.hpp"
#include "Viewer.hpp"

class Dashboard;
class DashboardTitle;
class ServerFilter;
class VComboSettings;

class NodePanel : public TabWidget {
    Q_OBJECT

public:
    explicit NodePanel(QWidget* parent = nullptr);
    ~NodePanel() override;

    void setViewMode(Viewer::ViewMode);
    Viewer::ViewMode viewMode();

    ServerFilter* serverFilter();

    Dashboard* currentDashboard();
    void addWidget();
    void resetWidgets(QStringList);
    void reload();
    void rerender();
    void refreshCurrent();
    void resetCurrent();
    VInfo_ptr currentSelection();
    bool selectInTreeView(VInfo_ptr);
    void addToDashboard(const std::string& type);
    void init();
    void openDialog(VInfo_ptr, const std::string& type);
    void addSearchDialog();

    void writeSettings(VComboSettings*);
    void readSettings(VComboSettings*);

public Q_SLOTS:
    void slotCurrentWidgetChanged(int);
    void slotSelection(VInfo_ptr);
    void slotNewTab();
    void slotSelectionChangedInWidget(VInfo_ptr);

protected Q_SLOTS:
    void slotTabRemoved();
    void slotTabTitle(DashboardTitle* w);

Q_SIGNALS:
    void itemInfoChanged(QString);
    void currentWidgetChanged();
    void selectionChangedInCurrent(VInfo_ptr);
    void contentsChanged();

protected:
    void resizeEvent(QResizeEvent* e) override;
    void adjustTabTitle();
    int tabAreaWidth() const;

    Dashboard* addWidget(QString);
    void tabBarCommand(QString, int) override;
    Dashboard* nodeWidget(int index);
    static std::string tabSettingsId(int i);

    bool settingsAreRead_{false};
};

#endif /* ecflow_viewer_NodePanel_HPP */
