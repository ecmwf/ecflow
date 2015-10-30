//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "DashboardTitle.hpp"

#include "ServerHandler.hpp"
#include "VNode.hpp"

#include <QPainter>

DashboardTitle::DashboardTitle(ServerFilter* filter,QObject *parent) :
  QObject(parent),
  filter_(filter)
{
	filter_->addObserver(this);
}

DashboardTitle::~DashboardTitle()
{
	clear();
}

void DashboardTitle::clear()
{
	if(!filter_)
		return;

	filter_->removeObserver(this);
	for(std::vector<ServerItem*>::const_iterator it=filter_->items().begin(); it !=filter_->items().end(); ++it)
	{
		ServerHandler* s=(*it)->serverHandler();
		s->removeServerObserver(this);
	}

	filter_=0;
}


void DashboardTitle::notifyServerFilterAdded(ServerItem* item)
{
	ServerHandler* s=item->serverHandler();
	s->addServerObserver(this);
	updateTitle();
}

void DashboardTitle::notifyServerFilterRemoved(ServerItem* item)
{
	ServerHandler* s=item->serverHandler();
	s->removeServerObserver(this);
	updateTitle();
}

void DashboardTitle::notifyServerFilterChanged(ServerItem*)
{
	updateTitle();
}

void DashboardTitle::notifyServerFilterDelete()
{
	clear();
}

void DashboardTitle::notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a)
{
	updateTitle();
}

void DashboardTitle::notifyServerDelete(ServerHandler* server)
{
	//server->removeServerObserver(this);
}

void DashboardTitle::notifyEndServerClear(ServerHandler* server)
{
	updateTitle();
}

void DashboardTitle::notifyEndServerScan(ServerHandler* server)
{
	updateTitle();
}

void DashboardTitle::notifyServerConnectState(ServerHandler* server)
{
	updateTitle();
}

void DashboardTitle::notifyServerActivityChanged(ServerHandler* server)
{
	updateTitle();
}

void DashboardTitle::updateTitle()
{
	if(!filter_)
		return;

	const int marginX=0;
	const int marginY=0;
	const int textMarginX=2;
	const int textMarginY=1;
	const int gap=8;
	QFont f;
	QFontMetrics fm(f);

	//Compute the pixmap size
	int w=0;
	int h=fm.height()+2*marginY+2*textMarginY+1;

	QStringList texts;
	QList<QRect> textRects;
	QList<QRect> fillRects;
	QList<QColor> fillColors;
	QList<QColor> textColors;

	int xp=marginX;
	int yp=marginY;
	const std::vector<ServerItem*> items=filter_->items();
	for(std::vector<ServerItem*>::const_iterator it=items.begin(); it != items.end(); ++it)
	{
		//Get text
		QString str=QString::fromStdString((*it)->name());
		textRects << QRect(xp+textMarginX,yp+textMarginY,fm.width(str),fm.height());
		texts << str;

		//Get server status
		ServerHandler* server=(*it)->serverHandler();
		fillColors << server->vRoot()->stateColour();
		textColors << server->vRoot()->stateFontColour();
		fillRects << QRect(xp,yp,fm.width(str)+2*textMarginX,fm.height()+2*textMarginY);

		xp=fillRects.back().right()+gap;
	}
	w=xp-gap+marginX+2;

	//Render the pixmap
	QPixmap pix(w,h);
	pix.fill(Qt::transparent);
	QPainter painter(&pix);

	for(int i=0; i < texts.count(); i++)
	{
		QColor c=fillColors[i].darker(110);

		QColor cp=fillColors[i].darker(120);

		painter.setPen(cp);
		painter.setBrush(fillColors[i]);
		painter.drawRoundedRect(fillRects[i],0,0);

		QRect half=fillRects[i];
		half.setY(half.center().y());
		half.adjust(1,0,-1,-1);
		painter.setPen(Qt::NoPen);
		painter.fillRect(half,c);

		painter.setPen(QPen(textColors[i]));
		painter.drawText(textRects[i],texts[i]);
	}

	Q_EMIT changed("",pix);
}

