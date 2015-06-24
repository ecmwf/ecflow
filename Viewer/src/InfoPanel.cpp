//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "InfoPanel.hpp"

#include <QDebug>
#include <QToolButton>
#include <QVBoxLayout>

#include "InfoPanelHandler.hpp"
#include "JobItemWidget.hpp"
#include "MessageItemWidget.hpp"
#include "ScriptItemWidget.hpp"
#include "VariableItemWidget.hpp"
#include "VSettings.hpp"


InfoPanelDock::InfoPanelDock(QString label,QWidget * parent) :
   QDockWidget(label,parent),
   infoPanel_(0),
   closed_(true)
{
	setAllowedAreas(Qt::BottomDockWidgetArea |
				Qt::RightDockWidgetArea);

	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
				QDockWidget::DockWidgetFloatable);

	infoPanel_=new InfoPanel(parent);

	//Just to be sure that init is correct
	if(isVisible())
	{
		closed_=false;
		infoPanel_->setEnabled(true);
	}
	else
	{
		infoPanel_->setEnabled(false);
	}


	setWidget(infoPanel_);
}

void InfoPanelDock::showEvent(QShowEvent* event)
{
	if(closed_==true)
	{
		closed_=false;
		infoPanel_->setEnabled(true);
	}

	QWidget::showEvent(event);
}

void InfoPanelDock::closeEvent (QCloseEvent *event)
{
	QWidget::closeEvent(event);

	closed_=true;
	infoPanel_->clear();
	infoPanel_->setEnabled(false);
}

//==============================================
//
// InfoPanelItemHandler
//
//==============================================

QWidget* InfoPanelItemHandler::widget()
{
	return item_->realWidget();
}

bool InfoPanelItemHandler::match(const std::vector<InfoPanelDef*>& ids) const
{
	return (std::find(ids.begin(),ids.end(),def_) != ids.end());
}

void  InfoPanelItemHandler::addToTab(QTabWidget *tab)
{
	tab->addTab(item_->realWidget(),QString::fromStdString(def_->label()));
}

//==============================================
//
// InfoPanel
//
//==============================================

InfoPanel::InfoPanel(QWidget* parent) :
  DashboardWidget(parent)

{
	setupUi(this);

	frozenTb->setChecked(false);
	detachedTb->setChecked(false);


	connect(tab_,SIGNAL(currentChanged(int)),
				    this,SLOT(slotCurrentWidgetChanged(int)));


	connect(bcWidget_,SIGNAL(selected(VInfo_ptr)),
			this,SIGNAL(selectionChanged(VInfo_ptr)));
}

InfoPanel::~InfoPanel()
{
	Q_FOREACH(InfoPanelItemHandler *d,items_)
			delete d;
}

void InfoPanel::clear()
{
	//release info
	info_.reset();

	//Clear the tab contents
	for(int i=0; i < tab_->count(); i++)
	{
		if(InfoPanelItem* item=findItem(tab_->widget(i)))
		{
			item->clearContents();
		}
	}
	//Clear the tabs
	tab_->clear();

	//Clear the breadcrumbs
	bcWidget_->clear();
}

//TODO: It should be the slot
void InfoPanel::reset(VInfo_ptr info)
{
	//Set info
    adjustInfo(info);

    //Set tabs
	adjustTabs(info);

	//Set breadcrumbs
	bcWidget_->setPath(info);
}

//This slot is called when the info objec is selected
void InfoPanel::slotReload(VInfo_ptr node)
{
	//When the panel is disabled (i.e. the dock parent is hidden) or the mode is detached it cannot receive
	//the reload request
	if(!isEnabled() || detached())
		return;

	reset(node);
}

//Set the new VInfo object.
//We also we need to manage the node observers. The InfoItem
//will be the observer of the server of the object stored in
//the new VInfo
void InfoPanel::adjustInfo(VInfo_ptr info)
{
	ServerHandler *server=0;
  	bool sameServer=false;

  	//Check if there is data in info
  	if(info.get())
  	{
  		server=info->server();

  		sameServer=(info_)?(info_->server() == server):false;

  		//Handle observers
  		if(!sameServer)
  		{
  			if(info_ && info_->server())
  			{
  				info_->server()->removeServerObserver(this);
  				//info_->server()->removeNodeObserver(this);
  			}

  			info->server()->addServerObserver(this);
  			//info->server()->addNodeObserver(this);
  		}
  	}
  	//If the there is no data we clean everything and return
  	else
  	{
  	  	if(info_ && info_->server())
  	  	{
  	  		info_->server()->removeServerObserver(this);
  	  		//info_->server()->removeNodeObserver(this);
  	  	}
  	}

  	//Set the info
  	info_=info;
}

