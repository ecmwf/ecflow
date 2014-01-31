#ifndef MAINWINDOW_HPP_
#define MAINWINDOW_HPP_

//============================================================================
// Copyright 2014 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//============================================================================

#include <QMainWindow>
#include <QSettings>
#include <QTreeWidget>

#include "ui_MainWindow.h"

#include "Defs.hpp"

class NodePanel;

class MainWindow : public QMainWindow, private Ui::MainWindow  
{
    Q_OBJECT

public:
    MainWindow(QStringList,QWidget *parent=0);
    
    //void printDefTree(const std::string &server, int port);
    //void printNode(node_ptr node, int indent, QTreeWidgetItem *parent);

    static void init();
    static void showWindows();
    static void openWindow(QString id,QWidget *fromW=0);
    static void openWindow(QStringList id,QWidget *fromW=0);

protected slots:
	void on_actionNewTab_triggered();
	void on_actionNewWindow_triggered();
	void on_actionClose_triggered();
	void on_actionQuit_triggered();

private:
    void init(MainWindow*);
    void closeEvent(QCloseEvent*);
    void writeSettings(QSettings &settings);
    void readSettings(QSettings &settings);

    static MainWindow *makeWindow(QSettings&);
    static MainWindow *makeWindow();
    static MainWindow *makeWindow(QString id);
    static MainWindow *makeWindow(QStringList idLst);
    static bool aboutToClose(MainWindow*);
    static bool aboutToQuit(MainWindow*);
    static void save(MainWindow *);
    static MainWindow* findWindow(QWidget *childW);

    NodePanel* nodePanel_;

    static bool quitStarted_;
    static QList<MainWindow*> windows_;
};

#endif 
