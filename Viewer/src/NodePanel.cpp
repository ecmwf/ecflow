//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "NodePanel.hpp"

#include "NodeWidget.hpp"
#include "MainWindow.hpp"

NodePanel::NodePanel(QWidget* parent) :
  TabWidget(parent)

{
	connect(this,SIGNAL(currentIndexChanged(int)),
		    this,SLOT(slotCurrentWidgetChanged(int)));

	connect(this,SIGNAL(newTabRequested()),
		    this,SLOT(slotNewTab()));

}

NodePanel::~NodePanel()
{
}


/*Folder* NodePanel::currentFolder()
{
	NodeWidget* w=currentFolderWidget();
  	return (w)?w->currentFolder():0;
}

QList<Folder*> NodePanel::currentFolders()
{
	QList<Folder*>  lst;
	for(int i=0; i < count(); i++)
	{
		NodeWidget* w=folderWidget(i);
		if(w)
		{
		  	lst << w->currentFolder();
		}
	}
	return lst;
}

QString  NodePanel::folderPath(int index)
{
	if(NodeWidget* w=folderWidget(index))
	{
	  	if(Folder *f=w->currentFolder())
			return QString::fromStdString(f->fullName());
	}
	return QString();
}
*/
//=============================================
//
//  Tab management desktopAction
//
//=============================================

NodeWidget *NodePanel::addWidget(QString id)
{
	//if(path.isEmpty())
	//  	return 0;

  	NodeWidget  *nw=new NodeWidget("",this);

	//QString name=fw->currentFolderName();
  	QString name("node");
	//QPixmap pix=MvQIconProvider::pixmap(fw->currentFolder(),24);
  	QPixmap pix;

	addTab(nw,pix,name);


	connect(nw,SIGNAL(selectionChanged(ViewNodeInfo_ptr)),
			this,SIGNAL(selectionChanged(ViewNodeInfo_ptr)));

	/*connect(fw,SIGNAL(pathChanged()),
		this,SLOT(slotPathChanged()));

	connect(fw,SIGNAL(iconCommandRequested(QString,IconObjectH)),
		this,SLOT(slotIconCommand(QString,IconObjectH)));

	connect(fw,SIGNAL(desktopCommandRequested(QString,QPoint)),
		this,SLOT(slotDesktopCommand(QString,QPoint)));

	connect(fw,SIGNAL(itemInfoChanged(QString)),
		this,SIGNAL(itemInfoChanged(QString)));*/

	//setDefaults(this);

	return nw;
}


void NodePanel::resetWidgets(QStringList idLst)
{
	if(idLst.count() ==0)
		return;

	clear();

	foreach(QString id,idLst)
	{
	  	addWidget(id);
	}
}


/*MvQContextItemSet* NodePanel::cmSet()
{
	static MvQContextItemSet cmItems("FolderPanel");
  	return &cmItems;
}*/

//void NodePanel::slotCurrentWidgetChanged(int index)
//{
//	qDebug() << "current" << index;
//	emit currentWidgetChanged();
//}


//Handle the tab bar context menu actions
void NodePanel::tabBarCommand(QString name,int index)
{
  	if(name == "reloadTab")
	{
	  	NodeWidget *w=nodeWidget(index);
		if(w) w->reload();
	}
	else if(name == "closeOtherTabs")
	{
	  	removeOtherTabs(index);
	}
	else if(name == "closeTab")
	{
	  	removeTab(index);
	}
}

NodeWidget *NodePanel::nodeWidget(int index)
{
  	QWidget *w=widget(index);
  	return (w)?static_cast<NodeWidget*>(w):0;
}

NodeWidget *NodePanel::currentNodeWidget()
{
  	QWidget *w=currentWidget();
  	return static_cast<NodeWidget*>(w);
}

void NodePanel::slotCurrentWidgetChanged(int /*index*/)
{
  	emit currentWidgetChanged();
  	//setDefaults(this);
}

void  NodePanel::slotNewTab()
{
	addWidget("");

	/*if(Folder *f=currentFolder())
	{
		QString p=QString::fromStdString(f->fullName());
		addWidget(p);
	}*/
}


void NodePanel::setViewMode(Viewer::ViewMode mode)
{
	NodeWidget *w=currentNodeWidget();
	if(w) w->setViewMode(mode);
	//setDefaults(this);
}

Viewer::ViewMode NodePanel::viewMode()
{
  	NodeWidget *w=currentNodeWidget();
	return (w)?w->viewMode():Viewer::NoViewMode;
}

FilterData* NodePanel::filterData()
{
  	NodeWidget *w=currentNodeWidget();
	return (w)?w->filterData():0;
}

/*void NodePanel::slotNewWindow(bool)
{
	//MainWindow::openWindow("",this);

	if(Folder* f=currentFolder())
	{
	  	QString p=QString::fromStdString(f->fullName());
		MvQFileBrowser::openBrowser(p,this);
	}
}*/

/*void NodePanel::setViewMode(Viewer::NodeViewMode mode)
{
	NodeWidget *w=currentNodeWidget();
	if(w) w->setViewMode(mode);
	//setDefaults(this);
}

Viewer::FolderViewMode NodePanel::viewMode()
{
  	NodeWidget *w=currentNodeWidget();
	return (w)?w->viewMode():MvQ::NoViewMode;
}*/

//==========================================================
//
// Rescan
//
//==========================================================

void NodePanel::reload()
{
	for(int i=0; i < count(); i++)
	{
		if(QWidget *w=widget(i))
		{
			if(NodeWidget* nw=static_cast<NodeWidget*>(w))
				nw->reload();
		}
	}
}


//==========================================================
//
// Save/restore settings
//
//==========================================================

void NodePanel::writeSettings(QSettings &settings)
{
	settings.setValue("tabCount",count());

	settings.setValue("current",(currentIndex()>=0)?currentIndex():0);

	for(int i=0; i < count(); i++)
	{
		NodeWidget* cw=nodeWidget(i);

		settings.beginGroup("tab_" + QString::number(i));
		cw->writeSettings(settings);
		settings.endGroup();
	}
}

void NodePanel::readSettings(QSettings &settings)
{
	//Create folder tabs
	int cnt=settings.value("tabCount").toInt();
	int currentIndex=settings.value("current").toInt();

	if(cnt >0)
	{
		for(int i=0; i < cnt; i++)
		{
			settings.beginGroup("tab_" + QString::number(i));
			QString relPath=settings.value("path").toString();
			//Path mvPath(path.toStdString());
			//if(mvPath.exists())
			//{
		  		NodeWidget* nw=addWidget(relPath);
				if(nw)
					nw->readSettings(settings);

			//}
			settings.endGroup();
		}

		//Set current tab
		if(currentIndex >=0 && currentIndex < cnt)
		{
			setCurrentIndex(currentIndex);
		}
	}
	else
	{
			addWidget("");
			setCurrentIndex(0);
	}


	/*else if(!mvHomePath_.isEmpty())
	{
		Path mvPath(mvHomePath_.toStdString());
		if(mvPath.exists())
			addWidget(mvHomePath_);
	}*/

	//setDefaults(this);

	if(QWidget *w=currentNodeWidget())
	  	w->setFocus();
}



