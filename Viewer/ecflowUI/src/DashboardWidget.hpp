/*
 * Copyright 2009- ECMWF and INPE.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_DashboardWidget_HPP
#define ecflow_viewer_DashboardWidget_HPP

#include <string>

#include <QAction>
#include <QDockWidget>
#include <QIcon>
#include <QWidget>

#include "VInfo.hpp"

class DashboardDockTitleWidget;
class NodePathWidget;
class ServerFilter;
class VComboSettings;

class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(const std::string& type, QWidget* parent = nullptr);
    ~DashboardWidget() override = default;

    virtual void populateDockTitleBar(DashboardDockTitleWidget*) = 0;
    virtual void populateDialog()                                = 0;
    virtual void reload()                                        = 0;
    virtual void rerender()                                      = 0;
    virtual bool initialSelectionInView() { return false; }
    virtual VInfo_ptr currentSelection() { return VInfo_ptr(); }
    QAction* detachedAction() const { return detachedAction_; }
    QAction* maximisedAction() const { return maximisedAction_; }
    virtual QList<QAction*> dockTitleActions() { return QList<QAction*>(); }

    bool detached() const;
    void setDetached(bool b);
    bool isMaximised() const;
    void resetMaximised();
    void setEnableMaximised(bool st);
    bool isInDialog() const { return inDialog_; }

    virtual void writeSettings(VComboSettings*);
    virtual void readSettings(VComboSettings*);
    virtual void writeSettingsForDialog() {}
    virtual void readSettingsForDialog() {}

    const std::string type() const { return type_; }
    void id(const std::string& id) { id_ = id; }

public Q_SLOTS:
    virtual void setCurrentSelection(VInfo_ptr) = 0;

Q_SIGNALS:
    void titleUpdated(QString, QString type = QString());
    void selectionChanged(VInfo_ptr);
    void maximisedChanged(DashboardWidget*);
    void popInfoPanel(VInfo_ptr, QString);
    void dashboardCommand(VInfo_ptr, QString);

protected Q_SLOTS:
    void slotDetachedToggled(bool);
    void slotMaximisedToggled(bool);

protected:
    virtual void detachedChanged() = 0;
    void setInDialog(bool);

    std::string id_;
    std::string type_;
    bool acceptSetCurrent_;
    QAction* detachedAction_;
    QAction* maximisedAction_;
    bool ignoreMaximisedChange_;
    NodePathWidget* bcWidget_;

private:
    bool inDialog_;
};

#endif /* ecflow_viewer_DashboardWidget_HPP */
