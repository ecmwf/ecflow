//============================================================================
// Copyright 2015 ECMWF.
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
#include <QMessageBox>
#include <QSplitter>
#include <QToolBar>
#include <QVBoxLayout>

#include <QDebug>

#include "Version.hpp"


#include "MainWindow.hpp"

#include "AboutDialog.hpp"
#include "FilterWidget.hpp"
#include "InfoPanel.hpp"
#include "NodePathWidget.hpp"
#include "NodePanel.hpp"
#include "PropertyDialog.hpp"
#include "ServerHandler.hpp"
#include "ServerListDialog.hpp"
#include "MenuConfigDialog.hpp"
#include "UserMessage.hpp"
#include "VConfig.hpp"
#include "VSettings.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

bool MainWindow::quitStarted_=false;
QList<MainWindow*> MainWindow::windows_;
int MainWindow::maxWindowNum_=25;

MainWindow::MainWindow(QStringList idLst,QWidget *parent) : QMainWindow(parent)
{
    setupUi(this);
    
    setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle("EcflowUI (" + QString::fromStdString(ecf::Version::raw()) + ")");

    //Create the main layout
    QVBoxLayout* layout=new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    QWidget *w=new QWidget(this);
    w->setLayout(layout);
    setCentralWidget(w);

    //Servers menu menu
    serverFilterMenu_=new ServerFilterMenu(menuServer);

    //Create a node panel
    nodePanel_=new NodePanel(this);
    layout->addWidget(nodePanel_);
    
    connect(nodePanel_,SIGNAL(currentWidgetChanged()),
    		this,SLOT(slotCurrentChangedInPanel()));
}


MainWindow::~MainWindow()
{
	qDebug() << "exit";
}

void MainWindow::init(MainWindow *win)
{
  	if(!win)
	  	return;
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

void MainWindow::on_actionPreferences_triggered()
{
    PropertyDialog d;

	if(d.exec() == QDialog::Accepted)
	{
		if(d.configChanged())
		{
			configChanged(this);
		}
    }
}

void MainWindow::on_actionConfigureNodeMenu_triggered()
{
    MenuConfigDialog menuConfigDialog;
    
	if(menuConfigDialog.exec() == QDialog::Accepted)
	{
    }
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

void MainWindow::slotCurrentChangedInPanel()
{
	//filterWidget_->reload(nodePanel_->viewFilter());

	serverFilterMenu_->reload(nodePanel_->serverFilter());

	//breadcrumbs_->setPath(folderPanel_->currentFolder());
  	 //slotUpdateNavigationActions(folderPanel_->folderNavigation());

	 //updateIconSizeActionState();
	 //updateSearchPanel();
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

//void MainWindow::slotQuit()
//{
//	 MainWindow::aboutToQuit(this);
//}

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

void MainWindow::configChanged(MainWindow* owner)
{
	Q_FOREACH(MainWindow *win,windows_)
			win->rerenderContents();
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
  	if(QMessageBox::question(0,tr("Confirm quit"),tr("Do you want to quit ecFlowView?"),
			     QMessageBox::Yes | QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Yes)
	{
		quitStarted_=true;

		//Save browser settings
		MainWindow::save(topWin);

		//Exit ecFlowView
		QApplication::quit();
	}

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

	//Save global config
	VConfig::instance()->saveSettings();

	ServerHandler::saveSettings();


	//Save non-global config
	for(int i=0; i < windows_.count(); i++)
	{
		//windows_.at(i)->saveSettings();
	}
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









