//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "Dashboard.hpp"

#include "DashboardDialog.hpp"
#include "DashboardDock.hpp"
#include "DashboardTitle.hpp"
#include "InfoPanel.hpp"
#include "NodeWidget.hpp"
#include "ServerHandler.hpp"
#include "ServerFilter.hpp"
#include "TableNodeWidget.hpp"
#include "TreeNodeWidget.hpp"
#include "UserMessage.hpp"
#include "VFilter.hpp"
#include "VSettings.hpp"

#include <QDebug>
#include <QVBoxLayout>
#include <QLabel>
#include <QDockWidget>
#include <QToolButton>
#include "NodeSearchDialog.hpp"
#include "NodeSearchWidget.hpp"

int Dashboard::maxWidgetNum_=20;

Dashboard::Dashboard(QString rootNode,QWidget *parent) :
   QMainWindow(parent),
   settingsAreRead_(false)
{
	//We use the mainwindow as a widget. Its task is
	//to dock all the component widgets!
	setWindowFlags(Qt::Widget);

	//The serverfilter. It holds the list of servers displayed by this dashboard.
	serverFilter_=new ServerFilter();
	serverFilter_->addObserver(this);

	titleHandler_=new DashboardTitle(serverFilter_,this);

	//Central widget - we need to create it but we do not
	//use it. So we can hide it!
	QWidget *w=new QLabel("centre",this);
	setCentralWidget(w);
	w->hide();

	layout()->setContentsMargins(0,0,0,0);

	setDockOptions(QMainWindow::AnimatedDocks|QMainWindow::AllowTabbedDocks|QMainWindow::AllowNestedDocks);

	connect(titleHandler_,SIGNAL(changed(QString,QPixmap)),
			this,SLOT(slotTitle(QString,QPixmap)));
}

Dashboard::~Dashboard()
{
	Q_EMIT aboutToDelete();

	serverFilter_->removeObserver(this);
	delete serverFilter_;
}


DashboardWidget* Dashboard::addWidgetCore(const std::string& type)
{
	DashboardWidget *w=0;

	//Create a dashboard widget
	if(type == "tree")
	{
		NodeWidget* ctl=new TreeNodeWidget(serverFilter_,this);

		connect(ctl,SIGNAL(selectionChanged(VInfo_ptr)),
				this,SIGNAL(selectionChanged(VInfo_ptr)));

		connect(ctl,SIGNAL(popInfoPanel(VInfo_ptr,QString)),
				this,SLOT(slotPopInfoPanel(VInfo_ptr,QString)));

		connect(ctl,SIGNAL(dashboardCommand(VInfo_ptr,QString)),
				this,SLOT(slotCommand(VInfo_ptr,QString)));

		w=ctl;
	}
	else if(type == "table")
	{
		NodeWidget* ctl=new TableNodeWidget(serverFilter_,this);

		connect(ctl,SIGNAL(selectionChanged(VInfo_ptr)),
				this,SIGNAL(selectionChanged(VInfo_ptr)));

		connect(ctl,SIGNAL(popInfoPanel(VInfo_ptr,QString)),
				this,SLOT(slotPopInfoPanel(VInfo_ptr,QString)));

		connect(ctl,SIGNAL(dashboardCommand(VInfo_ptr,QString)),
				this,SLOT(slotCommand(VInfo_ptr,QString)));

		w=ctl;
	}
	else if(type == "info")
	{
		InfoPanel* ctl=new InfoPanel(this);
		connect(this,SIGNAL(selectionChanged(VInfo_ptr)),
					ctl,SLOT(slotReload(VInfo_ptr)));

		w=ctl;
	}

	return w;
}


DashboardWidget* Dashboard::addWidget(const std::string& type)
{
	//Get a unique dockId stored as objectName
	QString dockId=uniqueDockId();

	DashboardWidget*w=addWidget(type,dockId.toStdString());

	//At this point the widgets can be inactive. Reload will make them active!!!
	w->reload();

	if(type == "info")
	{
		VInfo_ptr info=currentSelectionInView();
		if(info && info.get())
		{
			if(InfoPanel* ip=static_cast<InfoPanel*>(w))
			{
				ip->slotReload(info);
			}
		}
	}

	return w;
}