void InfoPanel::adjustTabs(VInfo_ptr info)
{
	//Set tabs according to the current set of roles
	std::vector<InfoPanelDef*> ids;
	InfoPanelHandler::instance()->visible(info,ids);

	for(int i=0; i < ids.size(); i++)
	{
		qDebug() << ids[i]->name().c_str();
	}

	int match=0;
	for(int i=0; i < tab_->count(); i++)
	{
		if(InfoPanelItemHandler* d=findHandler(tab_->widget(i)))
		{
			//Clear the contents
			d->item()->clearContents();

			if(d->match(ids))
				match++;
		}
	}

	//Remember the current widget
	QWidget *current=tab_->currentWidget();
	InfoPanelItem* currentItem=findItem(current);

	//A new set of tabs is needed!
	if(match != ids.size())
	{
		//Remember the current widget
		//QWidget *current=tab_->currentWidget();

		//Remove the pages but does not delete them
        tab_->clear();

        for(std::vector<InfoPanelDef*>::iterator it=ids.begin(); it != ids.end(); it++)
		{
			if(InfoPanelItemHandler* d=findHandler(*it))
			{
				d->addToTab(tab_);
			}
		}

		//Try to set the previous current widget as current again
		bool hasCurrent=false;
		for(int i=0 ; i < tab_->count(); i++)
		{
			if(tab_->widget(i) == current)
			{
				tab_->setCurrentIndex(i);

				hasCurrent=true;
				break;
			}
		}
		//If the current widget is not present select the first
		if(!hasCurrent && tab_->count() >0)
		{
			tab_->setCurrentIndex(0);
			currentItem=findItem(tab_->widget(0));
		}
	}

	if(currentItem)
	{
		currentItem->reload(info);
	}
}

InfoPanelItem* InfoPanel::findItem(QWidget* w)
{
	if(!w)
		return 0;

	Q_FOREACH(InfoPanelItemHandler *d,items_)
	{
			if(d->widget() == w)
					return d->item();
	}

	return 0;
}

InfoPanelItemHandler* InfoPanel::findHandler(QWidget* w)
{
	if(!w)
		return 0;

	Q_FOREACH(InfoPanelItemHandler *d,items_)
	{
			if(d->widget() == w)
					return d;
	}

	return 0;
}

InfoPanelItemHandler* InfoPanel::findHandler(InfoPanelDef* def)
{
	Q_FOREACH(InfoPanelItemHandler *d,items_)
	{
			if(d->def() == def)
					return d;
	}

	return createHandler(def);
}

InfoPanelItemHandler* InfoPanel::createHandler(InfoPanelDef* def)
{
	if(InfoPanelItem *iw=InfoPanelItemFactory::create(def->name()))
	{
		InfoPanelItemHandler* h=new InfoPanelItemHandler(def,iw);
		items_ << h;
		return h;
	}
	return 0;
}


void InfoPanel::slotCurrentWidgetChanged(int idx)
{
	if(!info_.get())
		return;

	if(InfoPanelItem* item=findItem(tab_->widget(idx)))
	{
		qDebug() << "tab changed" << item->loaded();
		if(!item->loaded())
			item->reload(info_);
	}
}

void InfoPanel::on_addTb_clicked()
{
	//InfoPanel* p=new InfoPanel(parent);
	//emit newPanelAdded(p);
}

bool InfoPanel::frozen() const
{
	return frozenTb->isChecked();
}

bool InfoPanel::detached() const
{
	return detachedTb->isChecked();
}

void InfoPanel::detached(bool b)
{
	detachedTb->setChecked(b);
}


void InfoPanel::notifyDataLost(VInfo* info)
{
	if(info_ && info_.get() == info)
	{
		clear();
	}
}


//-------------------------------------------------
// ServerObserver methods
//-------------------------------------------------

void InfoPanel::notifyDefsChanged(ServerHandler *server, const std::vector<ecf::Aspect::Type>& aspect)
{
	if(info_.get())
	{
		if(info_->server() && info_->server() == server)
		{
			//Dispatch the change
			Q_FOREACH(InfoPanelItemHandler *item,items_)
			{
				item->item()->defsChanged(aspect);
			}
		}
	}
}

void InfoPanel::notifyServerDelete(ServerHandler* server)
{
	if(info_ && info_->server() == server)
	{
		clear();
	}
}

void InfoPanel::notifyBeginServerClear(ServerHandler* server)
{
}

void InfoPanel::notifyEndServerScan(ServerHandler* server)
{

}

void InfoPanel::notifyServerConnectState(ServerHandler* server)
{
	if(info_.get())
	{
		if(info_->server() && info_->server() == server)
		{
			//Dispatch the change
			Q_FOREACH(InfoPanelItemHandler *item,items_)
			{
				item->item()->connectStateChanged();
			}
		}
	}
}

void InfoPanel::notifyServerSuiteFilterChanged(ServerHandler* server)
{
	if(info_.get())
	{
		if(info_->server() && info_->server() == server)
		{
			//Dispatch the change
			Q_FOREACH(InfoPanelItemHandler *item,items_)
			{
				item->item()->suiteFilterChanged();
			}
		}
	}

}

void InfoPanel::writeSettings(VSettings* vs)
{
	vs->put("type","info");
	vs->put("dockId",id_);
}

void InfoPanel::readSettings(VSettings* vs)
{
	std::string type=vs->get<std::string>("type","");
	if(type != "info")
	{
		return;
	}
}
