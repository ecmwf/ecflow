//============================================================================
// Copyright 2014 ECMWF. 
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

#include "Defs.hpp"
#include "ClientInvoker.hpp"

#include "MainWindow.hpp"
#include "FilterWidget.hpp"
#include "InfoPanel.hpp"
#include "NodePathWidget.hpp"
#include "NodePanel.hpp"
#include "ServerHandler.hpp"
#include "ServerListDialog.hpp"
#include "MenuConfigDialog.hpp"
#include "UserMessage.hpp"
#include "VConfig.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

//Initialise static variables
bool MainWindow::quitStarted_=false;
QList<MainWindow*> MainWindow::windows_;

MainWindow::MainWindow(QStringList idLst,QWidget *parent) : QMainWindow(parent)
{
    setupUi(this);
    
    setAttribute(Qt::WA_DeleteOnClose);

    //Create action group for view mode
    //The order of the actions in the group must reflect the order in
    //the Viewer::NodeViewMode enum!!!!
    viewModeAg_=new QActionGroup(this);
    viewModeAg_->setExclusive(true);
    viewModeAg_->addAction(actionTreeView);
    viewModeAg_->addAction(actionTableView);

    connect(viewModeAg_,SIGNAL(triggered(QAction*)),
    		this,SLOT(slotViewMode(QAction*)));


    //Filter widget in toolbar
    //filterWidget_=new FilterWidget(this);
    //viewToolBar->addWidget(filterWidget_);

    NodePathWidget* pathWidget=new NodePathWidget(this);
    toolBar->addWidget(pathWidget);


    //Create the main layout
    QVBoxLayout* layout=new QVBoxLayout();
    QWidget *w=new QWidget(this);
    w->setLayout(layout);
    setCentralWidget(w);

    //QSplitter *sp=new QSplitter(Qt::Vertical,this);
    //layout->addWidget(sp);

    //View menu
    stateFilterMenu_=new StateFilterMenu(menuState);
    attrFilterMenu_=new AttributeFilterMenu(menuType);
    iconFilterMenu_=new IconFilterMenu(menuIcon);

    //Servers menu menu
    serverFilterMenu_=new ServerFilterMenu(menuServer);

    //Create a node panel
    nodePanel_=new NodePanel(this);
    layout->addWidget(nodePanel_);//sp->addWidget(nodePanel_);
    
    connect(nodePanel_,SIGNAL(currentWidgetChanged()),
    		this,SLOT(slotCurrentChangedInPanel()));

    connect(nodePanel_,SIGNAL(selectionChanged(ViewNodeInfo_ptr)),
       		pathWidget,SLOT(setPath(ViewNodeInfo_ptr)));

    connect(pathWidget,SIGNAL(selected(ViewNodeInfo_ptr)),
       		nodePanel_,SLOT(slotSelection(ViewNodeInfo_ptr)));

}


void MainWindow::init(MainWindow *win)
{
  	if(!win)
	  	return;

	//actionIconSidebar_->setChecked(browser->actionIconSidebar_->isChecked());
	//actionDrawers_->setChecked(browser->actionDrawers_->isChecked());
	//actionStatusbar_->setChecked(browser->actionStatusbar_->isChecked());
}

InfoPanel* MainWindow::addInfoPanel()
{
	//Dock widget at the bottom
    InfoPanelDock *dw = new InfoPanelDock(tr("Info panel"), this);

    //Find a unique objectName
    QString dname="dock_" + QString::number(findChildren<QDockWidget*>().count()-1);
    dw->setObjectName(dname);

    addDockWidget(Qt::BottomDockWidgetArea, dw);

    connect(nodePanel_,SIGNAL(selectionChanged(ViewNodeInfo_ptr)),
    		dw->infoPanel(),SLOT(slotReload(ViewNodeInfo_ptr)));

    connect(dw->infoPanel(),SIGNAL(selectionChanged(ViewNodeInfo_ptr)),
    		nodePanel_,SLOT(slotSelection(ViewNodeInfo_ptr)));

    return dw->infoPanel();
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
	ServerHandler::updateAll();
}

void MainWindow::on_actionReset_triggered()
{
	ServerHandler::updateAll();
	MainWindow::reload();
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
	ServerFilter *filter=0;
	if(VConfig *config=	nodePanel_->config())
	{
		filter=config->serverFilter();
	}

	ServerListDialog dialog(ServerListDialog::SelectionMode,filter,this);
	dialog.exec();
}

//void MainWindow::on_actionAddInfoPanel_triggered()
//{
//	addInfoPanel();
//}

void MainWindow::on_actionShowInInfoPanel_triggered()
{
	QList<QDockWidget*> dockLst=findChildren<QDockWidget*>();

	//If there is a visible non-detached panel it already shows the needed info
	foreach(QDockWidget* dw,dockLst)
	{
		if(dw->isVisible())
			if(InfoPanel* ip=static_cast<InfoPanel*>(dw->widget()))
				if(!ip->detached())
				{
					ip->slotReload(nodePanel_->currentSelection());
					return;
				}
	}


	foreach(QDockWidget* dw,dockLst)
	{
		if(!dw->isVisible())
		{
			if(InfoPanel* ip=static_cast<InfoPanel*>(dw->widget()))
			{
				ip->reset(nodePanel_->currentSelection());
				ip->detached(true);
				dw->show();
				return;
			}
		}
	}

	//Add a detached panel otherwise
	InfoPanel* ip=addInfoPanel();
	ip->reset(nodePanel_->currentSelection());
	ip->detached(true);

}

void MainWindow::slotViewMode(QAction* action)
{
	if(action->isChecked())
	{
		int index=viewModeAg_->actions().indexOf(action);
		if(index >=0)
			nodePanel_->setViewMode(static_cast<Viewer::ViewMode>(index));
	}
}

