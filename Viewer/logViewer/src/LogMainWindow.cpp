//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QSocketNotifier>
#include <QSplitter>
#include <QToolBar>
#include <QVBoxLayout>

#include "LogMainWindow.hpp"

#include "DirectoryHandler.hpp"
//#include "AboutDialog.hpp"
//#include "ChangeNotify.hpp"
//#include "ChangeNotifyWidget.hpp"
//#include "ClockWidget.hpp"
//#include "FilterWidget.hpp"
//#include "InfoPanel.hpp"
//#include "InfoPanelHandler.hpp"
//#include "MenuConfigDialog.hpp"
//#include "NodePathWidget.hpp"
//#include "NodePanel.hpp"
//#include "PropertyDialog.hpp"
//#include "ServerComInfoWidget.hpp"
//#include "ServerHandler.hpp"
//#include "ServerList.hpp"
//#include "ServerListDialog.hpp"
//#include "ServerListSyncWidget.hpp"
//#include "SessionHandler.hpp"
//#include "SaveSessionAsDialog.hpp"
//#include "CommandOutputDialog.hpp"
#include "LocalSocketServer.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
//#include "VConfig.hpp"
//#include "VIcon.hpp"
//#include "VSettings.hpp"
#include "Version.hpp"
//#include "WidgetNameProvider.hpp"

#include "LogLoadWidget.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "ui_LogMainWindow.h"

bool LogMainWindow::quitStarted_=false;
QList<LogMainWindow*> LogMainWindow::windows_;
int LogMainWindow::maxWindowNum_=25;
LocalSocketServer* LogMainWindow::socketServer_=NULL;


LogMainWindow::LogMainWindow(QStringList idLst,QWidget *parent) :
    QMainWindow(parent), ui_(new Ui::LogMainWindow)
{
    ui_->setupUi(this);

    //Assigns name to each object
    //setObjectName("win_" + QString::number(windows_.count()));

    setAttribute(Qt::WA_DeleteOnClose);

    //the window title    
    std::string title = "ecflowUI - LogViewer (" + ecf::Version::raw() + ")";
    setWindowTitle(QString::fromStdString(title));

    //Create the main layout
    QVBoxLayout* layout=new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    QWidget *w=new QWidget(this);
    w->setObjectName("c");
    w->setLayout(layout);
    setCentralWidget(w);

    //Create a node panel
    tab_=new QTabWidget(this);
    tab_->setTabsClosable(true);
    tab_->setTabBarAutoHide(true);
    layout->addWidget(tab_);

    textView_=new QPlainTextEdit(this);
    layout->addWidget(textView_);

    //LogLoadWidget *lw=new LogLoadWidget(this);
   // tab_->addTab(lw,"first");
    //lw->load("/home/graphics/cgr/ecflow_dev/ecflow-metab.5062.ecf.log");
    //lw->load("/home/graphics/cgr/ecflow_dev/vsms1.ecf.log");

    connect(ui_->actionExit,SIGNAL(triggered()),
            this,SLOT(slotQuit()));

    connect(ui_->actionClose,SIGNAL(triggered()),
                    this,SLOT(slotClose()));

    //--------------
    // Toolbar
    //--------------


    //--------------
    // Status bar
    //--------------

    //Add clock widget
    //clockWidget_=new ClockWidget(this);
    //statusBar()->addPermanentWidget(clockWidget_);

    if(!socketServer_)
    {
        socketServer_=new LocalSocketServer("log",NULL);
        connect(socketServer_,SIGNAL(messageReceived(QString)),
                this,SLOT(slotMessageReceived(QString)));

        std::string socketPath=DirectoryHandler::socketDir();
        textView_->appendPlainText("socket: "  +QString::fromStdString(socketPath));
        textView_->appendPlainText("server: " + socketServer_->serverName());
    }
}

LogMainWindow::~LogMainWindow()
{
    UiLog().dbg() << "MainWindow --> destructor";
    //delete winTitle_;
}

void LogMainWindow::init(LogMainWindow *win)
{
    //nodePanel_->init();

    if(!win)
        return;
}

//==============================================================
//
//  File menu
//
//==============================================================

void LogMainWindow::addNewTab(QString serverName,QString host, QString port,QString logFile)
{
    LogLoadWidget *lw=new LogLoadWidget(this);

    QFileInfo fInfo(logFile);
    if(!fInfo.exists())
        return;

    tab_->addTab(lw,fInfo.fileName());

    lw->load(serverName,host,port,logFile);
    //lw->load("/home/graphics/cgr/ecflow_dev/ecflow-metab.5062.ecf.log");
    //lw->load("/home/graphics/cgr/ecflow_dev/vsms1.ecf.log");
}

void LogMainWindow::slotNewTab()
{
    //nodePanel_->slotNewTab();
}

