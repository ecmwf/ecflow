//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "NodePanel.hpp"

#include "Dashboard.hpp"
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

Dashboard *NodePanel::addWidget(QString id)
{
	//if(path.isEmpty())
	//  	return 0;

  	Dashboard  *nw=new Dashboard("",this);

	//QString name=fw->currentFolderName();
  	QString name("node");
	//QPixmap pix=MvQIconProvider::pixmap(fw->currentFolder(),24);
  	QPixmap pix;

	addTab(nw,pix,name);


	connect(nw,SIGNAL(selectionChanged(VInfo_ptr)),
			this,SIGNAL(selectionChanged(VInfo_ptr)));

	connect(nw,SIGNAL(titleChanged(QWidget*,QString,QPixmap)),
				this,SLOT(slotTabTitle(QWidget*,QString,QPixmap)));

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
	  	Dashboard *w=nodeWidget(index);
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

Dashboard *NodePanel::nodeWidget(int index)
{
  	QWidget *w=widget(index);
  	return (w)?static_cast<Dashboard*>(w):0;
}

Dashboard *NodePanel::currentNodeWidget()
{
  	QWidget *w=currentWidget();
  	return static_cast<Dashboard*>(w);
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
	if(Dashboard *w=currentNodeWidget())
		return w->currentSelection();

	return VInfo_ptr();
}

void NodePanel::slotSelection(VInfo_ptr n)
{
	if(Dashboard *w=currentNodeWidget())
			w->currentSelection(n);
}

void NodePanel::setViewMode(Viewer::ViewMode mode)
{
	Dashboard *w=currentNodeWidget();
	if(w) w->setViewMode(mode);
	//setDefaults(this);
}

Viewer::ViewMode NodePanel::viewMode()
{
  	Dashboard *w=currentNodeWidget();
	return (w)?w->viewMode():Viewer::NoViewMode;
}

ServerFilter* NodePanel::serverFilter()
{
  	Dashboard *w=currentNodeWidget();
	return (w)?w->serverFilter():NULL;
}

void NodePanel::addToDashboard(const std::string& type)
{
	if(Dashboard *w=currentNodeWidget())
		w->addWidget(type);
}

void NodePanel::slotTabTitle(QWidget* w,QString text,QPixmap pix)
{
	int index=indexOfWidget(w);
	if(index != -1)
	{
		setTabText(index,text);
		setTabIcon(index,pix);
	}
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
			if(Dashboard* nw=static_cast<Dashboard*>(w))
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
		if(Dashboard* nw=nodeWidget(i))
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
			Dashboard* nw=addWidget("");
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


