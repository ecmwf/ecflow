
//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeWidget.hpp"

#include "NodeViewHandler.hpp"
#include "ServerHandler.hpp"
#include "ServerFilter.hpp"
#include "VFilter.hpp"

#include <QStackedLayout>
#include <QVBoxLayout>

NodeWidget::NodeWidget(QString rootNode,QWidget *parent) :
        QWidget(parent),
        config_(0)
{
	//Filters
	config_=new VConfig;

	// Layout
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setSpacing(0);
	mainLayout->setContentsMargins(1,1,1,1);

	//View mode stacked widget
	QStackedLayout* centralLayout=new QStackedLayout;
	mainLayout->addLayout(centralLayout,1);

	//View handler: handles how views are activated and appear on the
	//central layout
	handler_=new NodeViewHandler(centralLayout);

	NodeViewControl *ctl=0;

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
	handler_->setCurrentMode(Viewer::TreeViewMode);
}

NodeWidget::~NodeWidget()
{
	//MvQFolderWatcher::remove(this);
	delete config_;
}

VInfo_ptr NodeWidget::currentSelection()
{
	if(NodeViewControl *ctl=handler_->currentControl())
		return  ctl->currentSelection();

	return VInfo_ptr();
}

void NodeWidget::currentSelection(VInfo_ptr n)
{
	if(NodeViewControl *ctl=handler_->currentControl())
		ctl->currentSelection(n);
}

//------------------------
// Rescan
//------------------------

void NodeWidget::reload()
{
	if(NodeViewControl *ctl=handler_->currentControl())
		ctl->reload();
}

//------------------------
// ViewMode
//------------------------

Viewer::ViewMode NodeWidget::viewMode()
{
 	return handler_->currentMode();
}

void NodeWidget::setViewMode(Viewer::ViewMode mode)
{
	handler_->setCurrentMode(mode);
}

//----------------------------------
// Save and load settings!!
//----------------------------------

void NodeWidget::save(boost::property_tree::ptree &pt)
{
	//Server
	ServerFilter *serverFilter=config_->serverFilter();
	boost::property_tree::ptree serverArray;
	serverFilter->save(serverArray);
	pt.push_back(std::make_pair("server", serverArray));

	//State
	VFilter *filter=config_->stateFilter();
	boost::property_tree::ptree stateArray;
	filter->save(stateArray);
	pt.push_back(std::make_pair("state", stateArray));

	//Attribute
	filter=config_->attributeFilter();
	boost::property_tree::ptree attrArray;
	filter->save(attrArray);
	pt.push_back(std::make_pair("attribute", attrArray));

	//Icons
	filter=config_->iconFilter();
	boost::property_tree::ptree iconArray;
	filter->save(iconArray);
	pt.push_back(std::make_pair("icon", iconArray));
}

void NodeWidget::load(const boost::property_tree::ptree &pt)
{
	using boost::property_tree::ptree;

	ptree::const_assoc_iterator it=pt.find("server");
	if(it != pt.not_found())
	{
		ServerFilter *serverFilter=config_->serverFilter();
		serverFilter->load(it->second);
	}

	it=pt.find("state");
	if(it != pt.not_found())
	{
		VFilter *filter=config_->stateFilter();
		filter->load(it->second);
	}

	it=pt.find("attribute");
	if(it != pt.not_found())
	{
		VFilter *filter=config_->attributeFilter();
		filter->load(it->second);
	}

	it=pt.find("icon");
	if(it != pt.not_found())
	{
		VFilter *filter=config_->iconFilter();
		filter->load(it->second);
	}

	reload();
}
