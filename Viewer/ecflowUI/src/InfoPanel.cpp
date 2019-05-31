//============================================================================
// Copyright 2009-2019 ECMWF.
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
#include "NodePathWidget.hpp"
#include "ServerHandler.hpp"
#include "SessionHandler.hpp"
#include "UiLog.hpp"
#include "UIDebug.hpp"
#include "VSettings.hpp"
#include "WidgetNameProvider.hpp"

//#define _UI_INFOPANEL_DEBUG

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
  DashboardWidget("info",parent)
{
	setupUi(this);

    bcWidget_=new NodePathWidget(this);

    connect(tab_,SIGNAL(currentChanged(int)),
            this,SLOT(slotCurrentWidgetChanged(int)));

    connect(bcWidget_,SIGNAL(selected(VInfo_ptr)),
        this,SLOT(slotReloadFromBc(VInfo_ptr)));

	tab_->setIconSize(QSize(16,16));

    messageLabel_->hide();	

	//Initialise action state
    actionBreadcrumbs_->setChecked(bcWidget_->isGuiMode());
	actionFrozen_->setChecked(false);

    WidgetNameProvider::nameChildren(this);

    //Create the handler for all the possible panels!
    for(auto it : InfoPanelHandler::instance()->panels())
    {
        createHandler(it);
    }
}

InfoPanel::~InfoPanel()
{
    localClear();

	Q_FOREACH(InfoPanelItemHandler *d,items_)
		delete d;
}

QMenu* InfoPanel::buildOptionsMenu()
{
    auto *menu=new QMenu(this);
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

    //Add the bc to the titlebar. This will reparent the bcWidget!!! So we must not
    //access it in the destructor!!!
    tw->setBcWidget(bcWidget_);
}

//When the infopanel is in a dialog we need to add the optionsTb to the dialog.
void InfoPanel::populateDialog()
{
    setInDialog(true);

    //Add the bcWidget_ to the top of the dialogue
    bcWidget_->useTransparentBg(false);
    verticalLayout_->insertWidget(0,bcWidget_);

    QMenu *menu=buildOptionsMenu();

    QWidget *cornerW=new QWidget(this);
    auto *hb=new QHBoxLayout(cornerW);
    hb->setContentsMargins(0,0,0,0);
    hb->setSpacing(1);

    auto *detachedTb=new QToolButton(this);
    detachedTb->setAutoRaise(true);
    detachedTb->setDefaultAction(detachedAction_);
    hb->addWidget(detachedTb);
    setDetached(true); //by default a dialog is detached!

    auto* optionsTb=new QToolButton(this);
    optionsTb->setAutoRaise(true);
    optionsTb->setIcon(QPixmap(":/viewer/cogwheel.svg"));
    optionsTb->setPopupMode(QToolButton::InstantPopup);
    optionsTb->setToolTip(tr("Options"));
    optionsTb->setMenu(menu);
    hb->addWidget(optionsTb);

    tab_->setCornerWidget(cornerW);

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
    localClear();

    //Clear the breadcrumbs
    bcWidget_->clear();
}

//This is safe to call from the destructor
void InfoPanel::localClear()
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
}

