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
#include <QFileDialog>
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
#include "LocalSocketServer.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "Version.hpp"

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

    //Initial size
    setInitialSize(1100,800);

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
    textView_->hide(); //TODO: remove textview

    //LogLoadWidget *lw=new LogLoadWidget(this);
   // tab_->addTab(lw,"first");
    //lw->load("/home/graphics/cgr/ecflow_dev/ecflow-metab.5062.ecf.log");
    //lw->load("/home/graphics/cgr/ecflow_dev/vsms1.ecf.log");

    connect(tab_,SIGNAL(tabCloseRequested(int)),
            this,SLOT(slotCloseTab(int)));

    connect(ui_->actionNewTab,SIGNAL(triggered()),
            this,SLOT(slotAddFile()));

    connect(ui_->actionReplaceLogFile,SIGNAL(triggered()),
            this,SLOT(slotReplaceFile()));

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


//==============================================================
//
//  File menu
//
//==============================================================


void LogMainWindow::addNewTab(QString serverName,QString host, QString port,QString logFile,bool forced)
{
    LogLoadWidget *lw=new LogLoadWidget(this);

    QFileInfo fInfo(logFile);
    if(!fInfo.exists() && forced)
        return;

    tab_->addTab(lw,fInfo.fileName());
    lw->load(serverName,host,port,logFile);
    //lw->load("/home/graphics/cgr/ecflow_dev/ecflow-metab.5062.ecf.log");
    //lw->load("/home/graphics/cgr/ecflow_dev/vsms1.ecf.log");
}

void LogMainWindow::addNewTab(QString logFile)
{
    LogLoadWidget *lw=new LogLoadWidget(this);

    QFileInfo fInfo(logFile);
    if(!fInfo.exists())
        return;

    tab_->addTab(lw,fInfo.fileName());
    lw->load(logFile);
    //lw->load("/home/graphics/cgr/ecflow_dev/ecflow-metab.5062.ecf.log");
    //lw->load("/home/graphics/cgr/ecflow_dev/vsms1.ecf.log");
}

void LogMainWindow::slotNewTab()
{
    //nodePanel_->slotNewTab();
}

void LogMainWindow::slotCloseTab(int idx)
{
    if(QWidget *w=tab_->widget(idx))
    {
        tab_->removeTab(idx);
        delete w;
    }
}

QString LogMainWindow::userSelectFile()
{
    QStringList fileLst=loadedFiles();
    QString startDir;
    if(fileLst.count() > 0)
    {
        startDir=fileLst.last();
        QFileInfo fi(startDir);
        startDir=fi.dir().path();
    }

    QFileDialog dialog(this,QObject::tr("Add log file to LogViewer"),startDir);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setLabelText(QFileDialog::Accept,"Ok");

    if(dialog.exec() == QDialog::Accepted)
    {
        QStringList lst=dialog.selectedFiles();
        if(lst.count() > 0)
        {
            QString f=lst[0];
            QString err;
            if(fileLst.contains(f))
            {
                QMessageBox::warning(this,"","File " +  f + " is already loaded",QMessageBox::Ok);
                return QString();
            }
            else
                return f;
        }
    }
    return QString();
}

void LogMainWindow::slotAddFile()
{
    QString f=userSelectFile();
    if(!f.isEmpty())
        addNewTab(f);
}

void LogMainWindow::slotReplaceFile()
{
    QString f=userSelectFile();
    if(!f.isEmpty())
    {
        if(LogLoadWidget *lw=static_cast<LogLoadWidget*>(tab_->widget(tab_->currentIndex())))
        {
            lw->load(f);
        }
    }
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

QStringList LogMainWindow::loadedFiles() const
{
    QStringList files;
    for(int i=0; i < tab_->count(); i++)
    {
         if(LogLoadWidget *lw=static_cast<LogLoadWidget*>(tab_->widget(i)))
            files << lw->logFile();
    }
    return files;
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

void LogMainWindow::writeSettings(QSettings& vs)
{
    //Qt settings
    vs.setValue("geometry",saveGeometry());
    vs.setValue("state",saveState());
}

void LogMainWindow::readSettings(QSettings& vs)
{
    if(vs.contains("geometry"))
        restoreGeometry(vs.value("geometry").toByteArray());

    if(vs.contains("state"))
        restoreState(vs.value("state").toByteArray());
}

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

#if 0
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
#endif
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

        //Save browser settings
        LogMainWindow::save(topWin);

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
    QSettings vs(LogMainWindow::qsFile("main"),QSettings::NativeFormat);

    vs.beginGroup("window_0");
    readSettings(vs);
    vs.endGroup();
}

void LogMainWindow::save(LogMainWindow *topWin)
{  
    QSettings vs(LogMainWindow::qsFile("main"),QSettings::NativeFormat);

    //We have to clear it so that not to remember all the previous windows
    vs.clear();

    //Add total window number and id of active window
    vs.setValue("windowCount",windows_.count());
    vs.setValue("topWindowId",windows_.indexOf(topWin));

    //Save info for all the windows
    for(int i=0; i < windows_.count(); i++)
    {
        QString id="window_"+ QString::number(i);
        vs.beginGroup(id);
        windows_.at(i)->writeSettings(vs);
        vs.endGroup();
    }
}

void LogMainWindow::reload()
{
    /*Q_FOREACH(MainWindow *w,windows_)
    {
        w->reloadContents();
    }*/
}

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

QString LogMainWindow::qsFile(QString name)
{
    std::string confDir=DirectoryHandler::logviewerConfigDir();
    std::string qsFile=DirectoryHandler::concatenate(confDir, name.toStdString() + ".conf");
    return QString::fromStdString(qsFile);
}

void LogMainWindow::setInitialSize(int w, int h)
{
    QDesktopWidget *dw=QApplication::desktop();
    QRect scg=dw->screenGeometry(dw->screenNumber());

    int wr=(scg.width()  > w) ? w : scg.width()-50;
    int hr=(scg.height() > h) ? h : scg.height()-50;

    resize(QSize(wr,hr));
}
