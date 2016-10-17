//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "InfoPanel.hpp"

#include <QDebug>
#include <QMenu>
#include <QToolButton>
#include <QVBoxLayout>

#include "DashboardDock.hpp"
#include "InfoPanelItem.hpp"
#include "InfoPanelHandler.hpp"
#include "ServerHandler.hpp"
#include "UserMessage.hpp"
#include "VSettings.hpp"

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
	int idx=tab->addTab(item_->realWidget(),QString::fromStdString(def_->label()));
	tab->setTabIcon(idx,QPixmap(":/viewer/" + QString::fromStdString(def_->icon())));
}

//==============================================
//
// InfoPanel
//
//==============================================

InfoPanel::InfoPanel(QWidget* parent) :
  DashboardWidget("info",parent),
  tabBeingCleared_(false),
  tabBeingAdjusted_(false)
{
	setupUi(this);

    connect(tab_,SIGNAL(currentChanged(int)),
            this,SLOT(slotCurrentWidgetChanged(int)));

    connect(bcWidget_,SIGNAL(selected(VInfo_ptr)),
        this,SLOT(slotReloadFromBc(VInfo_ptr)));

	tab_->setIconSize(QSize(16,16));

    messageLabel_->hide();	

	//Initialise action state
	actionBreadcrumbs_->setChecked(bcWidget_->active());
	actionFrozen_->setChecked(false);
}

InfoPanel::~InfoPanel()
{
	clear();

	Q_FOREACH(InfoPanelItemHandler *d,items_)
		delete d;
}

QMenu* InfoPanel::buildOptionsMenu()
{
    QMenu *menu=new QMenu(this);
    menu->setTearOffEnabled(true);

    menu->addAction(actionBreadcrumbs_);
    menu->addAction(actionFrozen_);

    return menu;
}

//When the infopanel is in a dockwidget we add the option actions
//to the dockwidget title bar widget
void InfoPanel::populateDockTitleBar(DashboardDockTitleWidget* tw)
{
    QMenu *menu=buildOptionsMenu();

	//Sets the menu on the toolbutton
	tw->optionsTb()->setMenu(menu);

	//This will set the title
	updateTitle();
}

//When the infopanel is in a dialog we need to add the optionsTb to the dialog.
void InfoPanel::populateDialog()
{
    QMenu *menu=buildOptionsMenu();

    detachedAction_->setIcon(QIcon());
    menu->addAction(detachedAction_);

    QToolButton* optionsTb=new QToolButton(this);
    optionsTb->setAutoRaise(true);
    optionsTb->setIcon(QPixmap(":/viewer/configure.svg"));
    optionsTb->setPopupMode(QToolButton::InstantPopup);
    optionsTb->setToolTip(tr("Options"));
    optionsTb->setMenu(menu);

    tab_->setCornerWidget(optionsTb);

    //This will set the dialog title
    updateTitle();
}

void InfoPanel::setCurrent(const std::string& name)
{
	for(int i=0; i < tab_->count(); i++)
	{
		if(InfoPanelItemHandler* d=findHandler(tab_->widget(i)))
		{
			//Clear the contents
			if(d->def() && d->def()->name() == name)
			{
				tab_->setCurrentIndex(i);
			}
		}
	}
}

void InfoPanel::clear()
{
    messageLabel_->hide();
    messageLabel_->clear();

    //Unregister from observer lists
	if(info_ && info_.get())
	{
		if(info_->server())
		{
			info_->server()->removeServerObserver(this);
		}

		info_->removeObserver(this);
	}

	//release info
	info_.reset();

	//Clear the tab contents
	for(int i=0; i < tab_->count(); i++)
	{
		if(InfoPanelItem* item=findItem(tab_->widget(i)))
		{
            //Diable and clear the contents
            item->setActive(false);
		}
	}
	//Clear the tabs
	clearTab();

	//Clear the breadcrumbs
	bcWidget_->clear();
}

//TODO: It should be the slot
void InfoPanel::reset(VInfo_ptr info)
{
    if(info_ && info)
    {
        //UserMessage::debug("path: " + info_->path() + " " + info->path());

        if(*(info_.get()) == *(info.get()))
            return;

        //it can happen that the stored info was not yet updated after a
        //server reload. If there is chance for it we try to regain its data and
        //comapare it again to the incoming node
        else if(info_->server() == info->server() &&
                info_->storedNodePath() == info->storedNodePath() &&
                !info_->node() && info->node())
        {
            info_->regainData();
            if(info_->node() == info->node())
                return;
        }
    }

    messageLabel_->hide();
    messageLabel_->clear();

    //Set info
    adjustInfo(info);

    //Set tabs
	adjustTabs(info);

	//Set breadcrumbs
	bcWidget_->setPath(info);

    updateTitle();
}

