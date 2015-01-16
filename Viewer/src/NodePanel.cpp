//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "NodePanel.hpp"

#include "NodeWidgetHandler.hpp"
#include "MainWindow.hpp"
#include "VSettings.hpp"

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

NodeWidgetHandler *NodePanel::addWidget(QString id)
{
	//if(path.isEmpty())
	//  	return 0;

  	NodeWidgetHandler  *nw=new NodeWidgetHandler("",this);

	//QString name=fw->currentFolderName();
  	QString name("node");
	//QPixmap pix=MvQIconProvider::pixmap(fw->currentFolder(),24);
  	QPixmap pix;

	addTab(nw,pix,name);


	connect(nw,SIGNAL(selectionChanged(VInfo_ptr)),
			this,SIGNAL(selectionChanged(VInfo_ptr)));

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

	Q_FOREACH(QString id,idLst)
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
	  	NodeWidgetHandler *w=nodeWidget(index);
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

NodeWidgetHandler *NodePanel::nodeWidget(int index)
{
  	QWidget *w=widget(index);
  	return (w)?static_cast<NodeWidgetHandler*>(w):0;
}

NodeWidgetHandler *NodePanel::currentNodeWidget()
{
  	QWidget *w=currentWidget();
  	return static_cast<NodeWidgetHandler*>(w);
}

void NodePanel::slotCurrentWidgetChanged(int /*index*/)
{
  	Q_EMIT currentWidgetChanged();
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

VInfo_ptr NodePanel::currentSelection()
{
	if(NodeWidgetHandler *w=currentNodeWidget())
		return w->currentSelection();

	return VInfo_ptr();
}

void NodePanel::slotSelection(VInfo_ptr n)
{
	if(NodeWidgetHandler *w=currentNodeWidget())
			w->currentSelection(n);
}

void NodePanel::setViewMode(Viewer::ViewMode mode)
{
	NodeWidgetHandler *w=currentNodeWidget();
	if(w) w->setViewMode(mode);
	//setDefaults(this);
}

Viewer::ViewMode NodePanel::viewMode()
{
  	NodeWidgetHandler *w=currentNodeWidget();
	return (w)?w->viewMode():Viewer::NoViewMode;
}

VConfig* NodePanel::config()
{
  	NodeWidgetHandler *w=currentNodeWidget();
	return (w)?w->config():NULL;
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
			if(NodeWidgetHandler* nw=static_cast<NodeWidgetHandler*>(w))
				nw->reload();
		}
	}
}




//==========================================================
//
// Save/restore settings
//
//==========================================================

void NodePanel::writeSettings(VSettings *vs)
{
	int currentIdx=(currentIndex()>=0)?currentIndex():0;

	vs->put("tabCount",count());
	vs->put("currentTabId",currentIdx);

	for(int i=0; i < count(); i++)
	{
		//boost::property_tree::ptree ptTab;
		if(NodeWidgetHandler* nw=nodeWidget(i))
		{
			std::string id=NodePanel::tabSettingsId(i);
			vs->beginGroup(id);
			nw->writeSettings(vs);
			vs->endGroup();
			//pt.add_child("tab_"+ boost::lexical_cast<std::string>(i),ptTab);
		}
	}
}

void NodePanel::readSettings(VSettings *vs)
{
	using boost::property_tree::ptree;

	int cnt=vs->get<int>("tabCount",0);
	int currentIndex=vs->get<int>("currentTabId",-1);

	for(int i=0; i < cnt; i++)
	{
		std::string id=NodePanel::tabSettingsId(i);
		if(vs->contains(id))
		{
			NodeWidgetHandler* nw=addWidget("");
			if(nw)
			{
				vs->beginGroup(id);
				nw->readSettings(vs);
				vs->endGroup();
			}
		}
	}

	//Set current tab
	if(currentIndex >=0 && currentIndex < count())
	{
		setCurrentIndex(currentIndex);
	}

	//If no tabs have been created
	if(count()==0)
	{
		addWidget("");
		setCurrentIndex(0);
	}

	if(QWidget *w=currentNodeWidget())
		  w->setFocus();

	//We emit it to trigger the whole window ui update!
	Q_EMIT currentWidgetChanged();
}

std::string NodePanel::tabSettingsId(int i)
{
	return "tab_" + boost::lexical_cast<std::string>(i);
}


