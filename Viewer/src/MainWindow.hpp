#ifndef MAINWINDOW_HPP_
#define MAINWINDOW_HPP_

//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//============================================================================

#include <QMainWindow>
#include <QSettings>

#include "ui_MainWindow.h"

#include "VInfo.hpp"
#include "VProperty.hpp"

#include <boost/property_tree/ptree.hpp>

class QActionGroup;
class QLabel;
class QToolButton;
class InfoPanel;
class NodePanel;
class ServerRefreshInfoWidget;
class ServerFilterMenu;
class SessionItem;
class VComboSettings;
class PropertyMapper;

class MainWindowTitleHandler : public VPropertyObserver
{
public:
   MainWindowTitleHandler(QMainWindow*);
   ~MainWindowTitleHandler();
   void update();
   void notifyChange(VProperty*);

protected:
   QMainWindow* win_;
   PropertyMapper* prop_;
};

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow(QStringList,QWidget *parent=0);
    ~MainWindow();
    
    static void init();
    static void showWindows();
    static void openWindow(QString id,QWidget *fromW=0);
    static void openWindow(QStringList id,QWidget *fromW=0);
    static void reload();
    static void saveSession(SessionItem*);
    static void changeNotifySelectionChanged(VInfo_ptr);

protected Q_SLOTS:
	void on_actionNewTab_triggered();
	void on_actionNewWindow_triggered();
	void on_actionClose_triggered();
	void on_actionQuit_triggered();
	void on_actionRefresh_triggered();
	void on_actionReset_triggered();
	void on_actionRefreshSelected_triggered();
	void on_actionResetSelected_triggered();
	void on_actionConfigureNodeMenu_triggered();
	void on_actionManageServers_triggered();
	void on_actionAddTreeWidget_triggered();
	void on_actionAddTableWidget_triggered();
	void on_actionAddInfoPanel_triggered();
	void on_actionPreferences_triggered();
	void on_actionSearch_triggered();
    void on_actionNotification_triggered();
	void on_actionAbout_triggered();
	void on_actionSaveSessionAs_triggered();
	void on_actionManageSessions_triggered();

	void slotCurrentChangedInPanel();
	void slotSelectionChanged(VInfo_ptr);
	void slotOpenInfoPanel();
	void slotConfigChanged();
	void slotContentsChanged();
    void slotServerSyncNotify(bool);
    void slotEditServerSettings(ServerHandler* s);

private:
    void init(MainWindow*);
    void closeEvent(QCloseEvent*);
    void addInfoPanelActions(QToolBar *toolbar);
    void reloadContents();
    void rerenderContents();
    bool selectInTreeView(VInfo_ptr info);
    void updateRefreshActions();
    void hideServerSyncNotify();
    void constructWindowTitle();
    void cleanUpOnQuit();

    void writeSettings(VComboSettings*);
    void readSettings(VComboSettings*);

    static MainWindow* makeWindow(VComboSettings* vs);
    static MainWindow *makeWindow();
    static MainWindow *makeWindow(QString id);
    static MainWindow *makeWindow(QStringList idLst);
    static bool aboutToClose(MainWindow*);
    static bool aboutToQuit(MainWindow*);
    static void save(MainWindow *);
    static void saveContents(MainWindow *);
    static MainWindow* findWindow(QWidget *childW);
    static void configChanged(MainWindow *);
    static void hideServerSyncNotify(MainWindow*);
    static void cleanUpOnQuit(MainWindow *);

    ServerFilterMenu* serverFilterMenu_;
    NodePanel* nodePanel_;
    QList<QAction*> infoPanelActions_;
    VInfo_ptr selection_;
    QToolButton* serverSyncNotifyTb_;
    ServerRefreshInfoWidget *serverComWidget_;
    MainWindowTitleHandler* winTitle_;

    static bool quitStarted_;
    static QList<MainWindow*> windows_;
    static int maxWindowNum_;
};

#endif 