//This slot is called when the info object is selected in another panel
void InfoPanel::slotReload(VInfo_ptr info)
{
    //When the mode is detached it cannot receive
	//the reload request
    if(info_ && detached())
		return;

    if(info && info->isAttribute())
    {
        reset(VInfo::createParent(info));
    }
    else
    {
        reset(info);
    }
}


void InfoPanel::slotReloadFromBc(VInfo_ptr info)
{
    reset(info);
    if(info_)
       Q_EMIT selectionChanged(info_);
}

void InfoPanel::linkSelected(VInfo_ptr info)
{
    //Here info can be an attribute!
    slotReload(info);
    if(info_ && info)
       Q_EMIT selectionChanged(info);
}

//Set the new VInfo object.
//We also we need to manage the node observers. The InfoItem
//will be the observer of the server of the object stored in
//the new VInfo
void InfoPanel::adjustInfo(VInfo_ptr info)
{
  	//Check if there is data in info
    if(info)
  	{
  		ServerHandler *server=info->server();

  		bool sameServer=(info_)?(info_->server() == server):false;

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
  	if(info_)
  	{
  		info_->removeObserver(this);
  	}

  	info_=info;

  	if(info_)
  	{
  		info_->addObserver(this);
  	}

}

void InfoPanel::adjustTabs(VInfo_ptr info)
{
	//Set tabs according to the current set of roles
	std::vector<InfoPanelDef*> ids;
	InfoPanelHandler::instance()->visible(info,ids);

	for(int i=0; i < ids.size(); i++)
	{
		UserMessage::message(UserMessage::DBG,false,std::string("InfoPanel --> tab: ") + ids[i]->name());
	}

	int match=0;
	for(int i=0; i < tab_->count(); i++)
	{
		if(InfoPanelItemHandler* d=findHandler(tab_->widget(i)))
		{
			//Disable and force to clear the contents           
			d->item()->setActive(false);

			if(d->match(ids))
				match++;
		}
	}

	//Remember the current widget
	QWidget *current=tab_->currentWidget();
	InfoPanelItem* currentItem=findItem(current);

	//A new set of tabs is needed!
	if(tab_->count() != ids.size() || match != ids.size())
	{
		//We set this flag true so that the change of the current tab should not
		//trigger a reload! We want to reload the current tab only after the
		//tab adjustment.
		tabBeingAdjusted_=true;

		//Remove the pages but does not delete them.
		clearTab();

        for(std::vector<InfoPanelDef*>::iterator it=ids.begin(); it != ids.end(); ++it)
		{
			if(InfoPanelItemHandler* d=findHandler(*it))
			{
                d->addToTab(tab_);
                d->item()->setActive(true);
			}
		}

		//Try to set the previous current widget as current again
        currentItem=0;
        bool hasCurrent=false;
		for(int i=0 ; i < tab_->count(); i++)
		{
			if(tab_->widget(i) == current)
			{
				tab_->setCurrentIndex(i);
                currentItem=findItem(current);
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

		tabBeingAdjusted_=false;
	}

    //We use the same set of tabs
    else
    {
        for(int i=0; i < tab_->count(); i++)
        {
            if(InfoPanelItemHandler* d=findHandler(tab_->widget(i)))
            {
                d->item()->setActive(true);
            }
        }
    }

	//We reload the current tab
	if(currentItem)
    {
        currentItem->setSelected(true,info);
        //currentItem->reload(info);
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
        iw->setOwner(this);
        iw->setFrozen(frozen());
		iw->setDetached(detached());

		//iw will be added to the tab so the tab will be its parent. Moreover
        //the tab will stay its parent even if iw got removed from the tab!
		//So when the tab is deleted all the iw-s will be correctly deleted as well.

		InfoPanelItemHandler* h=new InfoPanelItemHandler(def,iw);
		items_ << h;
		return h;
	}
	return 0;
}


void InfoPanel::slotCurrentWidgetChanged(int idx)
{
	if(tabBeingCleared_ || tabBeingAdjusted_)
		return;

	if(!info_.get())
		return;

	if(InfoPanelItem* current=findItem(tab_->widget(idx)))
	{
        current->setSelected(true,info_);

		//Reload the item if it is needed
        /*if(!current->isSuspended() && !current->info())
            current->reload(info_);*/

        //Deselect the others
		for(int i=0; i < tab_->count(); i++)
		{
			if(InfoPanelItemHandler* d=findHandler(tab_->widget(i)))
			{
				if(d->item() != current)
                    d->item()->setSelected(false,info_);
			}
		}
	}
}

void InfoPanel::clearTab()
{
	tabBeingCleared_=true;
	tab_->clear();
	tabBeingCleared_=false;
}

void InfoPanel::detachedChanged()
{
    Q_FOREACH(InfoPanelItemHandler *item,items_)
    {
        item->item()->setDetached(detached());
    }
    updateTitle();
}

void InfoPanel::on_actionBreadcrumbs__toggled(bool b)
{
	if(b)
	{
		bcWidget_->active(true);
		bcWidget_->setPath(info_);
	}
	else
	{
		bcWidget_->active(false);
	}
}

void InfoPanel::on_actionFrozen__toggled(bool b)
{
	Q_FOREACH(InfoPanelItemHandler *item,items_)
	{
		item->item()->setFrozen(b);
	}
    updateTitle();
}

bool InfoPanel::frozen() const
{
	return actionFrozen_->isChecked();
}

void InfoPanel::updateTitle()
{
	QString baseTxt="<b>Info panel</b>";

	QString txt;
	if(frozen())
		txt+="frozen";

	if(!txt.isEmpty())
	{
		txt=baseTxt + " (" + txt + ")";
	}
	else
	{
		txt=baseTxt;
	}

    if(info_ && info_.get())
    {
        txt+=" - " + QString::fromStdString(info_->path());
    }

	Q_EMIT titleUpdated(txt);
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
	if(frozen())
		return;

    if(info_)
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

//This must be called at the beginning of a reset
void InfoPanel::notifyBeginServerClear(ServerHandler* server)
{
    if(info_)
    {
        if(info_->server() && info_->server() == server)
        {           
            messageLabel_->showWarning("Server <b>" + QString::fromStdString(server->name()) + "</b> is being reloaded. \
                   Until it is finished only <b>limited functionalty</b> is avaliable in the Info Panel!");

            messageLabel_->startLoadLabel();

            Q_FOREACH(InfoPanelItemHandler *item,items_)
            {
                item->item()->setSuspended(true,info_);
            }
        }
    }
}

//This must be called at the end of a reset
void InfoPanel::notifyEndServerScan(ServerHandler* server)
{
    if(info_)
    {
        if(info_->server() && info_->server() == server)
        {
            messageLabel_->hide();
            messageLabel_->clear();

            //We try to ressurect the info. We have to do it explicitly because it is not guaranteed
            //that notifyEndServerScan() will be first called on the VInfo then on the InfoPanel. So it
            //is possible that the node exists but is still set to NULL in VInfo.
            info_->regainData();

            //If the info is not available dataLost() might have already been called and
            //the panel was reset!
            if(!info_)
                return;

            Q_ASSERT(info_->server() && info_->node());

            //Otherwise we resume all the tabs
            Q_FOREACH(InfoPanelItemHandler *item,items_)
            {
                item->item()->setSuspended(false,info_);
            }
        }
    }
}

void InfoPanel::notifyServerConnectState(ServerHandler* server)
{
    if(frozen())
        return;

    if(info_)
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
	//TODO: does frozen make sense in this case?
	if(frozen())
		return;

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

void InfoPanel::notifyServerSyncFinished(ServerHandler* server)
{
	//TODO: does frozen make sense in this case?
	if(frozen())
		return;

	if(info_.get())
	{
		if(info_->server() && info_->server() == server)
		{
			//Dispatch the change
			Q_FOREACH(InfoPanelItemHandler *item,items_)
			{
				item->item()->serverSyncFinished();
			}
		}
	}
}

void InfoPanel::rerender()
{
	bcWidget_->rerender();
}

void InfoPanel::writeSettings(VSettings* vs)
{
	vs->put("type",type_);
	vs->put("dockId",id_);

	bcWidget_->writeSettings(vs);

	vs->putAsBool("frozen",frozen());

    DashboardWidget::writeSettings(vs);

    Q_FOREACH(InfoPanelItemHandler *d,items_)
    {
        if(d->item())
            d->item()->writeSettings(vs);
    }
}

void InfoPanel::readSettings(VSettings* vs)
{
	std::string type=vs->get<std::string>("type","");
	if(type != type_)
	{
		return;
	}

	//--------------------------
	//Breadcrumbs
	//--------------------------

	bcWidget_->readSettings(vs);

	//Synchronise the action and the breadcrumbs state
	//This will not emit the trigered signal of the action!!
	actionBreadcrumbs_->setChecked(bcWidget_->active());

	actionFrozen_->setChecked(vs->getAsBool("frozen",frozen()));

    DashboardWidget::readSettings(vs);

    Q_FOREACH(InfoPanelItemHandler *d,items_)
    {
        if(d->item())
            d->item()->readSettings(vs);
    }
}

