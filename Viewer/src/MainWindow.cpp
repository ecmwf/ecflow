//============================================================================
// Copyright 2009-2017 ECMWF.
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
#include <QDockWidget>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QSplitter>
#include <QToolBar>
#include <QVBoxLayout>

#include "MainWindow.hpp"

#include "AboutDialog.hpp"
#include "ChangeNotifyWidget.hpp"
#include "FilterWidget.hpp"
#include "InfoPanel.hpp"
#include "InfoPanelHandler.hpp"
#include "MenuConfigDialog.hpp"
#include "NodePathWidget.hpp"
#include "NodePanel.hpp"
#include "PropertyDialog.hpp"
#include "ServerHandler.hpp"
#include "ServerList.hpp"
#include "ServerListDialog.hpp"
#include "ServerListSyncWidget.hpp"
#include "SessionHandler.hpp"
#include "SaveSessionAsDialog.hpp"
#include "UiLog.hpp"
#include "VConfig.hpp"
#include "VIcon.hpp"
#include "VSettings.hpp"
#include "Version.hpp"
#include "WidgetNameProvider.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

bool MainWindow::quitStarted_=false;
QList<MainWindow*> MainWindow::windows_;
int MainWindow::maxWindowNum_=25;

MainWindow::MainWindow(QStringList idLst,QWidget *parent) :
    QMainWindow(parent),
    serverSyncNotifyTb_(0)
{
    setupUi(this);
    
    //Assigns name to each object
    setObjectName("win_" + QString::number(windows_.count()));

    setAttribute(Qt::WA_DeleteOnClose);

    constructWindowTitle();

    //Create the main layout
    QVBoxLayout* layout=new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    QWidget *w=new QWidget(this);
    w->setObjectName("c");
    w->setLayout(layout);
    setCentralWidget(w);

    //Servers menu menu
    serverFilterMenu_=new ServerFilterMenu(menuServer);

    //Create a node panel
    nodePanel_=new NodePanel(this);
    layout->addWidget(nodePanel_);
    
    connect(nodePanel_,SIGNAL(currentWidgetChanged()),
    		this,SLOT(slotCurrentChangedInPanel()));

    connect(nodePanel_,SIGNAL(selectionChanged(VInfo_ptr)),
    			this,SLOT(slotSelectionChanged(VInfo_ptr)));

    connect(nodePanel_,SIGNAL(contentsChanged()),
    	    this,SLOT(slotContentsChanged()));

    //Add temporary preview label
   /* QLabel *label=new QLabel(" This is a preview version and has not been verified for operational use! ",this);
    label->setAutoFillBackground(true);
    label->setProperty("previewLabel","1");

    QLabel *label1=new QLabel("      ",this);

    viewToolBar->addWidget(label1);
    viewToolBar->addWidget(label);*/

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    viewToolBar->addWidget(spacer);

    //QToolBar* ipToolBar=new QToolBar(this);
    addInfoPanelActions(viewToolBar);
    //addToolBar(ipToolBar);

    //Actions based on selection
    actionRefreshSelected->setEnabled(false);
    actionResetSelected->setEnabled(false);

    //Status bar

    //Add server list sync notification
    if(ServerList::instance()->hasSyncChange())
    {
        //Add server list sync notification
        serverSyncNotifyTb_=new QToolButton(this);
        serverSyncNotifyTb_->setAutoRaise(true);
        serverSyncNotifyTb_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        serverSyncNotifyTb_->setIcon(QPixmap(":/viewer/info.svg"));
        serverSyncNotifyTb_->setText("Server list updated");
        serverSyncNotifyTb_->setToolTip("Your local copy of the <b>system server list</b> was updated. Click to see the changes.");
        statusBar()->addWidget(serverSyncNotifyTb_);

        connect(serverSyncNotifyTb_,SIGNAL(clicked(bool)),
            this,SLOT(slotServerSyncNotify(bool)));
    }

    //Add notification widget
    ChangeNotifyWidget* chw=new ChangeNotifyWidget(this);
    statusBar()->addPermanentWidget(chw);

    //Assigns name to each object
    WidgetNameProvider::nameChildren(this);

    //actionSearch->setVisible(false);
}

MainWindow::~MainWindow()
{
    UiLog().dbg() << "MainWindow --> desctutor";
	serverFilterMenu_->aboutToDestroy();
}

