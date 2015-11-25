//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "FilterWidget.hpp"

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#include "VNState.hpp"
#include "VAttribute.hpp"
#include "VConfig.hpp"
#include "VIcon.hpp"
#include "VFilter.hpp"

#include "ServerItem.hpp"
#include "ServerFilter.hpp"

//===========================================
//
// VParamFilterMenu
//
//===========================================

VParamFilterMenu::VParamFilterMenu(QMenu * parent,VParamSet* filter,QString title,ItemMode itemMode,DecorMode decorMode) :
 	menu_(parent),
	filter_(filter),
	itemMode_(itemMode),
	decorMode_(decorMode)
{
	buildTitle(title,parent);

	QAction* acSep = new QAction(this);
	acSep->setSeparator(true);
	menu_->addAction(acSep);

	//Param name must be unique
	for(std::set<VParam*>::const_iterator it=filter_->all().begin(); it != filter_->all().end(); ++it)
	{
		addAction((*it)->label(),
				  (*it)->name());
	}

	if(decorMode_ == ColourDecor)
	{
		QLinearGradient grad;
		grad.setCoordinateMode(QGradient::ObjectBoundingMode);
		grad.setStart(0,0);
		grad.setFinalStop(0,1);

		int lighter=150;
		bool useStateGrad=true;
		if(VProperty* p=VConfig::instance()->find("view.common.node_gradient"))
		{
		    useStateGrad=p->value().toBool();
		}

		QFont f;
		QFontMetrics fm(f);
		int pixSize=fm.height()-2;

		Q_FOREACH(QAction* ac,menu_->actions())
		{
			if(!ac->isSeparator())
			{
				if(VNState* vs=VNState::find(ac->data().toString().toStdString()))
				{
	    		    //Fill rect
				    QColor bg=vs->colour();
				    QColor bgLight=bg.lighter(lighter);
				    QColor border=bg.darker(125);
				    QBrush bgBrush;
				    if(useStateGrad)
				    {
				       grad.setColorAt(0,bgLight);
				       grad.setColorAt(1,bg);
				       bgBrush=QBrush(grad);
				    }
				    else
				       bgBrush=QBrush(bg);

					QPixmap pix(pixSize,pixSize);
					QPainter painter(&pix);

					QRect fillRect(0,0,pixSize,pixSize);
					painter.fillRect(fillRect,bgBrush);
					painter.setPen(border);
					painter.drawRect(fillRect.adjusted(0,0,-1,-1));
					ac->setIcon(pix);
				}
			}
		}
	}

	QAction *ac = new QAction(this);
	ac->setSeparator(true);
	menu_->addAction(ac);

	if(itemMode_ == FilterMode)
	{
		ac = new QAction(this);
		ac->setText(tr("Clear filter"));
		menu_->addAction(ac);
		connect(ac,SIGNAL(triggered(bool)),
			this,SLOT(slotUnselectAll(bool)));
	}
	else
	{
		ac = new QAction(this);
		ac->setText(tr("Show all"));
		menu_->addAction(ac);
		connect(ac,SIGNAL(triggered(bool)),
					this,SLOT(slotSelectAll(bool)));

		ac = new QAction(this);
		ac->setText(tr("Hide all"));
		menu_->addAction(ac);
		connect(ac,SIGNAL(triggered(bool)),
			this,SLOT(slotUnselectAll(bool)));

	}

	reload();
}

void VParamFilterMenu::buildTitle(QString title,QMenu* parent)
{
	QLabel* titleLabel=new QLabel(title,menu_);
	QFont f=menu_->font();
	f.setBold(true);
	titleLabel->setFont(f);
	titleLabel->setAlignment(Qt::AlignHCenter);
	titleLabel->setAutoFillBackground(true);
	QPalette pal=titleLabel->palette();
	QColor winCol=menu_->palette().color(QPalette::Window);
	pal.setColor(QPalette::Window,QColor(237,238,238));
	titleLabel->setPalette(pal);

	int titlePadding=3;
	int topMargin=2;
	if(parent && parent->isTearOffEnabled())
	{
		titlePadding=1;
		topMargin=0;
	}

	QString titleQss="QLabel {padding: " + QString::number(titlePadding) + "px;}";
	titleLabel->setStyleSheet(titleQss);

	QWidget *w=new QWidget(menu_);
	QVBoxLayout *vb=new QVBoxLayout(w);
	vb->setContentsMargins(2,topMargin,2,2);
	//vb->addSpacing(2);
	vb->addWidget(titleLabel);
	//vb->addSpacing(2);

	QWidgetAction *titleAc = new QWidgetAction(menu_);
	//Qt doc says: the ownership of the widget is passed to the widgetaction.
	//So when the action is deleted it will be deleted as well.
	titleAc->setDefaultWidget(w);
	//titleAc->setEnabled(false);
	menu_->addAction(titleAc);
}


