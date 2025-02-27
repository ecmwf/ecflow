/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "MainWindow.hpp"

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QDockWidget>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QSplitter>
#include <QToolBar>
#include <QVBoxLayout>
#include <boost/property_tree/json_parser.hpp>

#include "AboutDialog.hpp"
#include "ChangeNotify.hpp"
#include "ChangeNotifyWidget.hpp"
#include "ClockWidget.hpp"
#include "CommandOutputDialog.hpp"
#include "ConnectState.hpp"
#include "FilterWidget.hpp"
#include "InfoPanel.hpp"
#include "InfoPanelHandler.hpp"
#include "LogViewerCom.hpp"
#include "MenuConfigDialog.hpp"
#include "NodePanel.hpp"
#include "NodePathWidget.hpp"
#include "PropertyDialog.hpp"
#include "SaveSessionAsDialog.hpp"
#include "ServerComInfoWidget.hpp"
#include "ServerHandler.hpp"
#include "ServerList.hpp"
#include "ServerListDialog.hpp"
#include "ServerListSyncWidget.hpp"
#include "SessionHandler.hpp"
#include "ShortcutHelpDialog.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "VAttributeType.hpp"
#include "VConfig.hpp"
#include "VIcon.hpp"
#include "VSettings.hpp"
#include "WidgetNameProvider.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Version.hpp"

bool MainWindow::quitStarted_ = false;
QList<MainWindow*> MainWindow::windows_;
int MainWindow::maxWindowNum_ = 25;

#ifdef ECFLOW_LOGVIEW
LogViewerCom* MainWindow::logCom_ = nullptr;
#endif

MainWindow::MainWindow(QStringList /*idLst*/, QWidget* parent) : QMainWindow(parent), serverSyncNotifyTb_(nullptr) {
    setupUi(this);

    // Assigns name to each object
    setObjectName("win_" + QString::number(windows_.count()));

    setAttribute(Qt::WA_DeleteOnClose);

    // the window title
    winTitle_ = new MainWindowTitleHandler(this);
    winTitle_->update();

    // Create the main layout
    auto* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    auto* w = new QWidget(this);
    w->setObjectName("c");
    w->setLayout(layout);
    setCentralWidget(w);

    // Servers menu menu
    serverFilterMenu_ = new ServerFilterMenu(menuServer);

    // Create a node panel
    nodePanel_ = new NodePanel(this);
    layout->addWidget(nodePanel_);

    connect(nodePanel_, SIGNAL(currentWidgetChanged()), this, SLOT(slotCurrentChangedInPanel()));

    connect(nodePanel_, SIGNAL(selectionChangedInCurrent(VInfo_ptr)), this, SLOT(slotSelectionChanged(VInfo_ptr)));

    connect(nodePanel_, SIGNAL(contentsChanged()), this, SLOT(slotContentsChanged()));

    //--------------
    // Toolbar
    //--------------

    // Add server refresh widget to the front of the toolbar
    serverComWidget_ = new ServerRefreshInfoWidget(actionRefreshSelected, this);
    Q_ASSERT(actionSearch);
    viewToolBar->insertWidget(actionSearch, serverComWidget_);
    // viewToolBar->addWidget(serverComWidget_);

    connect(serverComWidget_,
            SIGNAL(serverSettingsEditRequested(ServerHandler*)),
            this,
            SLOT(slotEditServerSettings(ServerHandler*)));

    // insert a spacer after the the server refresh widget
    auto* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // viewToolBar->insertWidget(actionSearch,spacer);
    viewToolBar->addWidget(spacer);

    // Add more actions
    addInfoPanelActions(viewToolBar);

    // Add shortcuts to action tooltips
    Viewer::addShortCutToToolTip(viewToolBar->actions());

    // Initialise actions based on selection
    actionRefreshSelected->setEnabled(false);
    actionResetSelected->setEnabled(false);

    //--------------
    // Status bar
    //--------------

    // Add server list sync notification
    serverSyncNotifyTb_ = new QToolButton(this);
    serverSyncNotifyTb_->setAutoRaise(true);
    serverSyncNotifyTb_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    serverSyncNotifyTb_->setIcon(QPixmap(":/viewer/info.svg"));
    serverSyncNotifyTb_->setText("Server list updated");
    serverSyncNotifyTb_->setToolTip(
        "Your local copy of the <b>system server list</b> was updated. Click to see the changes.");
    statusBar()->addWidget(serverSyncNotifyTb_);
    serverSyncNotifyTb_->hide();
    connect(serverSyncNotifyTb_, SIGNAL(clicked(bool)), this, SLOT(slotServerSyncNotify(bool)));

    initServerSyncTbInternal();

    // Add notification widget
    auto* chw = new ChangeNotifyWidget(this);
    statusBar()->addPermanentWidget(chw);

    // Add clock widget
    clockWidget_ = new ClockWidget(this);
    statusBar()->addPermanentWidget(clockWidget_);

    // serverComWidget_=new ServerComLineDisplay(this);
    // statusBar()->addPermanentWidget(serverComWidget_);

    // Assigns name to each object
    WidgetNameProvider::nameChildren(this);

    // actionSearch->setVisible(false);
}