void MainWindow::init(MainWindow *win)
{
	nodePanel_->init();

	if(!win)
	  	return;
}

void MainWindow::addInfoPanelActions(QToolBar *toolbar)
{
   for(std::vector<InfoPanelDef*>::const_iterator it=InfoPanelHandler::instance()->panels().begin();
           it != InfoPanelHandler::instance()->panels().end(); ++it)
   {
	   if((*it)->show().find("toolbar") != std::string::npos)
	   {
		   QAction *ac=toolbar->addAction(QString::fromStdString((*it)->label()));
           QPixmap pix(":/viewer/" + QString::fromStdString((*it)->icon()));
           ac->setObjectName(QString::fromStdString((*it)->label()));
           ac->setIcon(QIcon(pix));
		   ac->setData(QString::fromStdString((*it)->name()));
		   ac->setToolTip(QString::fromStdString((*it)->tooltip()));

		   connect(ac,SIGNAL(triggered()),
				   this,SLOT(slotOpenInfoPanel()));

		   infoPanelActions_ << ac;
	   }
   }
}


void MainWindow::constructWindowTitle()
{
    char *userTitle = getenv("ECFUI_TITLE");
    std::string mainTitle = (userTitle != NULL) ? std::string(userTitle) + " (" + ecf::Version::raw() + ")"
                                                : VConfig::instance()->appLongName();

    // add the name of the session to the title bar?
    std::string sessionName = SessionHandler::instance()->current()->name();
    if (sessionName == "default")
        sessionName = "";
    else
        sessionName = " (session: " + sessionName + ")";

    setWindowTitle(QString::fromStdString(mainTitle) + "  -  Preview version" + QString::fromStdString(sessionName));
}


//==============================================================
//
//  File menu
//
//==============================================================

void MainWindow::on_actionNewTab_triggered()
{
	nodePanel_->slotNewTab();
}

void MainWindow::on_actionNewWindow_triggered()
{
	MainWindow::openWindow("",this);
}

void MainWindow::on_actionClose_triggered()
{
	close();
}

void MainWindow::on_actionQuit_triggered()
{
	MainWindow::aboutToQuit(this);
}

void MainWindow::on_actionRefresh_triggered()
{
	nodePanel_->refreshCurrent();
}

void MainWindow::on_actionReset_triggered()
{
	nodePanel_->resetCurrent();
}

void MainWindow::on_actionRefreshSelected_triggered()
{
	if(selection_ && selection_.get())
	{
		if(ServerHandler* s=selection_->server())
		{
			s->refresh();
		}
	}
}

void MainWindow::on_actionResetSelected_triggered()
{
	if(selection_ && selection_.get())
	{
		if(ServerHandler* s=selection_->server())
		{
			s->reset();
		}
	}
}

void MainWindow::on_actionPreferences_triggered()
{
    PropertyDialog* d=new PropertyDialog; //belongs to the whole app

    connect(d,SIGNAL(configChanged()),
    		this,SLOT(slotConfigChanged()));

	if(d->exec() == QDialog::Accepted)
	{
		if(d->isConfigChanged())
		{
			configChanged(this);
		}
    }

	delete d;
}

void MainWindow::on_actionManageSessions_triggered()
{
	QMessageBox::information(0, tr("Manage Sessions"),
		tr("To manage sessions, please restart ecFlowUI with the -s command-line option"));
}

void MainWindow::slotConfigChanged()
{
	configChanged(this);
}

void MainWindow::on_actionConfigureNodeMenu_triggered()
{
    MenuConfigDialog menuConfigDialog;
    
	if(menuConfigDialog.exec() == QDialog::Accepted)
	{
    }
}

void MainWindow::on_actionSearch_triggered()
{
    //It takes ownership of the dialogue.
    nodePanel_->addSearchDialog();
}

void MainWindow::on_actionManageServers_triggered()
{
	ServerListDialog dialog(ServerListDialog::SelectionMode,nodePanel_->serverFilter(),this);
	dialog.exec();
}

void MainWindow::on_actionAddTreeWidget_triggered()
{
	nodePanel_->addToDashboard("tree");
}

void MainWindow::on_actionAddTableWidget_triggered()
{
	nodePanel_->addToDashboard("table");
}

void MainWindow::on_actionAddInfoPanel_triggered()
{
	nodePanel_->addToDashboard("info");
}

