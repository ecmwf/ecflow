
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
#include "TableNodeView.hpp"
#include "TreeNodeView.hpp"
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

	//View handler
	views_=new NodeViewHandler(centralLayout);

	//Tree view
	TreeNodeView *treeView= new TreeNodeView("",config_,this);
	views_->add(Viewer::TreeViewMode,treeView);

	connect(treeView,SIGNAL(selectionChanged(ViewNodeInfo_ptr)),
			this,SIGNAL(selectionChanged(ViewNodeInfo_ptr)));

	/*connect(iconView_,SIGNAL(currentFolderChanged(Folder*)),
		this,SLOT(slotFolderReplacedInView(Folder*)));

	connect(iconView_,SIGNAL(iconCommandRequested(QString,IconObjectH)),
		this,SIGNAL(iconCommandRequested(QString,IconObjectH)));

	connect(iconView_,SIGNAL(desktopCommandRequested(QString,QPoint)),
		this,SIGNAL(desktopCommandRequested(QString,QPoint)));

	connect(iconView_,SIGNAL(itemEntered(QString)),
		this, SIGNAL(itemInfoChanged(QString)));*/

	// Detailed view

	TableNodeView *tableView=new TableNodeView("",config_,this);
	views_->add(Viewer::TableViewMode,tableView);

	connect(tableView,SIGNAL(selectionChanged(ViewNodeInfo_ptr)),
			this,SIGNAL(selectionChanged(ViewNodeInfo_ptr)));


	/*connect(detailedView_,SIGNAL(currentFolderChanged(Folder*)),
		this,SLOT(slotFolderReplacedInView(Folder*)));

	connect(detailedView_,SIGNAL(iconCommandRequested(QString,IconObjectH)),
		this,SIGNAL(iconCommandRequested(QString,IconObjectH)));

	connect(detailedView_,SIGNAL(desktopCommandRequested(QString,QPoint)),
		this,SIGNAL(desktopCommandRequested(QString,QPoint)));

	connect(detailedView_,SIGNAL(itemEntered(QString)),
		this,SIGNAL(itemInfoChanged(QString)));*/


	//Init view mode
	views_->setCurrentMode(Viewer::TreeViewMode);


	//MvQFolderWatcher::add(this);
}

NodeWidget::~NodeWidget()
{
	//MvQFolderWatcher::remove(this);
	delete config_;
}

/*Folder* NodeWidget::currentFolder()
{
	return folderModel_->folder();
}

QString NodeWidget::currentFolderName()
{
	if(folderModel_->folder())
	  	return QString::fromStdString(folderModel_->folder()->name());

	return QString();
}*/



//When the  object itself is changed (e.g. renamed)
/*void NodeWidget::slotFolderChanged(Folder* folder)
{
	slotFolderReplacedInView(folder);
}*/

//------------------------
// Rescan
//------------------------

void NodeWidget::reload()
{
	//MvQFolderWatcher::reload(this);
	views_->currentBase()->reload();

}

//------------------------
// ViewMode
//------------------------

Viewer::ViewMode NodeWidget::viewMode()
{
 	return views_->currentMode();
}

void NodeWidget::setViewMode(Viewer::ViewMode mode)
{
	views_->setCurrentMode(mode);
}

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