MainWindow::~MainWindow() {
    UiLog().dbg() << "MainWindow --> destructor";
    delete winTitle_;
    if (!quitStarted_) {
        serverFilterMenu_->aboutToDestroy();
    }
}

void MainWindow::init(MainWindow* win) {
    nodePanel_->init();

    if (!win)
        return;
}

void MainWindow::addInfoPanelActions(QToolBar* toolbar) {
    for (auto it : InfoPanelHandler::instance()->panels()) {
        if (it->show().find("toolbar") != std::string::npos) {
            QAction* ac = toolbar->addAction(QString::fromStdString(it->label()));
            QPixmap pix(":/viewer/" + QString::fromStdString(it->icon()));
            ac->setObjectName(QString::fromStdString(it->label()));
            ac->setIcon(QIcon(pix));
            ac->setData(QString::fromStdString(it->name()));
            ac->setToolTip(QString::fromStdString(it->buttonTooltip()));

            connect(ac, SIGNAL(triggered()), this, SLOT(slotOpenInfoPanel()));

            infoPanelActions_ << ac;
        }
    }
}

ServerHandler* MainWindow::selectedServer() const {
    return (selection_) ? (selection_->server()) : nullptr;
}

//==============================================================
//
//  File menu
//
//==============================================================

void MainWindow::on_actionNewTab_triggered() {
    nodePanel_->slotNewTab();
}

void MainWindow::on_actionNewWindow_triggered() {
    MainWindow::openWindow("", this);
}

void MainWindow::on_actionClose_triggered() {
    close();
}

void MainWindow::on_actionQuit_triggered() {
    MainWindow::aboutToQuit(this);
}

void MainWindow::on_actionRefresh_triggered() {
    nodePanel_->refreshCurrent();
}

void MainWindow::on_actionReset_triggered() {
    nodePanel_->resetCurrent();
}

void MainWindow::on_actionRefreshSelected_triggered() {
    if (selection_ && selection_.get()) {
        if (ServerHandler* s = selection_->server()) {
            s->refresh();
        }
    }
}

void MainWindow::on_actionResetSelected_triggered() {
    if (selection_ && selection_.get()) {
        if (ServerHandler* s = selection_->server()) {
            s->reset();
        }
    }
}

void MainWindow::on_actionPreferences_triggered() {
    startPreferences(this, "");
}

void MainWindow::on_actionManageSessions_triggered() {
    QMessageBox::information(nullptr,
                             tr("Manage Sessions"),
                             tr("To manage sessions, please restart ecFlowUI with the -s command-line option"));
}

void MainWindow::slotConfigChanged() {
    configChanged(this);
}

void MainWindow::on_actionConfigureNodeMenu_triggered() {
    MenuConfigDialog menuConfigDialog;

    if (menuConfigDialog.exec() == QDialog::Accepted) {}
}

void MainWindow::on_actionSearch_triggered() {
    // It takes ownership of the dialogue.
    nodePanel_->addSearchDialog();
}