void VParamFilterMenu::addAction(QString name,QString id)
{
	QAction *ac = new QAction(this);
	ac->setText(name);
	ac->setData(id);
	ac->setCheckable(true);
	ac->setChecked(false);

	menu_->addAction(ac);

	//It will not be emitted when setChecked is called!
    connect(ac,SIGNAL(triggered(bool)),
					this,SLOT(slotChanged(bool)));
}

void VParamFilterMenu::slotSelectAll(bool)
{
	Q_FOREACH(QAction* ac,menu_->actions())
	{
		if(!ac->isSeparator() &&
			ac->isCheckable())
		{
			ac->setChecked(true);
		}
	}

	slotChanged(true);
}

void VParamFilterMenu::slotUnselectAll(bool)
{
	Q_FOREACH(QAction* ac,menu_->actions())
	{
		if(!ac->isSeparator() &&
			ac->isCheckable())
		{
			ac->setChecked(false);
		}
	}

	slotChanged(true);
}

void VParamFilterMenu::slotChanged(bool)
{
	std::set<std::string> items;
	Q_FOREACH(QAction* ac,menu_->actions())
	{
		if(!ac->isSeparator() &&
			ac->isCheckable() && ac->isChecked())
		{
			items.insert(ac->data().toString().toStdString());
		}
	}

	if(filter_)
		filter_->current(items);
}

void VParamFilterMenu::reload()
{
	Q_FOREACH(QAction* ac,menu_->actions())
	{
		if(!ac->isSeparator())
		{
			ac->setChecked(filter_->isSet(ac->data().toString().toStdString()));
		}
	}
}


/*
//===========================================
//
// StateFilterMenu
//
//===========================================

StateFilterMenu::StateFilterMenu(QMenu * parent) :
		VParamFilterMenu(parent,VNState::filterItems())
{
	Q_FOREACH(QAction* ac,menu_->actions())
	{
		if(!ac->isSeparator())
		{
			if(VNState* vs=VNState::find(ac->data().toString().toStdString()))
			{
				QPixmap pix(10,10);
				QPainter painter(&pix);
				pix.fill(vs->colour());
				painter.setPen(Qt::black);
				painter.drawRect(0,0,9,9);
				ac->setIcon(pix);
			}
		}
	}
}

//===========================================
//
// AttributeFilterMenu
//
//===========================================

AttributeFilterMenu::AttributeFilterMenu(QMenu * parent) :
		VParamFilterMenu(parent,VAttribute::filterItems())
{
}

//===========================================
//
// IconFilterMenu
//
//===========================================

IconFilterMenu::IconFilterMenu(QMenu * parent) :
		VParamFilterMenu(parent,VIcon::filterItems())
{
}
*/


//===========================================
//
// ServerFilterMenu
//
//===========================================

ServerFilterMenu::ServerFilterMenu(QMenu * parent) :
 	menu_(parent),
	filter_(NULL)
{
	loadFont_.setBold(true);

	allMenu_=new QMenu("All servers",menu_);
	menu_->addMenu(allMenu_);

	QAction* acFavSep = new QAction(this);
	acFavSep->setSeparator(true);
	menu_->addAction(acFavSep);

	QAction* acFavTitle = new QAction(this);
	acFavTitle->setText(tr("Favourite or loaded servers"));
	QFont f=acFavTitle->font();
	f.setBold(true);
	acFavTitle->setFont(f);

	menu_->addAction(acFavTitle);

	init();

	ServerList::instance()->addObserver(this);
}

ServerFilterMenu::~ServerFilterMenu()
{
	ServerList::instance()->removeObserver(this);
	if(filter_)
		filter_->removeObserver(this);
}

void ServerFilterMenu::aboutToDestroy()
{
	ServerList::instance()->removeObserver(this);
	if(filter_)
		filter_->removeObserver(this);

	clear();
}

void ServerFilterMenu::clear()
{
	Q_FOREACH(QAction* ac,acAllMap_)
	{
		delete ac;
	}
	acAllMap_.clear();

	clearFavourite();
}

void ServerFilterMenu::clearFavourite()
{
	Q_FOREACH(QAction* ac,acFavMap_)
	{
		delete ac;
	}
	acFavMap_.clear();
}

