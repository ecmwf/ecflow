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
#include <QVBoxLayout>

#include "JobItemWidget.hpp"
#include "InfoItemWidget.hpp"
#include "MessageItemWidget.hpp"
#include "ScriptItemWidget.hpp"
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

bool InfoPanelItemHandler::match(QStringList ids) const
{
	return ids.contains(id_);
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
	QVBoxLayout *layout=new QVBoxLayout(this);
	setLayout(layout);

	tab_=new QTabWidget(this);
	layout->addWidget(tab_);

	connect(tab_,SIGNAL(currentChanged(int)),
			    this,SLOT(slotCurrentWidgetChanged(int)));

	//Check which roles are allowed
	QStringList ids;
	ids << "info" << "variable" << "message" << "script" << "job" << "output" << "why" << "manual" << "trigger";

	//Set tabs according to the current set of roles
	adjust(ids);

}

InfoPanel::~InfoPanel()
{
	foreach(InfoPanelItemHandler *d,items_)
			delete d;
}


void InfoPanel::slotReload(ViewNodeInfo_ptr node)
{
	currentNode_=node;

	//Check which roles are allowed
	QStringList ids;
	ids << "info" << "variable" << "message" << "script" << "job" << "output" << "why" << "manual" << "trigger";

	//Set tabs according to the current set of roles
	adjust(ids);

	qDebug() << "current" << tab_->currentIndex();

	//Reload the current widget in the tab and clears the others
	for(int i=0; i < tab_->count(); i++)
	{
			if(InfoPanelItem* item=findItem(tab_->widget(i)))
			{
				if(i== tab_->currentIndex())
				{
					qDebug() << "reload" << i;
					item->reload(node);
				}
				else
					item->clearContents();
			}
		}
}

void InfoPanel::adjust(QStringList ids)
{
	int match=0;
	for(int i=0; i < tab_->count(); i++)
	{
		if(InfoPanelItemHandler* d=findHandler(tab_->widget(i)))
		{
			if(d->match(ids))
					match++;
		}
	}

	if(match != ids.count())
	{
		//Remember the current widget
		QWidget *current=tab_->currentWidget();

		tab_->clear();
		foreach(QString id, ids)
		{
			if(InfoPanelItemHandler* d=findHandler(id))
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
			tab_->setCurrentIndex(0);
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

InfoPanelItemHandler* InfoPanel::findHandler(QString id)
{
	foreach(InfoPanelItemHandler *d,items_)
	{
			if(d->id() == id)
					return d;
	}

	return createHandler(id);
}

InfoPanelItemHandler* InfoPanel::createHandler(QString id)
{
	qDebug() << "create" << id;

	if(InfoPanelItem *iw=InfoPanelItemFactory::create(id.toStdString()))
	{
		qDebug() <<"widget ok";
		InfoPanelItemHandler* h=new InfoPanelItemHandler(id,id,iw);
		items_ << h;
		return h;
	}
	return 0;
}


void InfoPanel::slotCurrentWidgetChanged(int idx)
{
	if(!currentNode_.get())
		return;

	if(InfoPanelItem* item=findItem(tab_->widget(idx)))
	{
		qDebug() << "tab changed" << item->loaded();
		if(!item->loaded())
			item->reload(currentNode_);
	}
}