void MainWindow::on_actionNotification_triggered() {
    ChangeNotify::showDialog();
}

void MainWindow::on_actionCommandOutput_triggered() {
    CommandOutputDialog::showDialog();
}

void MainWindow::on_actionManageServers_triggered() {
    ServerListDialog dialog(ServerListDialog::SelectionMode, nodePanel_->serverFilter(), this);
    dialog.exec();
}

void MainWindow::on_actionAddTreeWidget_triggered() {
    nodePanel_->addToDashboard("tree");
}

void MainWindow::on_actionAddTableWidget_triggered() {
    nodePanel_->addToDashboard("table");
}

void MainWindow::on_actionAddInfoPanel_triggered() {
    nodePanel_->addToDashboard("info");
}

void MainWindow::on_actionAbout_triggered() {
    AboutDialog d;
    d.exec();
}

void MainWindow::on_actionShortcutHelp_triggered() {
    ShortcutHelpDialog d;
    d.exec();
}

void MainWindow::on_actionSaveSessionAs_triggered() {
    SaveSessionAsDialog d;
    d.exec();
}

void MainWindow::slotCurrentChangedInPanel() {
    slotSelectionChanged(nodePanel_->currentSelection());

    // filterWidget_->reload(nodePanel_->viewFilter());

    serverFilterMenu_->reload(nodePanel_->serverFilter());

    // breadcrumbs_->setPath(folderPanel_->currentFolder());
    // slotUpdateNavigationActions(folderPanel_->folderNavigation());

    // updateIconSizeActionState();
    // updateSearchPanel();
}

// The selection changed in one of the views
void MainWindow::slotSelectionChanged(VInfo_ptr info) {
    selection_ = info;

    // Get the set of visible info panel tabs for the selection
    std::vector<InfoPanelDef*> ids;
    if (info && info->isAttribute()) {
        InfoPanelHandler::instance()->visible(VInfo::createParent(info), ids);
    }
    else {
        InfoPanelHandler::instance()->visible(selection_, ids);
    }

    // Set status of the info panel actions in the toolbar accordingly
    Q_FOREACH (QAction* ac, infoPanelActions_) {
        ac->setEnabled(false);

        std::string name = ac->data().toString().toStdString();

        for (std::vector<InfoPanelDef*>::const_iterator it = ids.begin(); it != ids.end(); ++it) {
            if ((*it)->name() == name) {
                ac->setEnabled(true);
                break;
            }
        }
    }

    // Update the refres action/info to the selection
    updateRefreshActions();

    // Update the window titlebar
    winTitle_->update();
}

void MainWindow::updateRefreshActions() {
    ServerHandler* s = nullptr;
    if (selection_) {
        s = selection_->server();
    }

    serverComWidget_->setServer(s);

    if (s && s->isEnabled()) {
        bool hasSel = (selection_ != nullptr);
        actionRefreshSelected->setEnabled(hasSel);
        actionResetSelected->setEnabled(hasSel);
    }
    else {
        actionRefreshSelected->setEnabled(false);
        actionResetSelected->setEnabled(false);
    }
}

void MainWindow::slotOpenInfoPanel() {
    if (auto* ac = static_cast<QAction*>(sender())) {
        std::string name = ac->data().toString().toStdString();
        nodePanel_->openDialog(selection_, name);
    }
}

void MainWindow::reloadContents() {
    nodePanel_->reload();
}

// Rerender all the views and breadcrumbs
void MainWindow::rerenderContents() {
    nodePanel_->rerender();
}

void MainWindow::slotContentsChanged() {
    MainWindow::saveContents(nullptr);
}

bool MainWindow::selectInTreeView(VInfo_ptr info) {
    return nodePanel_->selectInTreeView(info);
}