void ServerFilterMenu::init()
{
	clear();

	for(int i=0; i < ServerList::instance()->count(); i++)
	{
		ServerItem* item=ServerList::instance()->itemAt(i);
		QString name=QString::fromStdString(item->name());
		QAction *ac=createAction(name,i);
		acAllMap_[name]=ac;
	}

	Q_FOREACH(QAction *ac,acAllMap_)
	{
		allMenu_->addAction(ac);
	}

	buildFavourite();
}

void ServerFilterMenu::buildFavourite()
{
	clearFavourite();

	for(int i=0; i < ServerList::instance()->count(); i++)
	{
		ServerItem* item=ServerList::instance()->itemAt(i);
		if(item->isFavourite() || (filter_ && filter_->isFiltered(item)))
		{
			QString name=QString::fromStdString(item->name());
			acFavMap_[name]=createAction(name,i);
		}
	}

	Q_FOREACH(QAction *ac,acFavMap_)
	{
		menu_->addAction(ac);
	}

}

QAction* ServerFilterMenu::createAction(QString name,int id)
{
	QAction *ac = new QAction(this);
	ac->setText(name);
	ac->setData(id);
	ac->setCheckable(true);
	ac->setChecked(false);

	//It will not be emitted when setChecked is called!!
    connect(ac,SIGNAL(triggered(bool)),
					this,SLOT(slotChanged(bool)));

    return ac;
}


void ServerFilterMenu::slotChanged(bool)
{
	if(!filter_)
		return;

	if(QAction *ac=static_cast<QAction*>(sender()))
	{
		if(ac->isSeparator()) return;

		if(ServerItem *item=ServerList::instance()->itemAt(ac->data().toInt()))
		{
			QString name=ac->text();
			bool checked=ac->isChecked();
			if(checked)
				filter_->addServer(item);
			else
				filter_->removeServer(item);

			//At this point the action (ac) might be deleted so
			//we need to use the name and check state for syncing
			syncActionState(name,checked);
		}
	}
}

void ServerFilterMenu::syncActionState(QString name,bool checked)
{
	QMap<QString,QAction*>::const_iterator it = acAllMap_.find(name);
	if(it != acAllMap_.end() && it.value()->isChecked() != checked)
	{
		//Triggered() will not be called!!
		it.value()->setChecked(checked);
	}

	it = acFavMap_.find(name);
	if(it != acFavMap_.end() && it.value()->isChecked() != checked)
	{
		//Triggered() will not be called!!
		it.value()->setChecked(checked);
	}
}

//Reset actions state when a new filter is loaded
void ServerFilterMenu::reload(ServerFilter *filter)
{
	if(filter_)
		filter_->removeObserver(this);

	filter_=filter;

	if(filter_)
		filter_->addObserver(this);

	reload();
}

//Reset actions state when a new filter is loaded
void ServerFilterMenu::reload()
{
	buildFavourite();

	QMap<QString,QAction*>::const_iterator it=acAllMap_.constBegin();
	while(it != acAllMap_.constEnd())
	{
		if(ServerItem *item=ServerList::instance()->find(it.value()->text().toStdString()))
		{
			bool current=it.value()->isChecked();
			if(current != filter_->isFiltered(item))
			{
				//Triggered() will not be called!!
				it.value()->setChecked(!current);
				it.value()->setFont(current?font_:loadFont_);
			}
		}
		++it;
	}

	it=acFavMap_.constBegin();
	while(it != acFavMap_.constEnd())
	{
		if(ServerItem *item=ServerList::instance()->find(it.value()->text().toStdString()))
		{
			bool current=it.value()->isChecked();
			if(current != filter_->isFiltered(item))
			{
				//Triggered() will not be called!!
				it.value()->setChecked(!current);
				it.value()->setFont(current?font_:loadFont_);
			}
		}
		++it;
	}
}

void ServerFilterMenu::notifyServerListChanged()
{
	init();
	reload();
}

void ServerFilterMenu::notifyServerListFavouriteChanged(ServerItem* item)
{
	reload();
}

void ServerFilterMenu::notifyServerFilterAdded(ServerItem*)
{
	reload();
}

void ServerFilterMenu::notifyServerFilterRemoved(ServerItem*)
{
	reload();
}

void ServerFilterMenu::notifyServerFilterChanged(ServerItem*)
{
	reload();
}

void ServerFilterMenu::notifyServerFilterDelete()
{
	reload(0);
}
