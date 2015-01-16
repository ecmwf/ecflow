
//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeWidgetHandler.hpp"

#include "NodeViewHandler.hpp"
#include "ServerHandler.hpp"
#include "ServerFilter.hpp"
#include "VFilter.hpp"
#include "VSettings.hpp"

#include <QStackedLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QDockWidget>

LayoutManager::LayoutManager(QWidget *parent) :
	parent_(parent),
	splitter_(0),
	splitterRight_(0)
{

}

void LayoutManager::add(QWidget *w)
{
	if(widgets_.empty())
	{
		splitter_=new QSplitter(parent_);
		parent_->layout()->addWidget(splitter_);
		splitter_->setOrientation(Qt::Horizontal);
		splitter_->addWidget(w);
	}
	else if(widgets_.count()==1)
	{
		splitter_->addWidget(w);
	}
	else if(widgets_.count()==2)
	{
		if(!splitterRight_)
		{
			splitterRight_=new QSplitter(widgets_[1]);
			splitterRight_->setOrientation(Qt::Vertical);
			splitterRight_->addWidget(w);
		}
		else
		{
			splitterRight_->addWidget(w);
		}
	}

	widgets_ << w;
}


NodeWidgetHandler::NodeWidgetHandler(QString rootNode,QWidget *parent) :
        QMainWindow(parent),
        config_(0),
        lm_(0)
{
	//We use the mainwindow as a widget. Its task is
	//to dock all the component widgets!
	setWindowFlags(Qt::Widget);

	//Filters
	config_=new VConfig;

	// Layout
	//QVBoxLayout *mainLayout = new QVBoxLayout(this);
	//mainLayout->setSpacing(0);
	//mainLayout->setContentsMargins(1,1,1,1);

	//Central widget - we need to create it but we do not
	//use it. So we can hide it!
	QWidget *w=new QWidget(this);
	setCentralWidget(w);
	w->hide();

	//QVBoxLayout *ml = new QVBoxLayout(ww);
	//ml->setSpacing(0);
	//ml->setContentsMargins(1,1,1,1);

	NodeWidget* ctl=new TreeNodeWidget(config_,this);
	ctl->active(true);
	widgets_ << ctl;

	QDockWidget *dw=new QDockWidget("tree",this);
	dw->setWidget(ctl);
	addDockWidget(Qt::RightDockWidgetArea, dw);

	ctl=new TableNodeWidget(config_,this);
	ctl->active(true);
	widgets_ << ctl;

	dw=new QDockWidget("table",this);
	dw->setWidget(ctl);
	addDockWidget(Qt::RightDockWidgetArea, dw);

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

NodeWidgetHandler::~NodeWidgetHandler()
{
	//MvQFolderWatcher::remove(this);
	delete config_;
}
/*
void NodeWidgetHandler::addTreeWidget()
{
	NodeWidget* ctl=new TreeNodeWidget(config_,this);
	ctl->active(true);
	widgets_ << ctl;

	QDockWidget *dw=new QDockWidget("tree",this);
	dw->setWidget(ctl);
	addDockWidget(Qt::RightDockWidgetArea, dw);
}

void NodeWidgetHandler::addTableWidget()
{
	NodeWidget* ctl=new TableNodeWidget(config_,this);
	ctl->active(true);
	widgets_ << ctl;

	QDockWidget *dw=new QDockWidget("tree",this);
	dw->setWidget(ctl);
	addDockWidget(Qt::RightDockWidgetArea, dw);
}

void NodeWidgetHandler::addInfoPanel()
{

}

*/

VInfo_ptr NodeWidgetHandler::currentSelection()
{
	//if(NodeWidget *ctl=handler_->currentControl())
	//	return  ctl->currentSelection();

	return VInfo_ptr();
}

void NodeWidgetHandler::currentSelection(VInfo_ptr n)
{
	//if(NodeWidget *ctl=handler_->currentControl())
	//	ctl->currentSelection(n);
}

//------------------------
// Rescan
//------------------------

void NodeWidgetHandler::reload()
{
	//if(NodeWidget *ctl=handler_->currentControl())
	//	ctl->reload();




	for(int i=0; i < widgets_.count(); i++)
		widgets_.at(i)->reload();

}

//------------------------
// ViewMode
//------------------------

Viewer::ViewMode NodeWidgetHandler::viewMode()
{
 	//return handler_->currentMode();
	return Viewer::TreeViewMode;
}

void NodeWidgetHandler::setViewMode(Viewer::ViewMode mode)
{
	//handler_->setCurrentMode(mode);
}

//----------------------------------
// Save and load settings!!
//----------------------------------

void NodeWidgetHandler::writeSettings(VSettings* vs)
{
	config_->writeSettings(vs);
}

void NodeWidgetHandler::readSettings(VSettings* vs)
{
	config_->readSettings(vs);
	reload();
}
