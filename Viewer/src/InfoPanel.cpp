//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "InfoPanel.hpp"

#include <QVBoxLayout>

#include "VariableItemWidget.hpp"

//==============================================
//
// InfoPanelItemHandler
//
//==============================================

QWidget* InfoPanelItemHandler::widget()
{
	return item_->realWidget();
}

bool InfoPanelItemHandler::match(QList<Viewer::ItemRole> roles) const
{
	return roles.contains(role_);
}

void  InfoPanelItemHandler::addToTab(QTabWidget *tab)
{
	tab->addTab(item_->realWidget(),label_);
}

//==============================================
//
// InfoPanel
//
//==============================================


InfoPanel::InfoPanel(QWidget* parent) :
  QWidget(parent)

{
	connect(this,SIGNAL(currentIndexChanged(int)),
		    this,SLOT(slotCurrentWidgetChanged(int)));

	connect(this,SIGNAL(newTabRequested()),
		    this,SLOT(slotNewTab()));

	QVBoxLayout *layout=new QVBoxLayout(this);
	setLayout(layout);

	tab_=new QTabWidget(this);
	layout->addWidget(tab_);

	items_ << new InfoPanelItemHandler(Viewer::VariableRole,tr("Variables"),
										  new VariableItemWidget(this));

}

InfoPanel::~InfoPanel()
{
	foreach(InfoPanelItemHandler *d,items_)
			delete d;
}


void InfoPanel::slotReload(ViewNodeInfo_ptr node)
{
	//Check which roles are allowed
	QList<Viewer::ItemRole> roles;
	roles << Viewer::VariableRole;

	//Set tabs according to the current set of roles
	adjustToRoles(roles);

	//Reload all the widgets in the tab
	for(int i=0; i < tab_->count(); i++)
	{
			if(InfoPanelItem* item=findItem(tab_->widget(i)))
			{
				item->reload(node);
			}
		}

}

void InfoPanel::adjustToRoles(QList<Viewer::ItemRole> roles)
{
	int match=0;
	for(int i=0; i < tab_->count(); i++)
	{
		if(InfoPanelItemHandler* d=findHandler(tab_->widget(i)))
		{
			if(d->match(roles))
					match++;
		}
	}

	if(match != roles.count())
	{
		tab_->clear();
		foreach(Viewer::ItemRole role, roles)
		{
			if(InfoPanelItemHandler* d=findHandler(role))
			{
					d->addToTab(tab_);
			}
		}
	}
}

InfoPanelItem* InfoPanel::findItem(QWidget* w)
{
	if(!w)
		return 0;

	foreach(InfoPanelItemHandler *d,items_)
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

	foreach(InfoPanelItemHandler *d,items_)
	{
			if(d->widget() == w)
					return d;
	}

	return 0;
}

InfoPanelItemHandler* InfoPanel::findHandler(Viewer::ItemRole role)
{
	foreach(InfoPanelItemHandler *d,items_)
	{
			if(d->role() == role)
					return d;
	}

	return 0;
}