bool InfoPanel::reset(VInfo_ptr info)
{
    if(info_ && info)
    {
        //UiLog().dbg() << "path: " << info_->path() << " " << info->path();

        if(*(info_.get()) == *(info.get()))
            return false;

        //it can happen that the stored info was not yet updated after a
        //server reload. If there is chance for it we try to regain its data and
        //comapare it again to the incoming node
        else if(info_->server() == info->server() &&
                info_->storedNodePath() == info->storedNodePath() &&
                !info_->node() && info->node())
        {
            info_->regainData();
            if(info_->node() == info->node())
                return false;
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

    return true;
}

bool InfoPanel::reloadCore(VInfo_ptr info)
{
    bool retVal=false; // no real reset/reload happened!!

    lastBroadcastInfo_ = info;

    //When the mode is detached it cannot receive
    //the reload request
    if(info_ && detached())
        return retVal;

    if(info && info->isAttribute())
    {
        retVal=reset(VInfo::createParent(info));
    }
    else
    {
        retVal=reset(info);
    }

    return retVal;
}

//This slot is called when the info object is selected in another panel
void InfoPanel::slotReload(VInfo_ptr info)
{
    reloadCore(info);
}

void InfoPanel::slotReloadFromBc(VInfo_ptr info)
{
    lastBroadcastInfo_ = info;

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

#ifdef _UI_INFOPANEL_DEBUG
	for(int i=0; i < ids.size(); i++)
	{
        UiLog().dbg() << "InfoPanel --> tab: " << ids[i]->name();
	}
#endif

	int match=0;
	for(int i=0; i < tab_->count(); i++)
	{
		if(InfoPanelItemHandler* d=findHandler(tab_->widget(i)))
        {
            //We only keep the tab as it is when all these match:
            //1. it is a server tab that must always be visible whatever
            //node is selected
            //2. the server data in the tab has to be kept (i.e. static)
            //3. the server in the new info  object is the same as in the tab

            //Sanity check
            if(d->item()->keepServerDataOnLoad())
            {
                UI_ASSERT(d->match(ids),"d=" << d->def()->name()) ;
            }

            //Disable and force to clear the contents when it is not the case
            //described above
            if(!(d->item()->keepServerDataOnLoad() &&
                 d->item()->hasSameContents(info)))
            {
                d->item()->setActive(false);
            }

			if(d->match(ids))
				match++;
		}
	}

	//Remember the current widget
	QWidget *current=tab_->currentWidget();
	InfoPanelItem* currentItem=findItem(current);

	//A new set of tabs is needed!
    if(tab_->count() != static_cast<int>(ids.size()) || match != static_cast<int>(ids.size()))
	{
		//We set this flag true so that the change of the current tab should not
		//trigger a reload! We want to reload the current tab only after the
		//tab adjustment.
		tabBeingAdjusted_=true;

		//Remove the pages but does not delete them.
		clearTab();

        for(auto & id : ids)
		{
			if(InfoPanelItemHandler* d=findHandler(id))
			{
                d->addToTab(tab_);
                d->item()->setActive(true);
			}
		}

		//Try to set the previous current widget as current again
        currentItem=nullptr;
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
        if(info_ && currentItem->keepServerDataOnLoad())
            currentItem->notifyInfoChanged(info_->nodePath());

        //currentItem->reload(info);
	}
}

InfoPanelItem* InfoPanel::findItem(QWidget* w)
{
	if(!w)
		return nullptr;

	Q_FOREACH(InfoPanelItemHandler *d,items_)
	{
        if(d->widget() == w)
            return d->item();
	}

	return nullptr;
}

InfoPanelItemHandler* InfoPanel::findHandler(QWidget* w)
{
	if(!w)
		return nullptr;

	Q_FOREACH(InfoPanelItemHandler *d,items_)
	{
        if(d->widget() == w)
            return d;
	}

	return nullptr;
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
        WidgetNameProvider::nameChildren(iw->realWidget());
        iw->setOwner(this);
        iw->setFrozen(frozen());
        iw->setDetached(detached(),true);

		//iw will be added to the tab so the tab will be its parent. Moreover
        //the tab will stay its parent even if iw got removed from the tab!
		//So when the tab is deleted all the iw-s will be correctly deleted as well.

		auto* h=new InfoPanelItemHandler(def,iw);
		items_ << h;
		return h;
	}
	return nullptr;
}

//We clicked on another tab
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
    // if we leave the detached node we need to set the current
    // broadcast node on the infopanel!!!
    if(!detached() && lastBroadcastInfo_)
    {
        // just set the detached state on the panel items (tabs)
        // without doing an update
        Q_FOREACH(InfoPanelItemHandler *item,items_)
        {
            item->item()->setDetached(detached(), false);
        }
        lastBroadcastInfo_->regainData();
        if(reloadCore(lastBroadcastInfo_))
        {
           return;
        }
    }

    //if we are here no reset/update happened so we just need to
    //notify/update all the items!!!
    Q_FOREACH(InfoPanelItemHandler *item,items_)
    {
        item->item()->setDetached(detached(), true);
    }
    updateTitle();
}

void InfoPanel::on_actionBreadcrumbs__toggled(bool b)
{
    if(isInDialog())
    {
        bcWidget_->setVisible(b);
    }
    else
    {
        if(b)
        {
            bcWidget_->setMode(NodePathWidget::GuiMode);
        }
        else
        {
            bcWidget_->setMode(NodePathWidget::TextMode);
        }
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
    if(isInDialog())
    {
        QString txt;
        if(frozen())
            txt+="(frozen) ";

        if(info_)
        {
            txt+=QString::fromStdString(info_->path());
        }

        Q_EMIT titleUpdated(txt);
    }
}

void InfoPanel::relayInfoPanelCommand(VInfo_ptr info,QString cmd)
{
    Q_EMIT popInfoPanel(info,cmd);
}

void InfoPanel::relayDashboardCommand(VInfo_ptr info,QString cmd)
{
    Q_EMIT dashboardCommand(info,cmd);
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

    if(info_)
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

void InfoPanel::notifyEndServerSync(ServerHandler* server)
{
	//TODO: does frozen make sense in this case?
	if(frozen())
		return;

    if(info_)
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

void InfoPanel::writeSettings(VComboSettings* vs)
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

void InfoPanel::readSettings(VComboSettings* vs)
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
    actionBreadcrumbs_->setChecked(bcWidget_->isGuiMode());

	actionFrozen_->setChecked(vs->getAsBool("frozen",frozen()));

    DashboardWidget::readSettings(vs);

    Q_FOREACH(InfoPanelItemHandler *d,items_)
    {
        if(d->item())
            d->item()->readSettings(vs);
    }
}

void InfoPanel::writeSettingsForDialog()
{
    SessionItem* cs=SessionHandler::instance()->current();
    assert(cs);
    VSettings vs(cs->infoPanelDialogFile());

    vs.putAsBool("breadcrumbs",bcWidget_->isVisible());
    vs.putAsBool("frozen",frozen());
    vs.putAsBool("detached",detached());
    vs.write();
}

void InfoPanel::readSettingsForDialog()
{
    SessionItem* cs=SessionHandler::instance()->current();
    assert(cs);
    VSettings vs(cs->infoPanelDialogFile());
    vs.read(false);

    actionBreadcrumbs_->setChecked(vs.getAsBool("breadcrumbs",true));
    bcWidget_->setVisible(actionBreadcrumbs_->isChecked());

    actionFrozen_->setChecked(vs.getAsBool("frozen",frozen()));
    detachedAction_->setChecked(vs.getAsBool("detached",detached()));
}
