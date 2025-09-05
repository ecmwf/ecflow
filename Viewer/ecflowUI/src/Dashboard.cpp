/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "Dashboard.hpp"

#include <QContextMenuEvent>
#include <QDebug>
#include <QDockWidget>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

#include "DashboardDialog.hpp"
#include "DashboardDock.hpp"
#include "DashboardTitle.hpp"
#include "InfoPanel.hpp"
#include "NodeSearchWidget.hpp"
#include "NodeSearchWindow.hpp"
#include "NodeWidget.hpp"
#include "ServerFilter.hpp"
#include "ServerHandler.hpp"
#include "TableNodeWidget.hpp"
#include "TreeNodeWidget.hpp"
#include "UiLog.hpp"
#include "VConfig.hpp"
#include "VFilter.hpp"
#include "VSettings.hpp"
#include "WidgetNameProvider.hpp"
#include "ecflow/core/Converter.hpp"

int Dashboard::maxWidgetNum_ = 20;

Dashboard::Dashboard(QString /*rootNode*/, QWidget* parent) : QMainWindow(parent), settingsAreRead_(false) {
    // We use the mainwindow as a widget. Its task is
    // to dock all the component widgets!
    setWindowFlags(Qt::Widget);

    setObjectName("db");

    // The serverfilter. It holds the list of servers displayed by this dashboard.
    serverFilter_ = new ServerFilter();
    serverFilter_->addObserver(this);

    titleHandler_ = new DashboardTitle(serverFilter_, this);

    // Central widget - we need to create it but we do not
    // use it. So we can hide it!
    QWidget* w = new QLabel("centre", this);
    w->setObjectName("dbc");
    setCentralWidget(w);
    w->hide();

    layout()->setContentsMargins(0, 0, 0, 0);

    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::AllowNestedDocks);
}

Dashboard::~Dashboard() {
    widgets_.clear();
    popupWidgets_.clear();

    Q_EMIT aboutToDelete();

    serverFilter_->removeObserver(this);
    delete serverFilter_;
}

DashboardWidget* Dashboard::addWidgetCore(const std::string& type, bool userAddedView) {
    DashboardWidget* w = nullptr;

    // Create a dashboard widget
    if (type == "tree") {
        NodeWidget* ctl = new TreeNodeWidget(serverFilter_, this);

        connect(ctl, SIGNAL(popInfoPanel(VInfo_ptr, QString)), this, SLOT(slotPopInfoPanel(VInfo_ptr, QString)));

        connect(ctl, SIGNAL(dashboardCommand(VInfo_ptr, QString)), this, SLOT(slotCommand(VInfo_ptr, QString)));

        w = ctl;
    }
    else if (type == "table") {
        NodeWidget* ctl = new TableNodeWidget(serverFilter_, userAddedView, this);

        connect(ctl, SIGNAL(popInfoPanel(VInfo_ptr, QString)), this, SLOT(slotPopInfoPanel(VInfo_ptr, QString)));

        connect(ctl, SIGNAL(dashboardCommand(VInfo_ptr, QString)), this, SLOT(slotCommand(VInfo_ptr, QString)));

        w = ctl;
    }
    else if (type == "info") {
        auto* ctl = new InfoPanel(this);

        connect(ctl, SIGNAL(popInfoPanel(VInfo_ptr, QString)), this, SLOT(slotPopInfoPanel(VInfo_ptr, QString)));

        connect(ctl, SIGNAL(dashboardCommand(VInfo_ptr, QString)), this, SLOT(slotCommand(VInfo_ptr, QString)));

        w = ctl;
    }

    if (w) {
        connect(w, SIGNAL(selectionChanged(VInfo_ptr)), this, SLOT(slotSelectionChanged(VInfo_ptr)));

        connect(w, SIGNAL(maximisedChanged(DashboardWidget*)), this, SLOT(slotMaximisedChanged(DashboardWidget*)));
    }

    return w;
}

void Dashboard::slotSelectionChanged(VInfo_ptr info) {
    auto* s = static_cast<DashboardWidget*>(sender());

    Q_FOREACH (DashboardWidget* dw, widgets_) {
        if (dw != s) {
            dw->setCurrentSelection(info);
        }
    }
    Q_FOREACH (DashboardWidget* dw, popupWidgets_) {
        if (dw != s) {
            dw->setCurrentSelection(info);
        }
    }

    Q_EMIT selectionChanged(info);
}

