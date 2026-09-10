/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_Dashboard_HPP
#define ecflow_viewer_Dashboard_HPP

#include <QMainWindow>

#include "ServerFilter.hpp"
#include "VInfo.hpp"
#include "VSettings.hpp"
#include "Viewer.hpp"

class DashboardTitle;
class DashboardWidget;
class ServerFilter;
class ServerItem;
class VComboSettings;

class Dashboard : public QMainWindow, public ServerFilterObserver {
    Q_OBJECT

public:
    explicit Dashboard(QString, QWidget* parent = nullptr);
    ~Dashboard() override;

    void reload();
    void rerender();

    DashboardWidget* addWidget(const std::string& type);
    DashboardWidget* addDialog(const std::string& type);
    Viewer::ViewMode viewMode();
    void setViewMode(Viewer::ViewMode);
    ServerFilter* serverFilter() const { return serverFilter_; }
    VInfo_ptr currentSelection();
    void currentSelection(VInfo_ptr n);
    bool selectInTreeView(VInfo_ptr);
    void addSearchDialog();
    DashboardTitle* titleHandler() const { return titleHandler_; }
    bool hasMaximised() const;
    bool hasMaximisedApplied() const;

    void notifyServerFilterAdded(ServerItem* item) override;
    void notifyServerFilterRemoved(ServerItem* item) override;
    void notifyServerFilterChanged(ServerItem*) override;
    void notifyServerFilterDelete() override;

    void writeSettings(VComboSettings*);
    void readSettings(VComboSettings*);

Q_SIGNALS:
    void selectionChanged(VInfo_ptr);
    void contentsChanged();
    void aboutToDelete();

public Q_SLOTS:
    void slotPopInfoPanel(VInfo_ptr, QString);
    void slotCommand(VInfo_ptr, QString);

protected Q_SLOTS:
    void slotDockClose();
    void slotDialogFinished(int);
    void slotDialogClosed();
    void slotPopInfoPanel(QString);
    void slotSelectionChanged(VInfo_ptr info);
    void slotMaximisedChanged(DashboardWidget* w);
    void slotLayoutChanged();

protected:
    void contextMenuEvent(QContextMenuEvent* e) override;

private:
    DashboardWidget* addWidgetCore(const std::string& type, bool userAddedView);
    DashboardWidget* addWidget(const std::string& type, const std::string& dockId, bool userAddedView);
    QString uniqueDockId();
    static std::string widgetSettingsId(int i);
    void initialSelectionInView();
    VInfo_ptr currentSelectionInView();
    void addSearchDialog(VInfo_ptr);
    void resetMaximised();
    void checkMaximisedState();

    ServerFilter* serverFilter_;
    DashboardTitle* titleHandler_;
    QList<DashboardWidget*> widgets_;
    QList<DashboardWidget*> popupWidgets_;
    QByteArray savedDockState_;
    bool settingsAreRead_;
    static int maxWidgetNum_;
};

#endif /* ecflow_viewer_Dashboard_HPP */
