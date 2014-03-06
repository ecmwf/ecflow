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
#include <QMessageBox>
#include <QVBoxLayout>

#include "Defs.hpp"
#include "ClientInvoker.hpp"

#include "MainWindow.hpp"
#include "NodePanel.hpp"
#include "ServerHandler.hpp"

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

    //Create the main layout
    QVBoxLayout* layout=new QVBoxLayout();
    QWidget *w=new QWidget(this);
    w->setLayout(layout);
    setCentralWidget(w);

    //Create a node panel
    nodePanel_=new NodePanel(this);
    layout->addWidget(nodePanel_);
    
    connect(nodePanel_,SIGNAL(currentWidgetChanged()),
    		this,SLOT(slotCurrentChangedInPanel()));


    //File menu

    /*connect(actionNewTab,SIGNAL(triggered()),
    		nodePanel_,SLOT(slotNewTab()));

    connect(actionNewWindow,SIGNAL(triggered()),
    		nodePanel_,SLOT(slotNewWindow()));

    connect(actionClose,SIGNAL(triggered()),
    		this,SLOT(close()));

    connect(actionQuit,SIGNAL(triggered()),
        		this,SLOT(slotQuit()));*/
}


void MainWindow::init(MainWindow *win)
{
  	if(!win)
	  	return;

	//actionIconSidebar_->setChecked(browser->actionIconSidebar_->isChecked());
	//actionDrawers_->setChecked(browser->actionDrawers_->isChecked());
	//actionStatusbar_->setChecked(browser->actionStatusbar_->isChecked());
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
	 //breadcrumbs_->setPath(folderPanel_->currentFolder());
  	 //slotUpdateNavigationActions(folderPanel_->folderNavigation());
	 syncViewModeAg(nodePanel_->viewMode());
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

void MainWindow::writeSettings(QSettings &settings)
{
	settings.setValue("geometry",saveGeometry());
	settings.setValue("state",saveState());
	//settings.setValue("drawerSplitter",drawerSplitter_->saveState());
	//settings.setValue("mainSplitter",mainSplitter_->saveState());
	settings.setValue("minimized",(windowState() & Qt::WindowMinimized)?1:0);

	//settings.setValue("bookmarksPanel",actionSidebar_->isChecked());
	//settings.setValue("iconsPanel",actionIconSidebar_->isChecked());
	//settings.setValue("drawersStatus",actionDrawers_->isChecked());
	//settings.setValue("statusbar",actionStatusbar_->isChecked());

	nodePanel_->writeSettings(settings);
}

void MainWindow::readSettings(QSettings &settings)
{
	restoreGeometry(settings.value("geometry").toByteArray());
	restoreState(settings.value("state").toByteArray());
	//mainSplitter_->restoreState(settings.value("mainSplitter").toByteArray());
	//drawerSplitter_->restoreState(settings.value("drawerSplitter").toByteArray());

	if(settings.value("minimized").toInt()== 1)
	{
	  	setWindowState(windowState() | Qt::WindowMinimized);
	}

	/*QString viewMode=settings.value("viewMode").toString();
	if(viewMode == MvQFileBrowserWidget::DetailedViewMode)
	 	actionIconView_->trigger();
	else if(viewMode == MvQFileBrowserWidget::DetailedViewMode)
	  	actionDetailedView_->trigger();*/

	/*QVariant var=settings.value("iconSize");
	if(!var.isNull())
	{
	  	slotSetIconSizeByIndex(iconSizes_.indexOf(var.toInt()));
	}

	if(settings.value("bookmarksPanel").isNull())
	{
		actionSidebar_->setChecked(false);
	}
	else
	{
		actionSidebar_->setChecked(settings.value("bookmarksPanel").toBool());
	}

	if(settings.value("iconsPanel").isNull())
	{
		actionIconSidebar_->setChecked(false);
	}
	else
	{
		actionIconSidebar_->setChecked(settings.value("iconsPanel").toBool());
	}

	if(settings.value("drawersStatus").isNull())
	{
		actionDrawers_->setChecked(true);
	}
	else
	{
		actionDrawers_->setChecked(settings.value("drawersStatus").toBool());
	}

	if(settings.value("statusbar").isNull())
	{
		actionStatusbar_->setChecked(true);
	}
	else
	{
	  	actionStatusbar_->setChecked(settings.value("statusbar").toBool());
	}*/

	nodePanel_->readSettings(settings);
}

//====================================================
//
// Static methods
//
//====================================================

MainWindow* MainWindow::makeWindow(QSettings &settings)
{
   	MainWindow* win=MainWindow::makeWindow();
	win->readSettings(settings);
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
	//Read the settings
  	QSettings settings("ECMWF","ecFlowView");

	settings.beginGroup("main");
  	int cnt=settings.value("windowCount").toInt();
	int topWinId=settings.value("topWindowId").toInt();
	settings.endGroup();

	//MvQDesktopSettings::readSettings(settings);

	if(cnt <0 || cnt > 100)
		return;

	for(int i=0; i < cnt; i++)
	{
		if(i != topWinId)
		{
	  		settings.beginGroup("window_" + QString::number(i));
			MainWindow::makeWindow(settings);
			settings.endGroup();
		}
	}

	if(topWinId != -1)
	{
	 	settings.beginGroup("window_" + QString::number(topWinId));
		MainWindow::makeWindow(settings);
		settings.endGroup();
	}

	if(windows_.count() == 0)
	{
		MainWindow::makeWindow();
	}

}

void MainWindow::reload()
{
	foreach(MainWindow *w,windows_)
	{
		w->reloadContents();
	}
}


void MainWindow::save(MainWindow *topWin)
{
	QSettings settings("ECMWF","ecFlowView");

	//We have to clear it so that not to remember all the previous windows
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("windowCount",windows_.count());
	settings.setValue("topWindowId",windows_.indexOf(topWin));
	settings.endGroup();

	for(int i=0; i < windows_.count(); i++)
	{
		settings.beginGroup("window_" + QString::number(i));
		windows_.at(i)->writeSettings(settings);
		settings.endGroup();
	}
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