void MainWindow::on_actionShowInInfoPanel_triggered()
{
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog d;
    d.exec();
}

void MainWindow::on_actionSaveSessionAs_triggered()
{
    SaveSessionAsDialog d;
    d.exec();
}


void MainWindow::slotCurrentChangedInPanel()
{
	slotSelectionChanged(nodePanel_->currentSelection());

	//filterWidget_->reload(nodePanel_->viewFilter());

	serverFilterMenu_->reload(nodePanel_->serverFilter());

	//breadcrumbs_->setPath(folderPanel_->currentFolder());
  	 //slotUpdateNavigationActions(folderPanel_->folderNavigation());

	 //updateIconSizeActionState();
	 //updateSearchPanel();
}

void MainWindow::slotSelectionChanged(VInfo_ptr info)
{
	selection_=info;

	std::vector<InfoPanelDef*> ids;
	InfoPanelHandler::instance()->visible(selection_,ids);

	Q_FOREACH(QAction* ac,infoPanelActions_)
	{
		ac->setEnabled(false);

		std::string name=ac->data().toString().toStdString();

        for(std::vector<InfoPanelDef*>::const_iterator it=ids.begin(); it != ids.end(); ++it)
		{
			 if((*it)->name() == name)
			 {
				ac->setEnabled(true);
				break;
			 }
		}
	}

	updateRefreshActions();
}

void MainWindow::updateRefreshActions()
{
	QString serverName;
	if(selection_ && selection_.get())
	{
		if(ServerHandler* s=selection_->server())
		{
			serverName=QString::fromStdString(s->name());
		}
	}

	bool hasSel=(selection_ && selection_.get());
	actionRefreshSelected->setEnabled(hasSel);
	actionResetSelected->setEnabled(hasSel);

	if(serverName.isEmpty())
	{
		QString tnew=tr("Refresh <b>selected</b> server<br>") +
					 + "<code>" + actionRefreshSelected->shortcut().toString() + "</code>";

		actionRefreshSelected->setToolTip(tnew);
	}
	else
	{
		QString t=actionRefreshSelected->toolTip();
		if(!t.contains(serverName))
		{
			QString tnew=tr("Refresh server <b>") + serverName + tr("</b><br>") +
			 + "<code>" + actionRefreshSelected->shortcut().toString() + "</code>";

			actionRefreshSelected->setToolTip(tnew);
		}
	}
}


void MainWindow::slotOpenInfoPanel()
{
	if(QAction* ac=static_cast<QAction*>(sender()))
	{
		std::string name=ac->data().toString().toStdString();
		nodePanel_->openDialog(selection_,name);
	}
}

void MainWindow::reloadContents()
{
	nodePanel_->reload();
}

//Rerender all the views and breadcrumbs
void MainWindow::rerenderContents()
{
	nodePanel_->rerender();
}

void MainWindow::slotContentsChanged()
{
	MainWindow::saveContents(NULL);
}

bool MainWindow::selectInTreeView(VInfo_ptr info)
{
    return nodePanel_->selectInTreeView(info);
}

void MainWindow::slotServerSyncNotify(bool)
{
    if(serverSyncNotifyTb_)
    {
        serverSyncNotifyTb_->hide();
        MainWindow::hideServerSyncNotify(this);

        ServerListDialog dialog(ServerListDialog::SelectionMode,nodePanel_->serverFilter(),this);
        dialog.showSysSyncLog();
        dialog.exec();
    }
}

void MainWindow::hideServerSyncNotify()
{
   if(serverSyncNotifyTb_)
      serverSyncNotifyTb_->hide();
}

//==============================================================
//
//  Close and quit
//
//==============================================================

