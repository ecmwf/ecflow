//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "NodePanel.hpp"

#include "Dashboard.hpp"
#include "DashboardTitle.hpp"
#include "InfoPanel.hpp"
#include "ServerHandler.hpp"
#include "VSettings.hpp"

#include <QDebug>
#include <QResizeEvent>

NodePanel::NodePanel(QWidget* parent) :
  TabWidget(parent)

{
    setObjectName("p");

    connect(this,SIGNAL(currentIndexChanged(int)),
		    this,SLOT(slotCurrentWidgetChanged(int)));

	connect(this,SIGNAL(newTabRequested()),
		    this,SLOT(slotNewTab()));

    connect(this,SIGNAL(tabRemoved()),
            this,SLOT(slotTabRemoved()));

}

NodePanel::~NodePanel()
{    
}

//=============================================
//
//  Tab management desktopAction
//
//=============================================

Dashboard *NodePanel::addWidget(QString id)
{
  	Dashboard  *nw=new Dashboard("",this);

    QString name("");
  	QPixmap pix;

	addTab(nw,pix,name);

	connect(nw,SIGNAL(selectionChanged(VInfo_ptr)),
			this,SIGNAL(selectionChanged(VInfo_ptr)));

    connect(nw->titleHandler(),SIGNAL(changed(DashboardTitle*)),
                this,SLOT(slotTabTitle(DashboardTitle*)));

	connect(nw,SIGNAL(contentsChanged()),
			this,SIGNAL(contentsChanged()));

    adjustTabTitle();

    //init the title
    slotTabTitle(nw->titleHandler());

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
    for(int i=0; i < count(); i++)
    {
        if(QWidget *w=widget(i))
        {
            if(Dashboard* nw=static_cast<Dashboard*>(w))
                nw->titleHandler()->setCurrent(i==currentIndex());
        }
     }

    Q_EMIT currentWidgetChanged();
  	//setDefaults(this);
}

void  NodePanel::slotNewTab()
{
    Dashboard *w=addWidget("");
    w->addWidget("tree");
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

bool NodePanel::selectInTreeView(VInfo_ptr info)
{
    for(int i=0; i < count(); i++)
    {
        if(QWidget *w=widget(i))
        {
            if(Dashboard* nw=static_cast<Dashboard*>(w))
                if(nw->selectInTreeView(info))
                {
                    setCurrentIndex(i);
                    return true;
                }
        }
    }
    return false;
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
    {
        w->addWidget(type);
    }
}

void NodePanel::openDialog(VInfo_ptr info,const std::string& type)
{
	if(Dashboard *w=currentDashboard())
	{
	    w->slotPopInfoPanel(info,QString::fromStdString(type));
	}
}
void NodePanel::addSearchDialog()
{
	if(Dashboard *w=currentDashboard())
	{
		w->addSearchDialog();
	}
}


void NodePanel::slotTabTitle(DashboardTitle* t)
{
    int index=indexOfWidget(t->dashboard());
    if(index != -1)
	{
        setTabText(index,t->title());
        setTabIcon(index,t->pix());
        setTabToolTip(index,t->tooltip());
        setTabWht(index,t->desc());
        setTabData(index,t->descPix());
	}
}

void NodePanel::slotTabRemoved()
{
   adjustTabTitle();
}

int NodePanel::tabAreaWidth() const
{
    return width()-80;
}

void NodePanel::adjustTabTitle()
{
    if(count() > 1)
    {       
        int tabWidth=tabAreaWidth()/count();
        if(tabWidth < 30)
            tabWidth=30;

        for(int i=0; i < count(); i++)
        {
            if(QWidget *w=widget(i))
            {
                if(Dashboard* nw=static_cast<Dashboard*>(w))
                    nw->titleHandler()->setMaxPixWidth(tabWidth);
            }
        }
    }
}

void NodePanel::resizeEvent(QResizeEvent *e)
{
    if(abs(e->oldSize().width()-width()) > 5)
        adjustTabTitle();
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

void NodePanel::init()
{
	Dashboard* nw=addWidget("");
	if(nw)
	{
		nw->addWidget("tree");
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