void MainWindow::initServerSyncTbInternal() {
    auto manager = ServerList::instance()->systemFileManager();
    if (manager && manager->hasInfo()) {
        if (!manager->unfetchedFiles().empty()) {
            serverSyncNotifyTb_->setIcon(QPixmap(":/viewer/error.svg"));
            serverSyncNotifyTb_->setText("System server list updated (with some errors)");
        }
        else {
            serverSyncNotifyTb_->setIcon(QPixmap(":/viewer/info.svg"));
            serverSyncNotifyTb_->setText("System server list updated");
        }
        Q_ASSERT(serverSyncNotifyTb_);
        serverSyncNotifyTb_->show();
    }
}

void MainWindow::slotServerSyncNotify(bool) {
    if (serverSyncNotifyTb_) {
        serverSyncNotifyTb_->hide();
        MainWindow::hideServerSyncNotify(this);

        ServerListDialog dialog(ServerListDialog::SelectionMode, nodePanel_->serverFilter(), this);
        dialog.showSysSyncLog();
        dialog.exec();
    }
}

void MainWindow::hideServerSyncNotify() {
    if (serverSyncNotifyTb_)
        serverSyncNotifyTb_->hide();
}

void MainWindow::slotEditServerSettings(ServerHandler* s) {
    VInfo_ptr info = VInfoServer::create(s);
    nodePanel_->openDialog(info, "server_settings");
}

//==============================================================
//
//  Close and quit
//
//==============================================================

void MainWindow::closeEvent(QCloseEvent* event) {
    UiLog().dbg() << "closeEvent";
    if (!quitStarted_) {
        UiLog().dbg() << "  start close";
        if (windows_.count() == 1) {
            UiLog().dbg() << " windows_.count=1, start quit";
            MainWindow::aboutToQuit(this);
            UiLog().dbg() << "closeEvent finished";
            return;
        }
        UiLog().dbg() << "  accept close event";
        windows_.removeOne(this);
        event->accept();
    }
    UiLog().dbg() << "closeEvent finished";
}

// On quitting we need to call the destructor of all the servers shown in the gui.
// This will guarantee that the server log out is properly done.
// Unfortunately when we quit qt does not call the destructor of the mainwindows.
// We tried to explicitely delete the mainwindows here but it caused a crash on the
// leap42 system. So here we only delete the nodePanel in the mainwindow. This panel
// contains all the tabs. Each tab contains a serverfilter and when the serverfilters
// get deleted in the end the destructors of the servers will be called.
void MainWindow::cleanUpOnQuit() {
    Q_ASSERT(quitStarted_ == true);
    serverFilterMenu_->aboutToDestroy();
    delete nodePanel_;
}

//====================================================
//
// Read/write settings
//
//====================================================

void MainWindow::writeSettings(VComboSettings* vs) {
    // Qt settings
    vs->putQs("geometry", saveGeometry());
    vs->putQs("state", saveState());

// See ECFLOW-1090
#if 0
    vs->putQs("minimized",(windowState() & Qt::WindowMinimized)?1:0);
#endif

    // Other setting
    vs->put("infoPanelCount", findChildren<QDockWidget*>().count());

    // Saves nodePanel
    nodePanel_->writeSettings(vs);
}

void MainWindow::readSettings(VComboSettings* vs) {
    auto cnt = vs->get<int>("infoPanelCount", 0);
    for (int i = 0; i < cnt; i++) {
        // addInfoPanel();
    }

    nodePanel_->readSettings(vs);

    // See ECFLOW-1090
#if 0
	if(vs->getQs("minimized").toInt()== 1)
	{
	  	setWindowState(windowState() | Qt::WindowMinimized);
	}
#endif

    if (vs->containsQs("geometry"))
        restoreGeometry(vs->getQs("geometry").toByteArray());

    if (vs->containsQs("state"))
        restoreState(vs->getQs("state").toByteArray());
}

//====================================================
//
// Static methods
//
//====================================================

MainWindow* MainWindow::makeWindow(VComboSettings* vs) {
    MainWindow* win = MainWindow::makeWindow();
    win->readSettings(vs);
    return win;
}

MainWindow* MainWindow::makeWindow() {
    QStringList idLst;
    return MainWindow::makeWindow(idLst);
}

MainWindow* MainWindow::makeWindow(QString id) {
    QStringList idLst;
    idLst << id;
    return MainWindow::makeWindow(idLst);
}

