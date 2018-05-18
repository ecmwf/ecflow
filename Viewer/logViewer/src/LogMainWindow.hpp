#ifndef LOGMAINWINDOW_HPP
#define LOGMAINWINDOW_HPP

//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <QMainWindow>
#include <QSettings>

//#include "ui_LogMainWindow.h"

//#include "VInfo.hpp"

#include <boost/property_tree/ptree.hpp>

class QActionGroup;
class QLabel;
class QPlainTextEdit;

class QToolButton;
class QTabWidget;
class ClockWidget;
class InfoPanel;
class NodePanel;
class ServerRefreshInfoWidget;
class ServerFilterMenu;
class SessionItem;
class VComboSettings;
class LocalSocketServer;

class MainWindow;

#if 0
class MainWindowTitleHandler
{
   friend class MainWindow;
public:
   MainWindowTitleHandler(MainWindow*);
   ~MainWindowTitleHandler();

   void update();

protected:
   void update(ServerHandler*);

   MainWindow* win_;
};
#endif

namespace Ui {
    class LogMainWindow;
}

class LogMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    LogMainWindow(QStringList,QWidget *parent=0);
    ~LogMainWindow();

    void addNewTab(QString name,QString host, QString port,QString logFile);

    static void init();
    static void showWindows();
    static void openWindow(QString id,QWidget *fromW=0);
    static void openWindow(QStringList id,QWidget *fromW=0);
    static LogMainWindow *makeWindow();
    static void reload();
    //static void saveSession(SessionItem*);
    //static void lookUpInTree(VInfo_ptr);
    //static void startPreferences(QString);
    //static void updateMenuMode(ServerHandler*);
    static LogMainWindow* firstWindow();

protected Q_SLOTS:
    void slotNewTab();
    void slotMessageReceived(QString msg);
    void slotClose();
    void slotQuit();

private:
    void init(LogMainWindow*);
    void closeEvent(QCloseEvent*);

    void reloadContents();
    void rerenderContents();

    void cleanUpOnQuit();

    //void writeSettings(VComboSettings*);
    //void readSettings(VComboSettings*);

    //static MainWindow* makeWindow(VComboSettings* vs);
    //static LogMainWindow *makeWindow();
    static LogMainWindow *makeWindow(QString id);
    static LogMainWindow *makeWindow(QStringList idLst);
    static bool aboutToClose(LogMainWindow*);
    static bool aboutToQuit(LogMainWindow*);
    static void save(LogMainWindow *);
    static void saveContents(LogMainWindow *);
    static LogMainWindow* findWindow(QWidget *childW);
    //static void configChanged(MainWindow *);
    //static void hideServerSyncNotify(MainWindow*);
    static void cleanUpOnQuit(LogMainWindow *);
    //static void startPreferences(MainWindow *,QString);

    Ui::LogMainWindow* ui_;
    //MainWindowTitleHandler* winTitle_;
    QTabWidget* tab_;
    //ClockWidget* clockWidget_;
    QPlainTextEdit* textView_;

    static bool quitStarted_;
    static QList<LogMainWindow*> windows_;
    static int maxWindowNum_;

    static LocalSocketServer* socketServer_;
};

#endif // LOGMAINWINDOW_HPP
