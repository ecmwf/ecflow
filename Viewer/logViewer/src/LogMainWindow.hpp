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

namespace Ui {
    class LogMainWindow;
}

class LogMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    LogMainWindow(QStringList,QWidget *parent=0);
    ~LogMainWindow();

    void init();
    void addNewTab(QString name,QString host, QString port,QString logFile,bool forced=false);
    void addNewTab(QString name);


    static void showWindows();
    //static void openWindow(QString id,QWidget *fromW=0);
    //static void openWindow(QStringList id,QWidget *fromW=0);
    static LogMainWindow *makeWindow();
    static void reload();
    //static void saveSession(SessionItem*);
    //static void lookUpInTree(VInfo_ptr);
    //static void startPreferences(QString);
    //static void updateMenuMode(ServerHandler*);
    static LogMainWindow* firstWindow();

protected Q_SLOTS:
    void slotAddFile();
    void slotReplaceFile();
    void slotNewTab();
    void slotCloseTab(int idx);
    void slotMessageReceived(QString msg);
    void slotClose();
    void slotQuit();

private:
    void closeEvent(QCloseEvent*);

    //void reloadContents();
    //void rerenderContents();
    QString userSelectFile();
    QStringList loadedFiles() const;

    void cleanUpOnQuit();
    void setInitialSize(int w, int h);
    void writeSettings(QSettings&);
    void readSettings(QSettings&);

    //static MainWindow* makeWindow(VComboSettings* vs);
    //static LogMainWindow *makeWindow();
    static QString qsFile(QString);
    static LogMainWindow *makeWindow(QString id);
    static LogMainWindow *makeWindow(QStringList idLst);
    static bool aboutToClose(LogMainWindow*);
    static bool aboutToQuit(LogMainWindow*);
    static void save(LogMainWindow *);
    static LogMainWindow* findWindow(QWidget *childW);
    //static void configChanged(MainWindow *);
    //static void hideServerSyncNotify(MainWindow*);
    static void cleanUpOnQuit(LogMainWindow *);
    //static void startPreferences(MainWindow *,QString);

    Ui::LogMainWindow* ui_;
    QTabWidget* tab_;
    QPlainTextEdit* textView_;

    static bool quitStarted_;
    static QList<LogMainWindow*> windows_;
    static int maxWindowNum_;

    static LocalSocketServer* socketServer_;
};

#endif // LOGMAINWINDOW_HPP