MainWindow* MainWindow::makeWindow(QStringList idLst) {
    auto* win = new MainWindow(idLst);
    windows_ << win;
    return win;
}

void MainWindow::openWindow(QString id, QWidget* fromW) {
    MainWindow* win = MainWindow::makeWindow(id);
    win->init(findWindow(fromW));
    win->show();
}

void MainWindow::openWindow(QStringList idLst, QWidget* fromW) {
    MainWindow* win = MainWindow::makeWindow(idLst);
    win->init(findWindow(fromW));
    win->show();
}

void MainWindow::showWindows() {
    for (auto win : windows_)
        win->show();
}

void MainWindow::configChanged(MainWindow*) {
    Q_FOREACH (MainWindow* win, windows_)
        win->rerenderContents();
}

void MainWindow::lookUpInTree(VInfo_ptr info) {
    Q_FOREACH (MainWindow* win, windows_)
        if (win->selectInTreeView(info))
            return;
}

void MainWindow::hideServerSyncNotify(MainWindow*) {
    Q_FOREACH (MainWindow* win, windows_)
        win->hideServerSyncNotify();
}

void MainWindow::cleanUpOnQuit(MainWindow*) {
    Q_FOREACH (MainWindow* win, windows_)
        win->cleanUpOnQuit();
}

// Add server list sync notification
void MainWindow::initServerSyncTb() {
    Q_FOREACH (MainWindow* win, windows_) {
        win->initServerSyncTbInternal();
    }
}