void LogMainWindow::slotMessageReceived(QString msg)
{
    textView_->appendPlainText(msg);

    QStringList lst=msg.split("::");
    if(lst.count() == 4)
    {
        addNewTab(lst[0],lst[1],lst[2],lst[3]);
    }
    else if(msg == "exit" || msg == "close")
         close();

}

void LogMainWindow::slotClose()
{
    close();
}

void LogMainWindow::slotQuit()
{
    LogMainWindow::aboutToQuit(this);
}

//==============================================================
//
//  Close and quit
//
//==============================================================

void LogMainWindow::closeEvent(QCloseEvent* event)
{
    if(LogMainWindow::aboutToClose(this))
    {
        windows_.removeOne(this);
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

//On quitting we need to call the destructor of all the servers shown in the gui.
//This will guarantee that the server log out is properly done.
//Unfortunately when we quit qt does not call the destructor of the mainwindows.
//We tried to explicitely delete the mainwindows here but it caused a crash on the
//leap42 system. So here we only delete the nodePanel in the mainwindow. This panel
//contains all the tabs. Each tab contains a serverfilter and when the serverfilters
//get deleted in the end the destructors of the servers will be called.
void LogMainWindow::cleanUpOnQuit()
{
    Q_ASSERT(quitStarted_==true);
    //serverFilterMenu_->aboutToDestroy();
}

//====================================================
//
// Read/write settings
//
//====================================================

#if 0
void LogMainWindow::writeSettings(VComboSettings *vs)
{
    //Qt settings
    vs->putQs("geometry",saveGeometry());
    vs->putQs("state",saveState());

//See ECFLOW-1090
#if 0
    vs->putQs("minimized",(windowState() & Qt::WindowMinimized)?1:0);
#endif

    //Other setting
    vs->put("infoPanelCount",findChildren<QDockWidget*>().count());

    //Saves nodePanel
    nodePanel_->writeSettings(vs);
}
#endif

#if 0
void LogMainWindow::readSettings(VComboSettings *vs)
{
    int cnt=vs->get<int>("infoPanelCount",0);
    for(int i=0; i < cnt ; i++)
    {
        //addInfoPanel();
    }

    nodePanel_->readSettings(vs);

    //See ECFLOW-1090
#if 0
    if(vs->getQs("minimized").toInt()== 1)
    {
        setWindowState(windowState() | Qt::WindowMinimized);
    }
#endif

    if(vs->containsQs("geometry"))
        restoreGeometry(vs->getQs("geometry").toByteArray());

    if(vs->containsQs("state"))
        restoreState(vs->getQs("state").toByteArray());
}
#endif

//====================================================
//
// Static methods
//
//====================================================

/*LogMainWindow* LogMainWindow::makeWindow(VComboSettings* vs)
{
    LogMainWindow* win=LogMainWindow::makeWindow();
    win->readSettings(vs);
    return win;
}
*/

LogMainWindow* LogMainWindow::makeWindow()
{
    QStringList idLst;
    return LogMainWindow::makeWindow(idLst);
}

LogMainWindow* LogMainWindow::makeWindow(QString id)
{
    QStringList idLst;
    idLst << id;
    return LogMainWindow::makeWindow(idLst);
}

LogMainWindow* LogMainWindow::makeWindow(QStringList idLst)
{
    LogMainWindow *win=new LogMainWindow(idLst);
    windows_ << win;
    return win;
}

void LogMainWindow::openWindow(QString id,QWidget *fromW)
{
    LogMainWindow* win=LogMainWindow::makeWindow(id);
    win->init(findWindow(fromW));
    win->show();
}

void LogMainWindow::openWindow(QStringList idLst,QWidget *fromW)
{
    LogMainWindow* win=LogMainWindow::makeWindow(idLst);
    win->init(findWindow(fromW));
    win->show();
}

void LogMainWindow::showWindows()
{
    Q_FOREACH(LogMainWindow *win,windows_)
        win->show();
}

void LogMainWindow::cleanUpOnQuit(LogMainWindow*)
{
    Q_FOREACH(LogMainWindow *win,windows_)
        win->cleanUpOnQuit();
}

//Return true if close is allowed, false otherwise
bool LogMainWindow::aboutToClose(LogMainWindow* win)
{
    //If quit has already stared we ignore the close signal from
    //the main windows.
    if(quitStarted_)
    {
        return false;
    }

    //Otherwise
    else
    {
        if(windows_.count() > 1)
        {
#if 0
            int tabCnt=win->nodePanel_->count();
            if(tabCnt > 1)
            {
                if(QMessageBox::question(0,tr("Confirm close"),tr("You are about to close <b>") + QString::number(tabCnt) + tr("</b> tabs. Are you sure you want to continue?"),
                          QMessageBox::Yes | QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Cancel)
                {
                    return false;
                }
            }
#endif
        }
        else if(windows_.count() == 1)
        {
            return LogMainWindow::aboutToQuit(win);
        }
        return true;
    }

     return true;
}

bool LogMainWindow::aboutToQuit(LogMainWindow* topWin)
{
#if 0
    if(QMessageBox::question(0,tr("Confirm quit"),
                 tr("Do you want to quit ") +
                 QString::fromStdString(VConfig::instance()->appName()) + "?",
                 QMessageBox::Yes | QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Yes)
    {
#endif
        quitStarted_=true;

#if 0
        //Save browser settings
        LogMainWindow::save(topWin);

        // handle session cleanup
        // temporary sessions can be saved or deleted
        SessionItem *si = SessionHandler::instance()->current();
        if (si->temporary())
        {
            if (si->askToPreserveTemporarySession())
            {
                if(QMessageBox::question(0,tr("Delete temporary session?"),
                            tr("This was a temporary session - would you like to preserve it for future use?"),
                            QMessageBox::Yes | QMessageBox::No,QMessageBox::No) == QMessageBox::No)
                    SessionHandler::destroyInstance();
            }
            else  // if askToPreserveTemporarySession() is false, then we assume we want to delete
            {
                SessionHandler::destroyInstance();
            }
        }
#endif

        //Ensure the ServerHandler destructors are called
        LogMainWindow::cleanUpOnQuit(topWin);

        //Exit ecFlowView
        QApplication::quit();
#if 0
    }
#endif

    return false;
}

void LogMainWindow::init()
{
#if 0
    SessionItem* cs=SessionHandler::instance()->current();
    assert(cs);

    VComboSettings vs(cs->sessionFile(),cs->windowFile());

    //Read configuration. If it fails we create an empty window!!
    if(!vs.read())
    {
         MainWindow::makeWindow(&vs);
         return;
    }

    //Get number of windows and topWindow index.
    int cnt=vs.get<int>("windowCount",0);
    int topWinId=vs.get<int>("topWindowId",-1);

    if(cnt > maxWindowNum_)
    {
        cnt=maxWindowNum_;
    }

    //Create all windows (except the topWindow) in a loop. The
    //topWindow should be created last so that it should always appear on top.
    std::string winPattern("window_");
    for(int i=0; i < cnt; i++)
    {
        if(i != topWinId)
        {
            std::string id=winPattern + boost::lexical_cast<std::string>(i);
            if(vs.contains(id))
            {
                vs.beginGroup(id);
                MainWindow::makeWindow(&vs);
                vs.endGroup();
            }
        }
    }

    //Create the topwindow
    if(topWinId != -1)
    {
        std::string id=winPattern + boost::lexical_cast<std::string>(topWinId);
        if(vs.contains(id))
        {
            vs.beginGroup(id);
            MainWindow::makeWindow(&vs);
            vs.endGroup();
        }
    }

    //If now windows were created we need to create an empty one
    if(windows_.count() == 0)
    {
        MainWindow::makeWindow(&vs);
    }

#endif

    if(windows_.count() == 0)
    {
        LogMainWindow::makeWindow();
    }

}

void LogMainWindow::save(LogMainWindow *topWin)
{
#if 0
    MainWindow::saveContents(topWin);

    //Save global config
    VConfig::instance()->saveSettings();

    //Save server list
    ServerHandler::saveSettings();

    //Save icon name list
    VIcon::saveLastNames();
#endif
}

void LogMainWindow::saveContents(LogMainWindow *topWin)
{
#if 0
    SessionItem* cs=SessionHandler::instance()->current();
    assert(cs);

    VComboSettings vs(cs->sessionFile(),cs->windowFile());

    //We have to clear it so that not to remember all the previous windows
    vs.clear();

    //Add total window number and id of active window
    vs.put("windowCount",windows_.count());
    vs.put("topWindowId",windows_.indexOf(topWin));

    //Save info for all the windows
    for(int i=0; i < windows_.count(); i++)
    {
        std::string id="window_"+boost::lexical_cast<std::string>(i);
        vs.beginGroup(id);
        windows_.at(i)->writeSettings(&vs);
        vs.endGroup();
    }

    //Write to json
    vs.write();
#endif
}


void LogMainWindow::reload()
{
    /*Q_FOREACH(MainWindow *w,windows_)
    {
        w->reloadContents();
    }*/
}

#if 0
void MainWindow::saveSession(SessionItem* s)
{

}
#endif

LogMainWindow* LogMainWindow::findWindow(QWidget *childW)
{
    Q_FOREACH(LogMainWindow *w,windows_)
    {
        if(static_cast<QWidget*>(w) == childW->window())
            return w;
    }

    return 0;
}

LogMainWindow* LogMainWindow::firstWindow()
{
    return (!windows_.isEmpty())?(windows_[0]):NULL;
}
