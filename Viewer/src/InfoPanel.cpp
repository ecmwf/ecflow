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

#include "JobItemWidget.hpp"
#include "MessageItemWidget.hpp"
#include "ScriptItemWidget.hpp"
#include "VariableItemWidget.hpp"


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
	setupUi(this);

	frozenTb->setChecked(false);
	detachedTb->setChecked(false);


	connect(tab_,SIGNAL(currentChanged(int)),
				    this,SLOT(slotCurrentWidgetChanged(int)));


	connect(bcWidget_,SIGNAL(selected(VInfo_ptr)),
			this,SIGNAL(selectionChanged(VInfo_ptr)));


	//Check which roles are allowed
	QStringList ids;
	ids << "overview" << "variable" << "message" << "script" << "job" << "output" << "why" << "manual" << "trigger";

	//Set tabs according to the current set of roles
	adjust(ids);

	//Build dockWidget
	/*dock_ = new QDockWidget(tr("Info panel"),parent);
	dock_->setAllowedAreas(Qt::BottomDockWidgetArea |
	                                           Qt::RightDockWidgetArea);
	dock_->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
	       		QDockWidget::DockWidgetFloatable);

	main->addDockWidget(Qt::BottomDockWidgetArea, dock_);*/

}

InfoPanel::~InfoPanel()
{
	Q_FOREACH(InfoPanelItemHandler *d,items_)
			delete d;
}

void InfoPanel::clear()
{
	currentNode_.reset();
	tab_->clear();
	bcWidget_->setPath(VInfo_ptr());
}

void InfoPanel::reset(VInfo_ptr node)
{
		currentNode_=node;

		//Check which roles are allowed
		QStringList ids;
		ids << "overview" << "variable" << "message" << "script" << "job" << "output" << "why" << "manual" << "trigger";

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

		bcWidget_->setPath(node);
}

void InfoPanel::slotReload(VInfo_ptr node)
{
	//When the panel is disabled (i.e. the dock parent is hidden) or the mode is detached it ccanoot receive
	//the reload request
	if(!isEnabled() || detached())
		return;

	reset(node);
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
		Q_FOREACH(QString id, ids)
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

InfoPanelItemHandler* InfoPanel::findHandler(QString id)
{
	Q_FOREACH(InfoPanelItemHandler *d,items_)
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