void MainWindow::closeEvent(QCloseEvent* event)
{
  	if(MainWindow::aboutToClose(this))
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
void MainWindow::cleanUpOnQuit()
{
    Q_ASSERT(quitStarted_==true);
    serverFilterMenu_->aboutToDestroy();
    delete nodePanel_;
}

//====================================================
//
// Read/write settings
//
//====================================================

void MainWindow::writeSettings(VComboSettings *vs)
{
	//Qt settings
	vs->putQs("geometry",saveGeometry());
	vs->putQs("state",saveState());
	vs->putQs("minimized",(windowState() & Qt::WindowMinimized)?1:0);

	//Other setting
	vs->put("infoPanelCount",findChildren<QDockWidget*>().count());

	//Saves nodePanel
	nodePanel_->writeSettings(vs);
}

void MainWindow::readSettings(VComboSettings *vs)
{
	int cnt=vs->get<int>("infoPanelCount",0);
	for(int i=0; i < cnt ; i++)
	{
		//addInfoPanel();
	}

	nodePanel_->readSettings(vs);

	if(vs->getQs("minimized").toInt()== 1)
	{
	  	setWindowState(windowState() | Qt::WindowMinimized);
	}

	if(vs->containsQs("geometry"))
		restoreGeometry(vs->getQs("geometry").toByteArray());

	if(vs->containsQs("state"))
		restoreState(vs->getQs("state").toByteArray());
}

//====================================================
//
// Static methods
//
//====================================================

MainWindow* MainWindow::makeWindow(VComboSettings* vs)
{
	MainWindow* win=MainWindow::makeWindow();
	win->readSettings(vs);
	return win;
}

MainWindow* MainWindow::makeWindow()
{
	QStringList idLst;
	return MainWindow::makeWindow(idLst);
}

MainWindow* MainWindow::makeWindow(QString id)
{
	QStringList idLst;
	idLst << id;
	return MainWindow::makeWindow(idLst);
}

MainWindow* MainWindow::makeWindow(QStringList idLst)
{
	MainWindow *win=new MainWindow(idLst);
	windows_ << win;
	return win;
}

void MainWindow::openWindow(QString id,QWidget *fromW)
{
  	MainWindow* win=MainWindow::makeWindow(id);
	win->init(findWindow(fromW));
	win->show();
}

void MainWindow::openWindow(QStringList idLst,QWidget *fromW)
{
	MainWindow* win=MainWindow::makeWindow(idLst);
	win->init(findWindow(fromW));
	win->show();
}

void MainWindow::showWindows()
{
  	Q_FOREACH(MainWindow *win,windows_)
		win->show();
}

void MainWindow::configChanged(MainWindow*)
{
	Q_FOREACH(MainWindow *win,windows_)
			win->rerenderContents();
}

void MainWindow::changeNotifySelectionChanged(VInfo_ptr info)
{
    Q_FOREACH(MainWindow *win,windows_)
        if(win->selectInTreeView(info))
            return;
}

void MainWindow::hideServerSyncNotify(MainWindow*)
{
    Q_FOREACH(MainWindow *win,windows_)
        win->hideServerSyncNotify();
}

void MainWindow::cleanUpOnQuit(MainWindow*)
{
    Q_FOREACH(MainWindow *win,windows_)
        win->cleanUpOnQuit();
}

//Return true if close is allowed, false otherwise
bool MainWindow::aboutToClose(MainWindow* win)
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
		  	int tabCnt=win->nodePanel_->count();
			if(tabCnt > 1)
			{
		  		if(QMessageBox::question(0,tr("Confirm close"),tr("You are about to close <b>") + QString::number(tabCnt) + tr("</b> tabs. Are you sure you want to continue?"),
			    	      QMessageBox::Yes | QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Cancel)
				{
					return false;
				}
			}
		}
		else if(windows_.count() == 1)
		{
            return MainWindow::aboutToQuit(win);
		}
		return true;
	}
}

bool MainWindow::aboutToQuit(MainWindow* topWin)
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
		MainWindow::save(topWin);

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

        //Ensure the ServerHandler destructors are called
        MainWindow::cleanUpOnQuit(topWin);

		//Exit ecFlowView
		QApplication::quit();
#if 0
    }
#endif

	return false;
}

void MainWindow::init()
{
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
}

void MainWindow::save(MainWindow *topWin)
{
	MainWindow::saveContents(topWin);

	//Save global config
	VConfig::instance()->saveSettings();

    //Save server list
	ServerHandler::saveSettings();

    //Save icon name list
    VIcon::saveLastNames();
}

void MainWindow::saveContents(MainWindow *topWin)
{
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
}


void MainWindow::reload()
{
	Q_FOREACH(MainWindow *w,windows_)
	{
		w->reloadContents();
	}
}

void MainWindow::saveSession(SessionItem* s)
{

}

MainWindow* MainWindow::findWindow(QWidget *childW)
{
  	Q_FOREACH(MainWindow *w,windows_)
	{
	  	if(static_cast<QWidget*>(w) == childW->window())
			return w;
	}

	return 0;
}