DashboardWidget* Dashboard::addWidget(const std::string& type) {
    // Get a unique dockId stored as objectName
    QString dockId = uniqueDockId();

    if (hasMaximised()) {
        resetMaximised();
    }

    bool popupTableFilter = false;
    if (VProperty* prop = VConfig::instance()->find("view.table.popupFilter")) {
        popupTableFilter = prop->value().toBool();
    }

    DashboardWidget* w = addWidget(type, dockId.toStdString(), popupTableFilter);

    // At this point the widgets can be inactive. Reload will make them active!!!
    w->reload();

    if (type == "info") {
        VInfo_ptr info = currentSelectionInView();
        if (info && info.get()) {
            if (auto* ip = static_cast<InfoPanel*>(w)) {
                ip->slotReload(info);
            }
        }
    }

    return w;
}

DashboardWidget* Dashboard::addWidget(const std::string& type, const std::string& dockId, bool userAddedView) {
    DashboardWidget* w = Dashboard::addWidgetCore(type, userAddedView);

    // If the db-widget creation fails we should do something!!!
    if (!w) {
        return nullptr;
    }

    // Store dockId in the db-widget
    w->id(dockId);

    widgets_ << w;

    // Create a dockwidget
    auto* dw = new DashboardDock(w, this);

    dw->setAllowedAreas(Qt::RightDockWidgetArea);

    // Store the dockId  in the dockwidget (as objectName)
    dw->setObjectName(QString::fromStdString(dockId));

    // Add the dockwidget to the dashboard
    addDockWidget(Qt::RightDockWidgetArea, dw);

    connect(dw, SIGNAL(closeRequested()), this, SLOT(slotDockClose()));

    checkMaximisedState();

    return w;
}

DashboardWidget* Dashboard::addDialog(const std::string& type) {
    DashboardWidget* w = Dashboard::addWidgetCore(type, false);

    // If the db-widget creation fails we should do something!!!
    if (!w) {
        return nullptr;
    }

    // The DashBoard or any of its children cannot be the parent of the
    // dialog because in this case it would be always on top its parent. This is
    // the behaviour when the dialog's parent is QMainWindow.
    auto* dia = new DashboardDialog(nullptr);

    // So the parent is 0 and we will emit a signal from the Dashboard
    // destructor to notify the dialog about the deletion. Then we can be
    // sure that the dialog deletes itself when the Dashboard gets deleted.
    connect(this, SIGNAL(aboutToDelete()), dia, SLOT(slotOwnerDelete()));

    connect(dia, SIGNAL(finished(int)), this, SLOT(slotDialogFinished(int)));

    connect(dia, SIGNAL(aboutToClose()), this, SLOT(slotDialogClosed()));

    // The dialog will reparent the widget
    dia->add(w);
    dia->show();

    return w;
}

void Dashboard::addSearchDialog() {
    // It will delete itself on close!!
    // The parent is 0, for the reason see the comment in addDialog()
    auto* d = new NodeSearchWindow(nullptr);
    d->queryWidget()->setServerFilter(serverFilter_);

    connect(d->queryWidget(), SIGNAL(selectionChanged(VInfo_ptr)), this, SLOT(slotSelectionChanged(VInfo_ptr)));

    connect(d->queryWidget(),
            SIGNAL(infoPanelCommand(VInfo_ptr, QString)),
            this,
            SLOT(slotPopInfoPanel(VInfo_ptr, QString)));

    // The dashboard signals the dialog on deletion
    connect(this, SIGNAL(aboutToDelete()), d, SLOT(slotOwnerDelete()));

    d->show();
}

void Dashboard::addSearchDialog(VInfo_ptr info) {
    // It will delete itself on close!!
    // The parent is 0, for the reason see the comment in addDialog()
    auto* d = new NodeSearchWindow(nullptr);
    d->queryWidget()->setServerFilter(serverFilter_);
    d->queryWidget()->setRootNode(info);

    connect(d->queryWidget(), SIGNAL(selectionChanged(VInfo_ptr)), this, SLOT(slotSelectionChanged(VInfo_ptr)));

    connect(d->queryWidget(),
            SIGNAL(infoPanelCommand(VInfo_ptr, QString)),
            this,
            SLOT(slotPopInfoPanel(VInfo_ptr, QString)));

    // The dashboard signals the dialog on deletion
    connect(this, SIGNAL(aboutToDelete()), d, SLOT(slotOwnerDelete()));

    d->show();
}

void Dashboard::slotDockClose() {
    if (auto* dock = static_cast<DashboardDock*>(sender())) {
        if (auto* dw = static_cast<DashboardWidget*>(dock->widget())) {
            widgets_.removeOne(dw);
            disconnect(this, nullptr, dw, nullptr);

            if (dw->isMaximised()) {
                resetMaximised();
            }

            checkMaximisedState();

            dw->deleteLater();
        }
        dock->deleteLater();
    }
}