DashboardWidget* Dashboard::addWidget(const std::string& type,const std::string& dockId)
{
	DashboardWidget *w=Dashboard::addWidgetCore(type);

	//If the db-widget creation fails we should do something!!!
	if(!w)
		return 0;

	//Store dockId in the db-widget
	w->id(dockId);

	widgets_ << w;

    //Create a dockwidget
	DashboardDock *dw = new DashboardDock(w, this);

    dw->setAllowedAreas(Qt::RightDockWidgetArea);

    //Store the dockId  in the dockwidget (as objectName)
    dw->setObjectName(QString::fromStdString(dockId));

    //Add the dockwidget to the dashboard
    addDockWidget(Qt::RightDockWidgetArea, dw);

    connect(dw,SIGNAL(closeRequested()),
    		this,SLOT(slotDockClose()));

    return w;
}

DashboardWidget* Dashboard::addDialog(const std::string& type)
{
	DashboardWidget *w=Dashboard::addWidgetCore(type);

	//If the db-widget creation fails we should do something!!!
	if(!w)
		return 0;

	//The DashBoard or any of its child cannot be the parent of the
	//dialog because in this case it would be always on top its parent. This is
	//the behaviour when the dialog's parent is QMainWindow.
	DashboardDialog* dia=new DashboardDialog(0);

    //So the parent is 0 and we will emit a signal from the Dashboard
	//destructor to notify the dialog about it. Then we can be sure
	//sure that the dialog deletes itself when the Dashboard gets deleted.
    connect(this,SIGNAL(aboutToDelete()),
    		dia,SLOT(slotOwnerDelete()));

    connect(dia,SIGNAL(finished(int)),
            this,SLOT(slotDialogFinished(int)));

    //The dialog will reparent the widget
    dia->add(w);
	dia->show();

    return w;
}

void Dashboard::addSearchDialog()
{
	//It will delete itself on close!!
	//The parent is 0, for the reason see the comment in addDialog()
	NodeSearchDialog* d=new NodeSearchDialog(0);
	d->queryWidget()->setServerFilter(serverFilter_);

	for(int i=0; i < widgets_.count(); i++)
	{
		if(widgets_.at(i)->type() == "tree")
		{
			connect(d->queryWidget(),SIGNAL(selectionChanged(VInfo_ptr)),
				    widgets_.at(i),SLOT(setCurrentSelection(VInfo_ptr)));
		}
	}

	//The dashboard signals the dialog on deletion
	connect(this,SIGNAL(aboutToDelete()),
	    	d,SLOT(slotOwnerDelete()));

	d->show();
}

void Dashboard::addSearchDialog(VInfo_ptr info)
{
	//It will delete itself on close!!
	//The parent is 0, for the reason see the comment in addDialog()
	NodeSearchDialog* d=new NodeSearchDialog(0);
	d->queryWidget()->setServerFilter(serverFilter_);
	d->queryWidget()->setRootNode(info);

	for(int i=0; i < widgets_.count(); i++)
	{
		if(widgets_.at(i)->type() == "tree")
		{
			connect(d->queryWidget(),SIGNAL(selectionChanged(VInfo_ptr)),
				    widgets_.at(i),SLOT(setCurrentSelection(VInfo_ptr)));
		}
	}

	//The dashboard signals the dialog on deletion
	connect(this,SIGNAL(aboutToDelete()),
		    d,SLOT(slotOwnerDelete()));

	d->show();
}

void Dashboard::slotDockClose()
{
	if(DashboardDock *dock=static_cast<DashboardDock*>(sender()))
	{
		if(DashboardWidget* dw=static_cast<DashboardWidget*>(dock->widget()))
		{
			widgets_.removeOne(dw);
			disconnect(this,0,dw,0);
			dw->deleteLater();
		}
	}
}

VInfo_ptr Dashboard::currentSelection()
{
	return currentSelectionInView();
}

void Dashboard::currentSelection(VInfo_ptr n)
{
	//if(NodeWidget *ctl=handler_->currentControl())
	//	ctl->currentSelection(n);
}

void Dashboard::slotPopInfoPanel(QString name)
{
	if(DashboardWidget *dw=addDialog("info"))
	{
		if(InfoPanel* ip=static_cast<InfoPanel*>(dw))
		{
            ip->setDetached(true);
			ip->setCurrent(name.toStdString());
		}
	}
}

void Dashboard::slotPopInfoPanel(VInfo_ptr info,QString name)
{
	if(DashboardWidget *dw=addDialog("info"))
	{
		if(InfoPanel* ip=static_cast<InfoPanel*>(dw))
		{
            ip->setDetached(true);
            ip->slotReload(info);
            ip->setCurrent(name.toStdString());
		}
	}
}

void Dashboard::slotTitle(QString s,QPixmap p)
{
	Q_EMIT titleChanged(this,s,p);
}



void Dashboard::slotCommand(VInfo_ptr info,QString cmd)
{
	if(!info || !info.get() )
		return;

	if(cmd == "search")
	{
		addSearchDialog(info);
	}
}