void MainWindow::slotCurrentChangedInPanel()
{
	syncViewModeAg(nodePanel_->viewMode());
	//filterWidget_->reload(nodePanel_->viewFilter());

	if(VConfig *config=	nodePanel_->config())
	{
		stateFilterMenu_->reload(config->stateFilter());
		attrFilterMenu_->reload(config->attributeFilter());
		iconFilterMenu_->reload(config->iconFilter());

		serverFilterMenu_->reload(config);
	}

	//breadcrumbs_->setPath(folderPanel_->currentFolder());
  	 //slotUpdateNavigationActions(folderPanel_->folderNavigation());

	 //updateIconSizeActionState();
	 //updateSearchPanel();
}

//This functions syncs the toggle state of the action group
void MainWindow::syncViewModeAg(Viewer::ViewMode mode)
{
	//It is safe to call setChecked() because the QActionGroup in this case
	//does not emit the triggered() signal

	int index=static_cast<int>(mode);
	if(index >=0 && index < viewModeAg_->actions().count())
		viewModeAg_->actions().at(index)->setChecked(true);
}

void MainWindow::reloadContents()
{
	nodePanel_->reload();
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
		//deleteLater();
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

void MainWindow::save(boost::property_tree::ptree &pt,QSettings &qs)
{
	qs.setValue("geometry",saveGeometry());
	qs.setValue("state",saveState());
	qs.setValue("minimized",(windowState() & Qt::WindowMinimized)?1:0);

	pt.put("infoPanelCount",findChildren<QDockWidget*>().count());

	nodePanel_->save(pt);
}

void MainWindow::load(const boost::property_tree::ptree& winPt,QSettings &qs)
{
	int cnt=winPt.get<int>("infoPanelCount",0);
	for(int i=0; i < cnt ; i++)
	{
		addInfoPanel();
	}

	nodePanel_->load(winPt);

	if(qs.value("minimized").toInt()== 1)
	{
	  	setWindowState(windowState() | Qt::WindowMinimized);
	}

	restoreGeometry(qs.value("geometry").toByteArray());
	restoreState(qs.value("state").toByteArray());
}


//====================================================
//
// Static methods
//
//====================================================

MainWindow* MainWindow::makeWindow(const boost::property_tree::ptree& winPt,QSettings& qs)
{
   	MainWindow* win=MainWindow::makeWindow();
	win->load(winPt,qs);
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
  	foreach(MainWindow *win,windows_)
		win->show();
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
	QSettings qs("ECMWF","ecFlowView");

	int cnt=0;
	int topWinId=-1;

	std::string fs = DirectoryHandler::concatenate(DirectoryHandler::configDir(), "session.json");

	//Parse param file using the boost JSON property tree parser
	using boost::property_tree::ptree;

	ptree pt;
	ptree pt_empty;

	try
	{
		boost::property_tree::json_parser::read_json(fs,pt);
	}
	catch (const boost::property_tree::json_parser::json_parser_error& e)
	{
		 std::string errorMessage = e.what();
		 UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse JSON session file : " + errorMessage));

		 MainWindow::makeWindow(pt_empty,qs);
		 return;
	}

	cnt=pt.get<int>("windowCount",0);
	topWinId=pt.get<int>("topWindowId",-1);

	if(cnt > 25)
	{
		cnt=25;
	}

	std::string winPattern("window_");
	for(int i=0; i < cnt; i++)
	{
		if(i != topWinId)
		{
			std::string id=winPattern + boost::lexical_cast<std::string>(i);
			ptree::const_assoc_iterator it=pt.find(id.c_str());
			if(it != pt.not_found())
			{
				const ptree &winPt = it->second;
				qs.beginGroup(QString::fromStdString(id));
				MainWindow::makeWindow(winPt,qs);
				qs.endGroup();
			}
		}
	}

	if(topWinId != -1)
	{
		std::string id=winPattern + boost::lexical_cast<std::string>(topWinId);
		ptree::const_assoc_iterator it=pt.find(id.c_str());
		if(it != pt.not_found())
		{
			const ptree &winPt = it->second;
			qs.beginGroup(QString::fromStdString(id));
			MainWindow::makeWindow(winPt,qs);
			qs.endGroup();
		}
	}

	if(windows_.count() == 0)
	{
		MainWindow::makeWindow(pt_empty,qs);
	}
}

void MainWindow::save(MainWindow *topWin)
{

	//Define qt settings
	QSettings qs("ECMWF","ecFlowView");

	//We have to clear it so that not to remember all the previous windows
	qs.clear();

	//Define json file
	std::string fs = DirectoryHandler::concatenate(DirectoryHandler::configDir(), "session.json");

	//Generate property tree
	boost::property_tree::ptree pt;

	//Add total window number and id of active window
	pt.put("windowCount",windows_.count());
	pt.put("topWindowId",windows_.indexOf(topWin));

	//Save info for all the windows
	for(int i=0; i < windows_.count(); i++)
	{
		std::string id="window_"+boost::lexical_cast<std::string>(i);

		boost::property_tree::ptree ptWin;

		qs.beginGroup(QString::fromStdString(id));
		windows_.at(i)->save(ptWin,qs);
		qs.endGroup();

		pt.add_child(id,ptWin);
	}

	//Write to json
	write_json(fs,pt);
}

void MainWindow::reload()
{
	foreach(MainWindow *w,windows_)
	{
		w->reloadContents();
	}
}

void MainWindow::saveSession(SessionItem* s)
{



}

MainWindow* MainWindow::findWindow(QWidget *childW)
{
  	foreach(MainWindow *w,windows_)
	{
	  	if(static_cast<QWidget*>(w) == childW->window())
			return w;
	}

	return 0;
}









