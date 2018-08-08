//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "DashboardTitle.hpp"

#include "Dashboard.hpp"
#include "ServerFilter.hpp"
#include "ServerHandler.hpp"
#include "VNode.hpp"
#include "UiLog.hpp"

#include <QLinearGradient>
#include <QPainter>

int DashboardTitle::lighter_=150;

//#define _UI_DASHBOARDTITLE_DEBUG

DashboardTitle::DashboardTitle(ServerFilter* filter,Dashboard *parent) :
  QObject(parent),
  dashboard_(parent),
  filter_(filter),
  maxPixWidth_(0),
  current_(false)
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
	for(auto it : filter_->items())
	{
		ServerHandler* s=it->serverHandler();
		s->removeServerObserver(this);
	}

	filter_=nullptr;
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

void DashboardTitle::setMaxPixWidth(int w)
{
    maxPixWidth_=w;
    updateTitle();
}

void DashboardTitle::setCurrent(bool b)
{
    if(b != current_)
    {
        current_=b;
        updateTitle();
    }
}

int DashboardTitle::fullWidth() const
{
    QFont f;
    QFontMetrics fm(f);
    const int gap=1;
    const int padding=10;
    int w=0;
    for(auto i : filter_->items())
    {
        QString str=QString::fromStdString(i->name());
        int tw=fm.width(str);
        if(tw > w) w=tw;
    }

    return 10+filter_->items().size()*(w+padding)+(filter_->items().size()-1)*gap;
}