//------------------------
// Dialogs
//------------------------

void Dashboard::slotDialogFinished(int)
{
	if(DashboardDialog* dia=static_cast<DashboardDialog*>(sender()))
	{
		disconnect(this,0,dia->dashboardWidget(),0);
	}
}

//------------------------
// Rescan
//------------------------

void Dashboard::reload()
{
	for(int i=0; i < widgets_.count(); i++)
		widgets_.at(i)->reload();
}

void Dashboard::rerender()
{
	for(int i=0; i < widgets_.count(); i++)
		widgets_.at(i)->rerender();

	titleHandler_->updateTitle();
}

//------------------------
// ViewMode
//------------------------

Viewer::ViewMode Dashboard::viewMode()
{
 	//return handler_->currentMode();
	return Viewer::TreeViewMode;
}

void Dashboard::setViewMode(Viewer::ViewMode mode)
{
	//handler_->setCurrentMode(mode);
}

//----------------------------------
// Save and load settings!!
//----------------------------------

void Dashboard::writeSettings(VComboSettings* vs)
{
	serverFilter_->writeSettings(vs);

	//Qt settings
	vs->putQs("state",saveState());

	//Other setting
	vs->put("widgetCount",findChildren<QDockWidget*>().count());

	for(int i=0; i < widgets_.count(); i++)
	{
		std::string id=Dashboard::widgetSettingsId(i);
		vs->beginGroup(id);
		widgets_.at(i)->writeSettings(vs);
		vs->endGroup();
	}
}

void Dashboard::readSettings(VComboSettings* vs)
{
	settingsAreRead_=true;

	serverFilter_->readSettings(vs);

	Q_FOREACH(QWidget* w,findChildren<QDockWidget*>())
	{
		UserMessage::message(UserMessage::DBG,false,std::string("DashBoard::readSettings() dock: ") +  w->objectName().toStdString());
	}

	//Read the information about the dashboard widgets.
	int cnt=vs->get<int>("widgetCount",0);
	for(int i=0; i < cnt; i++)
	{
		std::string id=Dashboard::widgetSettingsId(i);
		if(vs->contains(id))
		{
			vs->beginGroup(id);
			std::string type=vs->get<std::string>("type","");
			std::string dockId=vs->get<std::string>("dockId","");

			//Create a dashboard widget
			if(DashboardWidget *dw=addWidget(type,dockId))
			{
				//This will make the widgets active!!!
				dw->readSettings(vs);
			}
			vs->endGroup();
		}
	}

	//Restore state of the dockwidgets. It will not create the dockwidgets
	//themselves but set their states and geometry. This is based on
	//the the dockwidgets's objectname, so that has to be unique. We need to call
	//it when the dockwidgets have already been created.
	if(vs->containsQs("state"))
	{
		restoreState(vs->getQs("state").toByteArray());
	}

	selectFirstServerInView();

	settingsAreRead_=false;
}


void Dashboard::selectFirstServerInView()
{
	Q_FOREACH(DashboardWidget* w,widgets_)
	{
		if(w->selectFirstServerInView())
		{
			return;
		}
	}
}

VInfo_ptr Dashboard::currentSelectionInView()
{
	Q_FOREACH(DashboardWidget* w,widgets_)
	{
		VInfo_ptr info=w->currentSelection();
		if(info && info.get())
			return info;
	}

	return VInfo_ptr();
}

//Find an unique id for a new dockwidget
QString Dashboard::uniqueDockId()
{
	QSet<QString> ids;
	Q_FOREACH(QDockWidget* dw, findChildren<QDockWidget*>())
	{
		ids << dw->objectName();
	}

	for(unsigned i=0; i < 1000; i++)
	{
		QString uid="dock_" + QString::number(i);
		if(!ids.contains(uid))
		{
			return uid;
		}
	}

	//We should handle this situation!!
	assert(0);

	return "dock_1000";
}

std::string Dashboard::widgetSettingsId(int i)
{
	return "widget_" + boost::lexical_cast<std::string>(i);
}

void Dashboard::notifyServerFilterAdded(ServerItem* item)
{
	if(!settingsAreRead_)
		Q_EMIT contentsChanged();
}

void Dashboard::notifyServerFilterRemoved(ServerItem* item)
{
	if(!settingsAreRead_)
		Q_EMIT contentsChanged();
}

void Dashboard::notifyServerFilterChanged(ServerItem*)
{
	if(!settingsAreRead_)
		Q_EMIT contentsChanged();
}

void Dashboard::notifyServerFilterDelete()
{
	//if(!settingsAreRead_)
	//	Q_EMIT contentsChanged();
}


