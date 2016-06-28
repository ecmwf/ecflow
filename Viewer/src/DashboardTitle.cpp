//============================================================================
// Copyright 2016 ECMWF.
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

#include <QDebug>
#include <QLinearGradient>
#include <QPainter>

int DashboardTitle::lighter_=150;

#define _UI_DASHBOARDTITLE_DEBUG

DashboardTitle::DashboardTitle(ServerFilter* filter,Dashboard *parent) :
  QObject(parent),
  dashboard_(parent),
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

    QStringList texts;
    QList<QColor> fillColors;
    QList<QColor> textColors;

    const std::vector<ServerItem*> items=filter_->items();
    for(std::vector<ServerItem*>::const_iterator it=items.begin(); it != items.end(); ++it)
    {
        //Get text
        QString str=QString::fromStdString((*it)->name());
        QString host=QString::fromStdString((*it)->host());
        QString port=QString::fromStdString((*it)->port());

        //Get server status
        ServerHandler* server=(*it)->serverHandler();
        fillColors << server->vRoot()->stateColour();
        textColors << server->vRoot()->stateFontColour();

        texts << str;

        //Description for tab list menu
        if(!desc_.isEmpty())
        {
            desc_+=" ";
        }
        if(filter_->itemCount() <=3)
            desc_+=str + " (" + host + "@" + port + ")";
        else
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

#if 0
    if(titleMode_ == "full")
    {
        const int marginX=0;
        const int marginY=0;
        const int textMarginX=1;
        const int textMarginY=1;
        const int gap=2;
        QFont f;
        QFontMetrics fm(f);

        //Compute the pixmap size
        int w=0;
        int h=fm.height()+2*marginY+2*textMarginY+1;

        QList<QRect> textRects;
        QList<QRect> fillRects;

        int xp=marginX;
        int yp=marginY;
        for(int i=0; i < texts.count(); i++)
        {
            if(titleMode_ == "one")
            {
                texts[i].truncate(1);
            }
            else if(titleMode_ == "two")
            {
                texts[i].truncate(2);
            }

            textRects << QRect(xp+textMarginX,yp+textMarginY,fm.width(texts[i]),fm.height());
            fillRects << QRect(xp,yp,fm.width(texts[i])+2*textMarginX,fm.height()+2*textMarginY);
            xp=fillRects.back().right()+gap;
        }

        w=xp-gap+marginX+2;

        //Render the pixmap
        pix_=QPixmap(w,h);
        pix_.fill(Qt::transparent);
        QPainter painter(&pix_);

        for(int i=0; i < texts.count(); i++)
        {
            drawServerRect(&painter,fillColors[i],fillRects[i],texts[i],textColors[i],textRects[i]);
        }
    }
#endif

    {
        const int marginX=0;
        const int marginY=0;
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
            qDebug() << i << texts[i] << dw << fm.width(texts[0]);
#endif
            QString txt=fm.elidedText(texts[i],Qt::ElideRight,dw);
#ifdef _UI_DASHBOARDTITLE_DEBUG
            qDebug() << "  " << txt << fm.width(txt);
#endif
            QString ellips(0x2026); //horizontal ellipsis

            if(txt.startsWith(ellips))
            {
                txt=texts[i].left(2);
                txt=fm.elidedText(txt,Qt::ElideRight,dw);
            }
#ifdef _UI_DASHBOARDTITLE_DEBUG
            qDebug() << "  " << txt << fm.width(txt);
#endif
            if(txt.startsWith(ellips))
            {
                txt=texts[i].left(1);
                txt=fm.elidedText(txt,Qt::ElideRight,dw);
            }
#ifdef _UI_DASHBOARDTITLE_DEBUG
            qDebug() << "  " << txt << fm.width(txt);
#endif
            if(txt.isEmpty())
            {
                txt=texts[i].left(1);
            }
#ifdef _UI_DASHBOARDTITLE_DEBUG
            qDebug() << "  " << txt << fm.width(txt);
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
            qDebug() << "  " << texts[i] << fm.width(texts[i]);
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
            //drawServerRect(&painter,fillColors[i],fillRects[i],texts[i],textColors[i],textRects[i]);

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
                QColor bg1,bg2,fg,bgBorder;
                if(current_)
                {
                    //bg1=QColor(140,140,140);
                    //bg2=QColor(120,120,120);
                    //fg=QColor(250,250,250);
                    //bgBorder=QColor(100,100,100);

                    bg1=QColor(250,250,250);
                    bg2=QColor(230,230,230);
                    fg=QColor(0,0,0);
                    bgBorder=QColor(195,195,195);
                }
                else
                {
                    bg1=QColor(220,220,220);
                    bg2=QColor(200,200,200);
                    fg=QColor(0,0,0);
                    bgBorder=QColor(195,195,195);
                }

                QLinearGradient grad;
                grad.setCoordinateMode(QGradient::ObjectBoundingMode);
                grad.setStart(0,0);
                grad.setFinalStop(0,1);

                grad.setColorAt(0,bg1);
                grad.setColorAt(1,bg2);

                painter.setBrush(QBrush(grad));
                painter.setPen(bgBorder);
                painter.drawRect(textRects[i]);

                painter.setPen(fg);
                painter.drawText(textRects[i],Qt::AlignCenter,texts[i]);

                QColor borderCol=bg.darker(150);
                painter.setPen(borderCol);
                painter.setBrush(bg);
                painter.drawRect(fillRects[i].adjusted(2,0,-2,0));
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


#if 0
void DashboardTitle::drawServerRect(QPainter *painter,QColor bg,QRect fillRect,QString text,QColor textCol, QRect textRect)
{
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

    useStateGrad_=false;

    QBrush bgBrush;
    if(useStateGrad_)
    {
        grad.setColorAt(0,bgLight);
        grad.setColorAt(1,bg);
        bgBrush=QBrush(grad);
    }
    else
        bgBrush=QBrush(bg);

    painter->setPen(borderCol);
    painter->setBrush(bgBrush);
    painter->drawRect(fillRect);

    if(!text.isEmpty())
    {
        painter->setPen(QPen(textCol));
        painter->drawText(textRect,Qt::AlignCenter,text);
    }
}
#endif







