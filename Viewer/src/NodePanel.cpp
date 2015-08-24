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
#include "InfoPanel.hpp"
#include "ServerHandler.hpp"
#include "VSettings.hpp"

#include <QDebug>

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
	qDebug() << "a";
}

//=============================================
//
//  Tab management desktopAction
//
//=============================================

Dashboard *NodePanel::addWidget(QString id)
{
  	Dashboard  *nw=new Dashboard("",this);

  	QString name("node");
  	QPixmap pix;

	addTab(nw,pix,name);


	connect(nw,SIGNAL(selectionChanged(VInfo_ptr)),
			this,SIGNAL(selectionChanged(VInfo_ptr)));

	connect(nw,SIGNAL(titleChanged(QWidget*,QString,QPixmap)),
				this,SLOT(slotTabTitle(QWidget*,QString,QPixmap)));


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

Dashboard *NodePanel::currentDashboard()
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
	if(Dashboard *w=currentDashboard())
		return w->currentSelection();

	return VInfo_ptr();
}

void NodePanel::slotSelection(VInfo_ptr n)
{
	if(Dashboard *w=currentDashboard())
			w->currentSelection(n);
}

void NodePanel::setViewMode(Viewer::ViewMode mode)
{
	Dashboard *w=currentDashboard();
	if(w) w->setViewMode(mode);
	//setDefaults(this);
}

Viewer::ViewMode NodePanel::viewMode()
{
  	Dashboard *w=currentDashboard();
	return (w)?w->viewMode():Viewer::NoViewMode;
}

ServerFilter* NodePanel::serverFilter()
{
  	Dashboard *w=currentDashboard();
	return (w)?w->serverFilter():NULL;
}

void NodePanel::addToDashboard(const std::string& type)
{
	if(Dashboard *w=currentDashboard())
		w->addWidget(type);
}

void NodePanel::addInfoToDashboard(const std::string& name)
{
	if(Dashboard *w=currentDashboard())
	{
		//if(DashboardWidget *dw=w->addWidget("info"))
		if(DashboardWidget *dw=w->addDialog("info"))
		{
			if(InfoPanel* ip=static_cast<InfoPanel*>(dw))
			{
				ip->setCurrent(name);
			}
		}

	}
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
//
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

void NodePanel::rerender()
{
	for(int i=0; i < count(); i++)
	{
		if(QWidget *w=widget(i))
		{
			if(Dashboard* nw=static_cast<Dashboard*>(w))
				nw->rerender();
		}
	}
}


void NodePanel::refreshCurrent()
{
	ServerFilter* filter=serverFilter();
	if(!filter)
		return;

	for(std::vector<ServerItem*>::const_iterator it=filter->items().begin(); it != filter->items().end(); ++it)
	{
		if(ServerHandler *sh=(*it)->serverHandler())
		{
			sh->refresh();
		}
	}
}


void NodePanel::resetCurrent()
{
	ServerFilter* filter=serverFilter();
	if(!filter)
		return;

	for(std::vector<ServerItem*>::const_iterator it=filter->items().begin(); it != filter->items().end(); ++it)
	{
		if(ServerHandler *sh=(*it)->serverHandler())
		{
			sh->reset();
		}
	}
}

//==========================================================
//
// Save/restore settings
//
//==========================================================

void NodePanel::writeSettings(VComboSettings *vs)
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

void NodePanel::readSettings(VComboSettings *vs)
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
		if(Dashboard* d=currentDashboard())
		{
			d->addWidget("tree");
		}

	}

	if(QWidget *w=currentDashboard())
		  w->setFocus();

	//We emit it to trigger the whole window ui update!
	Q_EMIT currentWidgetChanged();
}

std::string NodePanel::tabSettingsId(int i)
{
	return "tab_" + boost::lexical_cast<std::string>(i);
}