VInfo_ptr Dashboard::currentSelection() {
    return currentSelectionInView();
}

void Dashboard::currentSelection(VInfo_ptr /*n*/) {
    // if(NodeWidget *ctl=handler_->currentControl())
    //	ctl->currentSelection(n);
}

void Dashboard::slotPopInfoPanel(QString name) {
    if (DashboardWidget* dw = addDialog("info")) {
        if (auto* ip = static_cast<InfoPanel*>(dw)) {
            ip->setDetached(true);
            ip->setCurrent(name.toStdString());
        }

        popupWidgets_ << dw;
    }
}

void Dashboard::slotPopInfoPanel(VInfo_ptr info, QString name) {
    if (DashboardWidget* dw = addDialog("info")) {
        if (auto* ip = static_cast<InfoPanel*>(dw)) {
            // ip->setDetached(true);
            ip->slotReload(info);
            ip->setCurrent(name.toStdString());
        }

        popupWidgets_ << dw;
    }
}

void Dashboard::slotCommand(VInfo_ptr info, QString cmd) {
    if (!info || !info.get()) {
        return;
    }

    if (cmd == "search") {
        addSearchDialog(info);
    }
}

//-------------------------------
// Maximise panel
//-------------------------------

void Dashboard::slotMaximisedChanged(DashboardWidget* w) {
    QList<DashboardDock*> dLst = findChildren<DashboardDock*>(QString());

    // maximise the given panel
    if (w->isMaximised()) {
        bool hasMaxApp = hasMaximisedApplied();
        // nothing can be maximised in practice at this point
        Q_ASSERT(hasMaxApp == false);
        savedDockState_ = saveState();

        Q_FOREACH (DashboardDock* d, dLst) {
            if (d->widget() == w) {
                d->setVisible(true);
            }
            else {
                auto* dw = static_cast<DashboardWidget*>(d->widget());
                Q_ASSERT(dw);
                dw->resetMaximised();
                d->setVisible(false);
            }
        }
    }
    // restore the previous state
    else {
        resetMaximised();
    }
}

bool Dashboard::hasMaximised() const {
    for (int i = 0; i < widgets_.count(); i++) {
        if (widgets_[i]->isMaximised()) {
            return true;
        }
    }
    return false;
}

bool Dashboard::hasMaximisedApplied() const {
    QList<DashboardDock*> dLst = findChildren<DashboardDock*>(QString());
    Q_FOREACH (DashboardDock* d, dLst) {
        if (d->isVisible() == false) {
            return true;
        }
    }
    return false;
}

void Dashboard::resetMaximised() {
    QList<DashboardDock*> dLst = findChildren<DashboardDock*>(QString());
    Q_FOREACH (DashboardDock* d, dLst) {
        auto* dw = static_cast<DashboardWidget*>(d->widget());
        Q_ASSERT(dw);
        dw->resetMaximised();
        d->setVisible(true);
    }

    if (!savedDockState_.isEmpty()) {
        restoreState(savedDockState_);
        savedDockState_.clear();
    }
}

void Dashboard::checkMaximisedState() {
    bool st = (widgets_.count() > 1);
    for (int i = 0; i < widgets_.count(); i++) {
        widgets_.at(i)->setEnableMaximised(st);
    }
}

//------------------------
// Dialogs
//------------------------

void Dashboard::slotDialogFinished(int) {
    if (auto* dia = static_cast<DashboardDialog*>(sender())) {
        disconnect(this, nullptr, dia->dashboardWidget(), nullptr);
        popupWidgets_.removeOne(dia->dashboardWidget());
    }
}

void Dashboard::slotDialogClosed() {
    if (auto* dia = static_cast<DashboardDialog*>(sender())) {
        disconnect(this, nullptr, dia->dashboardWidget(), nullptr);
        popupWidgets_.removeOne(dia->dashboardWidget());
    }
}

//------------------------
// Rescan
//------------------------

void Dashboard::reload() {
    for (int i = 0; i < widgets_.count(); i++) {
        widgets_.at(i)->reload();
    }
}

void Dashboard::rerender() {
    for (int i = 0; i < widgets_.count(); i++) {
        widgets_.at(i)->rerender();
    }

    titleHandler_->updateTitle();
}

//------------------------
// ViewMode
//------------------------

Viewer::ViewMode Dashboard::viewMode() {
    // return handler_->currentMode();
    return Viewer::TreeViewMode;
}

void Dashboard::setViewMode(Viewer::ViewMode /*mode*/) {
    // handler_->setCurrentMode(mode);
}

//----------------------------------
// Save and load settings!!
//----------------------------------

