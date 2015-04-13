
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

#include "InfoPanel.hpp"
#include "NodeWidget.hpp"
#include "ServerHandler.hpp"
#include "ServerFilter.hpp"
#include "TableNodeWidget.hpp"
#include "TreeNodeWidget.hpp"
#include "VFilter.hpp"
#include "VSettings.hpp"
#include "VNState.hpp"

#include <QDebug>
#include <QVBoxLayout>
#include <QLabel>
#include <QDockWidget>

int Dashboard::maxWidgetNum_=20;

Dashboard::Dashboard(QString rootNode,QWidget *parent) :
        QMainWindow(parent)
{
	//We use the mainwindow as a widget. Its task is
	//to dock all the component widgets!
	setWindowFlags(Qt::Widget);

	//The serverfilter. It holds the list of servers displayed by this dashboard.
	serverFilter_=new ServerFilter();

	//Central widget - we need to create it but we do not
	//use it. So we can hide it!
	QWidget *w=new QLabel("centre",this);
	setCentralWidget(w);
	w->hide();

	setDockOptions(QMainWindow::AnimatedDocks|QMainWindow::AllowTabbedDocks|QMainWindow::AllowNestedDocks);

	/*

	//View mode stacked widget
	QStackedLayout* centralLayout=new QStackedLayout;
	mainLayout->addLayout(centralLayout,1);

	//View handler: handles how views are activated and appear on the
	//central layout
	handler_=new NodeViewHandler(centralLayout);

	NodeWidget *ctl=0;

	//--------------------------------
	// Tree view
	//--------------------------------

	ctl=new TreeNodeViewControl(config_,this);
	handler_->add(Viewer::TreeViewMode,ctl);

	connect(ctl->widget(),SIGNAL(selectionChanged(VInfo_ptr)),
			this,SIGNAL(selectionChanged(VInfo_ptr)));

	//---------------------------------
	// Table view
	//---------------------------------

	ctl=new TableNodeViewControl(config_,this);
	handler_->add(Viewer::TableViewMode,ctl);

	connect(ctl->widget(),SIGNAL(selectionChanged(VInfo_ptr)),
				this,SIGNAL(selectionChanged(VInfo_ptr)));

	//Init view mode
	handler_->setCurrentMode(Viewer::TreeViewMode);*/
}

Dashboard::~Dashboard()
{
	delete serverFilter_;
}


DashboardWidget* Dashboard::addWidget(const std::string& type)
{
	//Get a unique dockId stored as objectName
	QString dockId=uniqueDockId();

	DashboardWidget*w=addWidget(type,dockId.toStdString());

	//At this point the widgets can be inactive. Reload will make them active!!!
	w->reload();
	return w;
}

DashboardWidget* Dashboard::addWidget(const std::string& type,const std::string& dockId)
{
	DashboardWidget *w=0;

	//Create a dashboard widget
	if(type == "tree")
	{
		NodeWidget* ctl=new TreeNodeWidget(serverFilter_,this);
		//ctl->active(true);
		connect(ctl,SIGNAL(selectionChanged(VInfo_ptr)),
					this,SIGNAL(selectionChanged(VInfo_ptr)));

		w=ctl;//widgets_ << ctl;
	}
	else if(type == "table")
	{
		NodeWidget* ctl=new TableNodeWidget(serverFilter_,this);
		//ctl->active(true);
		w=ctl;//widgets_ << ctl;
	}
	else if(type == "info")
	{
		InfoPanel* ctl=new InfoPanel(this);
		connect(this,SIGNAL(selectionChanged(VInfo_ptr)),
					ctl,SLOT(slotReload(VInfo_ptr)));

		w=ctl;//widgets_ << ctl;
	}

	//If the db-widget creation fails we should do something!!!
	if(!w)
		return 0;

	//Store dockId in the db-widget
	w->id(dockId);

	widgets_ << w;

	//Create a dockwidget at the right
    QDockWidget *dw = new QDockWidget(tr(type.c_str()), this);
    dw->setAllowedAreas(Qt::RightDockWidgetArea);

    //Store the dockId  in the dockwidget (as objectName)
    dw->setObjectName(QString::fromStdString(dockId));

    //Add the db-widget to the dockwidget
    dw->setWidget(w);

    //Add the dockwidget to the dashboard
    addDockWidget(Qt::RightDockWidgetArea, dw);

    return w;
}


VInfo_ptr Dashboard::currentSelection()
{
	//if(NodeWidget *ctl=handler_->currentControl())
	//	return  ctl->currentSelection();

	return VInfo_ptr();
}

void Dashboard::currentSelection(VInfo_ptr n)
{
	//if(NodeWidget *ctl=handler_->currentControl())
	//	ctl->currentSelection(n);
}

//-----------------------
// Title
//-----------------------

void Dashboard::updateTitle()
{
	const int marginX=0;
	const int marginY=0;
	const int textMarginX=2;
	const int textMarginY=1;
	const int gap=8;
	QFont f;
	QFontMetrics fm(f);

	//Compute the pixmap size
	int w=0;
	int h=fm.height()+2*marginY+2*textMarginY;

	QStringList texts;
	QList<QRect> textRects;
	QList<QRect> fillRects;
	QList<QColor> colors;

	int xp=marginX;
	int yp=marginY;
	const std::vector<ServerItem*> items=serverFilter_->items();
	for(std::vector<ServerItem*>::const_iterator it=items.begin(); it != items.end(); it++)
	{
		//Get text
		QString str=QString::fromStdString((*it)->name());
		textRects << QRect(xp+textMarginX,yp+textMarginY,fm.width(str),fm.height());
		texts << str;

		//Get server status
		ServerHandler* server=(*it)->serverHandler();
		colors << VNState::toColour(server);
		fillRects << QRect(xp,yp,fm.width(str)+2*textMarginX,fm.height()+2*textMarginY);

		xp=fillRects.back().right()+gap;
	}
	w=xp-gap+marginX;

	//Render the pixmap
	QPixmap pix(w,h);
	pix.fill(Qt::transparent);
	QPainter painter(&pix);

	for(int i=0; i < texts.count(); i++)
	{
		painter.setPen(Qt::NoPen);
		painter.fillRect(fillRects[i],colors[i]);
		painter.setPen(QPen());
		painter.drawText(textRects[i],texts[i]);
	}

	Q_EMIT titleChanged(this,"",pix);
}

//------------------------
// Rescan
//------------------------

void Dashboard::reload()
{
	//if(NodeWidget *ctl=handler_->currentControl())
	//	ctl->reload();


	for(int i=0; i < widgets_.count(); i++)
		widgets_.at(i)->reload();

	updateTitle();
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

void Dashboard::writeSettings(VSettings* vs)
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

void Dashboard::readSettings(VSettings* vs)
{
	serverFilter_->readSettings(vs);

	Q_FOREACH(QWidget* w,findChildren<QDockWidget*>())
	{
		qDebug() << "dock" <<  w->objectName();
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
		restoreState(vs->getQs("state").toByteArray());

	//Update the dashboard title
	updateTitle();


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