bool MainWindow::aboutToQuit(MainWindow* topWin) {
#if 0
    if(QMessageBox::question(0,tr("Confirm quit"),
  			     tr("Do you want to quit ") +
  			     QString::fromStdString(VConfig::instance()->appName()) + "?",
			     QMessageBox::Yes | QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Yes)
	{
#endif
    quitStarted_ = true;

    // Save browser settings
    MainWindow::save(topWin);

    // handle session cleanup
    // temporary sessions can be saved or deleted
    SessionItem* si = SessionHandler::instance()->current();
    if (si->temporary()) {
        if (si->askToPreserveTemporarySession()) {
            if (QMessageBox::question(
                    nullptr,
                    tr("Delete temporary session?"),
                    tr("This was a temporary session - would you like to preserve it for future use?"),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No) == QMessageBox::No)
                SessionHandler::destroyInstance();
        }
        else // if askToPreserveTemporarySession() is false, then we assume we want to delete
        {
            SessionHandler::destroyInstance();
        }
    }

    // all qt-based classes can decect that the app is quitting
    qApp->setProperty("inQuit", "1");

    // Ensure the ServerHandler destructors are called
    MainWindow::cleanUpOnQuit(topWin);

    // Exit ecFlowView
    QApplication::quit();
#if 0
    }
#endif

    return false;
}

void MainWindow::init() {
    SessionItem* cs = SessionHandler::instance()->current();
    assert(cs);

    VComboSettings vs(cs->sessionFile(), cs->windowFile());

    // Read configuration. If it fails we create an empty window!!
    if (!vs.read(true, "No session config was found! ecFlowUI will start with an empty window.")) {
        MainWindow::makeWindow(&vs);
        return;
    }

    // Get number of windows and topWindow index.
    auto cnt      = vs.get<int>("windowCount", 0);
    auto topWinId = vs.get<int>("topWindowId", -1);

    if (cnt > maxWindowNum_) {
        cnt = maxWindowNum_;
    }

    // Create all windows (except the topWindow) in a loop. The
    // topWindow should be created last so that it should always appear on top.
    std::string winPattern("window_");
    for (int i = 0; i < cnt; i++) {
        if (i != topWinId) {
            std::string id = winPattern + ecf::convert_to<std::string>(i);
            if (vs.contains(id)) {
                vs.beginGroup(id);
                MainWindow::makeWindow(&vs);
                vs.endGroup();
            }
        }
    }

    // Create the topwindow
    if (topWinId != -1) {
        std::string id = winPattern + ecf::convert_to<std::string>(topWinId);
        if (vs.contains(id)) {
            vs.beginGroup(id);
            MainWindow::makeWindow(&vs);
            vs.endGroup();
        }
    }

    // If now windows were created we need to create an empty one
    if (windows_.count() == 0) {
        MainWindow::makeWindow(&vs);
    }
}

void MainWindow::save(MainWindow* topWin) {
    MainWindow::saveContents(topWin);

    // Save global config
    VConfig::instance()->saveSettings();

    // Save server list
    ServerHandler::saveSettings();

    // Save attribute type list
    VAttributeType::saveLastNames();

    // Save icon name list
    VIcon::saveLastNames();
}

void MainWindow::saveContents(MainWindow* topWin) {
    SessionItem* cs = SessionHandler::instance()->current();
    assert(cs);

    VComboSettings vs(cs->sessionFile(), cs->windowFile());

    // We have to clear it so that not to remember all the previous windows
    vs.clear();

    // Add total window number and id of active window
    vs.put("windowCount", windows_.count());
    vs.put("topWindowId", windows_.indexOf(topWin));

    // Save info for all the windows
    for (int i = 0; i < windows_.count(); i++) {
        std::string id = "window_" + ecf::convert_to<std::string>(i);
        vs.beginGroup(id);
        windows_.at(i)->writeSettings(&vs);
        vs.endGroup();
    }

    // Write to json
    vs.write();
}

void MainWindow::reload() {
    Q_FOREACH (MainWindow* w, windows_) {
        w->reloadContents();
    }
}

void MainWindow::saveSession(SessionItem*) {
}

MainWindow* MainWindow::findWindow(QWidget* childW) {
    Q_FOREACH (MainWindow* w, windows_) {
        if (static_cast<QWidget*>(w) == childW->window())
            return w;
    }

    return nullptr;
}

MainWindow* MainWindow::firstWindow() {
    return (!windows_.isEmpty()) ? (windows_[0]) : NULL;
}

void MainWindow::startPreferences(QString option) {
    if (windows_.count() > 0)
        startPreferences(windows_.front(), option);
}

void MainWindow::startPreferences(MainWindow* w, QString option) {
    Q_ASSERT(w);

    auto* d = new PropertyDialog; // belongs to the whole app

    d->showPage(option);

    connect(d, SIGNAL(configChanged()), w, SLOT(slotConfigChanged()));

    if (d->exec() == QDialog::Accepted) {
        if (d->isConfigChanged()) {
            configChanged(w);
        }
    }

    delete d;
}

void MainWindow::updateMenuMode(ServerHandler* sh) {
    Q_FOREACH (MainWindow* w, windows_) {
        w->winTitle_->update(sh);
    }
}

//--------------------------------------------------------
//
// MainWindowTitle (class to handle the MainWindow title)
//
//--------------------------------------------------------

MainWindowTitleHandler::MainWindowTitleHandler(MainWindow* win) : win_(win) {
    Q_ASSERT(win_);
    std::vector<std::string> propVec;
}

MainWindowTitleHandler::~MainWindowTitleHandler() = default;

void MainWindowTitleHandler::update(ServerHandler* sh) {
    Q_ASSERT(win_);
    if (sh == win_->selectedServer())
        update();
}

void MainWindowTitleHandler::update() {
    Q_ASSERT(win_);

    char* userTitle       = getenv("ECFLOWUI_TITLE");
    std::string mainTitle = (userTitle != nullptr) ? std::string(userTitle) + " (" + ecf::Version::full() + ")"
                                                   : VConfig::instance()->appLongName();
    QString title         = QString::fromStdString(mainTitle);

    if (ServerHandler* sh = win_->selectedServer()) {
        QString menuMode = sh->nodeMenuMode();
        if (!menuMode.isEmpty())
            title += " - (menu: " + menuMode + ")";
    }

    // add the name of the session to the title bar?
    QString sessionName = QString::fromStdString(SessionHandler::instance()->current()->name());
    if (sessionName != "default")
        title += " - (session: " + sessionName + ")";

    win_->setWindowTitle(title);
}
