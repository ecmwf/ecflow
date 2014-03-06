
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
#include "TableNodeView.hpp"
#include "TreeNodeView.hpp"

#include <QStackedLayout>
#include <QVBoxLayout>

NodeWidget::NodeWidget(QString rootNode, QWidget *parent) :
        QWidget(parent)
{

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
	TreeNodeView *treeView= new TreeNodeView("",this);
	views_->add(Viewer::TreeViewMode,treeView,treeView);

	/*connect(iconView_,SIGNAL(currentFolderChanged(Folder*)),
		this,SLOT(slotFolderReplacedInView(Folder*)));

	connect(iconView_,SIGNAL(iconCommandRequested(QString,IconObjectH)),
		this,SIGNAL(iconCommandRequested(QString,IconObjectH)));

	connect(iconView_,SIGNAL(desktopCommandRequested(QString,QPoint)),
		this,SIGNAL(desktopCommandRequested(QString,QPoint)));

	connect(iconView_,SIGNAL(itemEntered(QString)),
		this, SIGNAL(itemInfoChanged(QString)));*/

	// Detailed view

	TableNodeView *tableView=new TableNodeView("",this);
	views_->add(Viewer::TableViewMode,tableView,tableView);

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

//------------------------
// Save/restore settings
//------------------------

void NodeWidget::writeSettings(QSettings &settings)
{
	settings.setValue("node","");
	settings.setValue("viewMode",views_->currentMode());

	//Folder *folder=currentFolder();
	//settings.setValue("path",(folder)?QString::fromStdString(folder->fullName()):"");
	//settings.setValue("viewMode",views_->currentModeId());
	//settings.setValue("iconSize",folderModel_->iconSize());
}

void NodeWidget::readSettings(QSettings &settings)
{
	//Viewer::ViewMode viewMode=settings.value("viewMode").toInt();
	//views_->setCurrentMode(viewMode);
}