void Dashboard::writeSettings(VComboSettings* vs) {
    serverFilter_->writeSettings(vs);

    // Qt settings
    if (savedDockState_.isEmpty() == false) {
        vs->putQs("state", savedDockState_);
    }
    else {
        vs->putQs("state", saveState());
    }

    // Other setting
    vs->put("widgetCount", findChildren<QDockWidget*>().count());

    for (int i = 0; i < widgets_.count(); i++) {
        std::string id = Dashboard::widgetSettingsId(i);
        vs->beginGroup(id);
        widgets_.at(i)->writeSettings(vs);
        vs->endGroup();
    }
}

void Dashboard::readSettings(VComboSettings* vs) {
    settingsAreRead_ = true;

    // This will create the ServerHandler objects
    serverFilter_->readSettings(vs);

    // At this point each ServerHandler is running its reset()!

    Q_FOREACH (QWidget* w, findChildren<QDockWidget*>()) {
        UiLog().dbg() << "DashBoard::readSettings() dock: " << w->objectName();
    }

    // Read the information about the dashboard widgets.
    auto cnt = vs->get<int>("widgetCount", 0);
    for (int i = 0; i < cnt; i++) {
        std::string id = Dashboard::widgetSettingsId(i);
        if (vs->contains(id)) {
            vs->beginGroup(id);
            auto type   = vs->get<std::string>("type", "");
            auto dockId = vs->get<std::string>("dockId", "");

            // Create a dashboard widget
            if (DashboardWidget* dw = addWidget(type, dockId, false)) {
                // This will make the widgets active!!! The ServerHandler reset() can
                // be still running at this point!
                dw->readSettings(vs);
            }
            vs->endGroup();
        }
    }

    // Restore state of the dockwidgets. It will not create the dockwidgets
    // themselves but set their states and geometry. This is based on
    // the the dockwidgets's objectname, so that has to be unique. We need to call
    // it when the dockwidgets have already been created.
    if (vs->containsQs("state")) {
        restoreState(vs->getQs("state").toByteArray());
    }

    initialSelectionInView();

    settingsAreRead_ = false;
}

#if 0
void Dashboard::slotInfoPanelSelection(VInfo_ptr info)
{
    selectInTreeView(info);
}
#endif

void Dashboard::initialSelectionInView() {
    Q_FOREACH (DashboardWidget* w, widgets_) {
        if (w->initialSelectionInView()) {
            return;
        }
    }
}

bool Dashboard::selectInTreeView(VInfo_ptr info) {
    if (!info) {
        return false;
    }

    Q_FOREACH (DashboardWidget* w, widgets_) {
        if (w->type() == "tree") {
            w->setCurrentSelection(info);
            return serverFilter_->isFiltered(info->server());
        }
    }

    return false;
}

VInfo_ptr Dashboard::currentSelectionInView() {
    Q_FOREACH (DashboardWidget* w, widgets_) {
        VInfo_ptr info = w->currentSelection();
        if (info && info.get()) {
            return info;
        }
    }

    return {};
}

// Find an unique id for a new dockwidget
QString Dashboard::uniqueDockId() {
    QSet<QString> ids;
    Q_FOREACH (QDockWidget* dw, findChildren<QDockWidget*>()) {
        ids << dw->objectName();
    }

    for (unsigned i = 0; i < 1000; i++) {
        QString uid = "dock_" + QString::number(i);
        if (!ids.contains(uid)) {
            return uid;
        }
    }

    // We should handle this situation!!
    assert(0);

    return "dock_1000";
}

std::string Dashboard::widgetSettingsId(int i) {
    return "widget_" + ecf::convert_to<std::string>(i);
}

void Dashboard::notifyServerFilterAdded(ServerItem* /*item*/) {
    if (!settingsAreRead_) {
        // If there are no views we automatically add a tree view
        if (widgets_.count() == 0) {
            addWidget("tree");
        }

        Q_EMIT contentsChanged();
    }
}

void Dashboard::notifyServerFilterRemoved(ServerItem* /*item*/) {
    if (!settingsAreRead_) {
        Q_EMIT contentsChanged();
    }
}

void Dashboard::notifyServerFilterChanged(ServerItem*) {
    if (!settingsAreRead_) {
        Q_EMIT contentsChanged();
    }
}

void Dashboard::notifyServerFilterDelete() {
    // if(!settingsAreRead_)
    //	Q_EMIT contentsChanged();
}

// We need to do this in order to prevent the dock window context menu from popping up.
// See ECFLOW-894
void Dashboard::contextMenuEvent(QContextMenuEvent* e) {
    e->accept();
}
