#ifndef MAINWINDOW_HPP_
#define MAINWINDOW_HPP_

//============================================================================
// Copyright 2015 ECMWF.
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

#include <boost/property_tree/ptree.hpp>

class QActionGroup;
class InfoPanel;
class NodePanel;
class ServerFilterMenu;
class SessionItem;
class VComboSettings;

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

protected Q_SLOTS:
	void on_actionNewTab_triggered();
	void on_actionNewWindow_triggered();
	void on_actionClose_triggered();
	void on_actionQuit_triggered();
	void on_actionRefresh_triggered();
	void on_actionReset_triggered();
	void on_actionConfigureNodeMenu_triggered();
	void on_actionManageServers_triggered();
	void on_actionShowInInfoPanel_triggered();
	void on_actionAddTreeWidget_triggered();
	void on_actionAddTableWidget_triggered();
	void on_actionAddInfoPanel_triggered();
	void on_actionPreferences_triggered();
	void on_actionSearch_triggered();
	void on_actionAbout_triggered();

	void slotCurrentChangedInPanel();
	void slotSelectionChanged(VInfo_ptr);
	void slotOpenInfoPanel();
	void slotConfigChanged();

private:
    void init(MainWindow*);
    void closeEvent(QCloseEvent*);
    void addInfoPanelActions(QToolBar *toolbar);
    void reloadContents();
    void rerenderContents();

    void writeSettings(VComboSettings*);
    void readSettings(VComboSettings*);

    static MainWindow* makeWindow(VComboSettings* vs);
    static MainWindow *makeWindow();
    static MainWindow *makeWindow(QString id);
    static MainWindow *makeWindow(QStringList idLst);
    static bool aboutToClose(MainWindow*);
    static bool aboutToQuit(MainWindow*);
    static void save(MainWindow *);
    static MainWindow* findWindow(QWidget *childW);
    static void configChanged(MainWindow *);

    ServerFilterMenu* serverFilterMenu_;
    NodePanel* nodePanel_;
    QList<QAction*> infoPanelActions_;
    VInfo_ptr selection_;

    static bool quitStarted_;
    static QList<MainWindow*> windows_;
    static int maxWindowNum_;
};

#endif 