void DashboardTitle::updateTitle()
{
    pix_=QPixmap();
    tooltip_.clear();
    title_.clear();
    desc_.clear();
    descPix_=QPixmap();

    if(!filter_ || filter_->itemCount()==0)
    {
        tooltip_=tr("No server is loaded in this this tab");
        desc_=tr("No server loaded in tab");
        Q_EMIT changed(this);
        return;
    }

    if(maxPixWidth_ == 0)
        return;

    QStringList texts;
    QList<QColor> fillColors;
    QList<QColor> textColors;

    const std::vector<ServerItem*>& items=filter_->items();
    for(auto item : items)
    {
        //Get text
        QString str=QString::fromStdString(item->name());
        QString host=QString::fromStdString(item->host());
        QString port=QString::fromStdString(item->port());

        //Get server status
        ServerHandler* server=item->serverHandler();
        fillColors << server->vRoot()->stateColour();
        textColors << server->vRoot()->stateFontColour();

        texts << str;

        //Description for tab list menu
        if(!desc_.isEmpty())
        {
            desc_+=", ";
        }
        desc_+=str;

        //Tooltip
        if(!tooltip_.isEmpty())
        {
            tooltip_+="<br>-------------------<br>";
        }
        tooltip_+=server->vRoot()->toolTip();
    }

    int num=texts.count();
    Q_ASSERT(num>0);

    {
        const int marginX=0;      
        const int gap=1;

        int maxBandH=2;
        int bandH=(current_)?maxBandH:2;

        QFont f;
        QFontMetrics fm(f);

        QList<QRect> textRects;
        QList<QRect> fillRects;

        //Compute the pixmap size
        int h=fm.height()+bandH+1;
        //int w=fm.width("ABCD...")+(num-1);
        int w=maxPixWidth_-10;
        int dw=w/num;

        int xp=0;
        int yp=0;
        bool noText=false;

        for(int i=0; i < texts.count(); i++)
        {
#ifdef _UI_DASHBOARDTITLE_DEBUG
            UiLog().dbg() << i << texts[i] << dw << fm.width(texts[0]);
#endif
            QString txt=fm.elidedText(texts[i],Qt::ElideRight,dw);
#ifdef _UI_DASHBOARDTITLE_DEBUG
            UiLog().dbg() << "  " << txt << fm.width(txt);
#endif
            QString ellips(0x2026); //horizontal ellipsis

            if(txt.startsWith(ellips))
            {
                txt=texts[i].left(2);
                txt=fm.elidedText(txt,Qt::ElideRight,dw);
            }
#ifdef _UI_DASHBOARDTITLE_DEBUG
            UiLog().dbg() << "  " << txt << fm.width(txt);
#endif
            if(txt.startsWith(ellips))
            {
                txt=texts[i].left(1);
                txt=fm.elidedText(txt,Qt::ElideRight,dw);
            }
#ifdef _UI_DASHBOARDTITLE_DEBUG
            UiLog().dbg()<< "  " << txt << fm.width(txt);
#endif
            if(txt.isEmpty())
            {
                txt=texts[i].left(1);
            }
#ifdef _UI_DASHBOARDTITLE_DEBUG
            UiLog().dbg() << "  " << txt << fm.width(txt);
#endif
            if(fm.width(txt) > dw)
            {
                texts[i]="";
                noText=true;
            }
            else
            {
                texts[i]=txt;
            }
#ifdef _UI_DASHBOARDTITLE_DEBUG
            UiLog().dbg() << "  " << texts[i] << fm.width(texts[i]);
#endif
            textRects << QRect(xp,yp,dw,fm.height());
            fillRects << QRect(xp,yp+fm.height(),dw,bandH);
            xp=fillRects.back().right()+gap;
        }

        w=xp-gap+marginX+2;

        //Render the pixmap
        pix_=QPixmap(w,h);
        pix_.fill(Qt::transparent);
        QPainter painter(&pix_);

        for(int i=0; i < texts.count(); i++)
        {       
            QColor bg=fillColors[i];

            if(noText)
            {
               QRect r=fillRects[i];
               r.setTop(0);
               r.setBottom(h-2);
               fillRects[i]=r;

               QColor bgLight;
               if(bg.value() < 235)
                   bgLight=bg.lighter(130);
               else
                   bgLight=bg.lighter(lighter_);

               QLinearGradient grad;
               grad.setCoordinateMode(QGradient::ObjectBoundingMode);
               grad.setStart(0,0);
               grad.setFinalStop(0,1);

               grad.setColorAt(0,bgLight);
               grad.setColorAt(1,bg);
               QBrush bgBrush(grad);

               QColor borderCol=bg.darker(150);
               painter.setPen(borderCol);
               painter.setBrush(bgBrush);
               painter.drawRect(fillRects[i]);
            }
            else
            {                
                QColor fg=QColor(0,0,0);
                painter.setPen(fg);
                painter.drawText(textRects[i],Qt::AlignCenter,texts[i]);

                QColor borderCol=bg.darker(140);
                painter.setPen(borderCol);
                painter.setBrush(bg);
                painter.drawRect(fillRects[i]);

                if( i >0)
                {
                    painter.setPen(QColor(140,140,140));
                    painter.drawLine(fillRects[i].left(),fillRects[i].bottom(),
                                     fillRects[i].left(),textRects[i].center().y());
                }
            }
        }
    }

    //Desc pix
    {
        QFont f;
        QFontMetrics fm(f);//Compute the pixmap size
        int h=fm.height()+2;
        //int w=fm.width("ABCD...")+(num-1);
        int w=h*5/4;
        int dw=w/num;

        descPix_=QPixmap(w+1,h+1);
        descPix_.fill(Qt::transparent);
        QPainter painter(&descPix_);

        int xp=0;
        for(int i=0; i < fillColors.count(); i++)
        {
            QRect r(xp,0,dw,h);
            QColor bg=fillColors[i];
            QColor bgLight;
            if(bg.value() < 235)
                bgLight=bg.lighter(130);
            else
                bgLight=bg.lighter(lighter_);

            QColor borderCol=bg.darker(150);
            QLinearGradient grad;
            grad.setCoordinateMode(QGradient::ObjectBoundingMode);
            grad.setStart(0,0);
            grad.setFinalStop(0,1);

            grad.setColorAt(0,bgLight);
            grad.setColorAt(1,bg);
            QBrush bgBrush(grad);

            painter.setPen(borderCol);
            painter.setBrush(bgBrush);
            painter.drawRect(r);
            xp+=dw;
        }

    }

    if(tooltip_.isEmpty())
        tooltip_=tr("No server is loaded in this this tab");

    if(desc_.isEmpty())
        desc_=tr("No server loaded in tab");

    Q_EMIT changed(this);
}





