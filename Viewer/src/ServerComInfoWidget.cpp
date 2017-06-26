//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ServerComInfoWidget.hpp"

#include <QHBoxLayout>
#include <QPainter>
#include <QTimer>
#include <QToolButton>

#include "ServerHandler.hpp"
#include "UIDebug.hpp"
#include "VNode.hpp"

struct ServerUpdateData
{
    QRect br_;
    int prevTime_;
    int nextTime_;
    QString prevText_;
    QString nextText_;
    float prog_;
};

ServerRefreshInfoWidget::ServerRefreshInfoWidget(QAction* refreshAction,QWidget *parent) :
    QWidget(parent),
    refreshAction_(refreshAction)
{
    Q_ASSERT(refreshAction_);

    QHBoxLayout *hb=new QHBoxLayout(this);
    hb->setContentsMargins(0,0,0,0);
    hb->setSpacing(0);

    //QToolButton* refreshTb=new QToolButton(this);
    //refreshTb->setAutoRaise(true);
    //refreshTb->setDefaultAction(refreshAction_);
    //hb->addWidget(refreshTb);

    infoW_=new ServerComLineDisplay(this);
    hb->addWidget(infoW_);

}

void ServerRefreshInfoWidget::setServer(ServerHandler* server)
{
    infoW_->setServer(server);
}

ServerComLineDisplay::ServerComLineDisplay(QWidget *parent) :
    QWidget(parent),
    server_(0),
    font_(QFont()),
    fm_(font_)
{
    font_=QFont();
    font_.setPointSize(font_.pointSize()-1);
    fm_=QFontMetrics(font_);

    int width_=200;
    int height_=fm_.height()+4+6;

    timer_=new QTimer(this);

    connect(timer_,SIGNAL(timeout()),
            this,SLOT(update()));

    timer_->setInterval(1000);
    timer_->start();

    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    setMinimumSize(width_,height_);
}

#if 0
void ServerComLineDisplay::crePixmap()
{
    int width_=200;
    int height_=25;
    pix_=QPixmap(width_,height_);
    pix_.fill(Qt::transparent);

    QPainter painter(&pix_);
}
#endif

void ServerComLineDisplay::setServer(ServerHandler* server)
{
    server_=server;
}

void ServerComLineDisplay::paintEvent(QPaintEvent*)
{
    if(!server_)
        return;

    int currentRight=0;
    int offset=4;
    int yPadding=2;

    int period,total,drift,toNext;
    bool hasUpdate=server_->updateInfo(period,total,drift,toNext);
    bool hasDrift=(hasUpdate && drift > 0);
    int h=height()-2*yPadding;
    int progHeight=h;
    int driftHeight=0;


    if(hasDrift)
    {
        progHeight=fm_.height()+2;
        driftHeight=h-progHeight;
    }

    //progress rect - with the server name in it
    QRect progRect=QRect(offset,yPadding,
                       fm_.width("ABCDEABDCEABCD"),
                       progHeight);

    QString progText=fm_.elidedText(QString::fromStdString(server_->name()),
                               Qt::ElideRight,progRect.width());

    //drif rect
    QRect driftRect;
    if(hasDrift)
    {
        driftRect=progRect;
        driftRect.setY(yPadding+progHeight);
        driftRect.setHeight(driftHeight);
    }

   //toNext rect
   currentRight=progRect.x()+progRect.width()+offset;

   QString toNextText=QString::number(total) + "s";
   if(hasUpdate)
       toNextText=formatTime(toNext);

   QRect toNextRect=progRect;
   toNextRect.setX(currentRight);
   toNextRect.setWidth(fm_.width(toNextText));

   //Start painting
   QPainter painter(this);
   painter.setFont(font_);

   //Draw progress rect - bg
   painter.fillRect(progRect,QColor(240,240,240));


   //Progress border
   painter.setBrush(Qt::NoBrush);
   painter.setPen(QColor(240,240,240));
   painter.drawRect(progRect);

   //Progress text - i.e. the server name
  // painter.setPen(server_->vRoot()->stateFontColour());
  // painter.drawText(progRect.adjusted(2,0,0,0),Qt::AlignLeft | Qt::AlignVCenter,progText);

   //Drift
   if(hasDrift)
   {
        painter.setBrush(QColor(20,20,20));
        painter.setPen(QColor(190,190,190));
        painter.drawRect(driftRect);

        float periodRatio=(static_cast<float>(period)/static_cast<float>(total));
        Q_ASSERT(periodRatio >= 0. && periodRatio <= 1.0001);
        if(periodRatio >= 1.) periodRatio=1;

        int pos=static_cast<int>(periodRatio* driftRect.width());
        QRect periodRect=driftRect;
        periodRect.setWidth(pos);

        painter.setBrush(QColor(255,255,255));
        painter.setPen(QColor(190,190,190));
        painter.drawRect(periodRect);
    }

   //Draw progress rect - progress part
   if(hasUpdate)
   {
       if(toNext > total) toNext=period;
       UI_ASSERT(total >= toNext,"total=" << total << " toNext=" << toNext <<
                 " period=" << period << " drift=" << drift);

       float progress=(static_cast<float>(total-toNext)/static_cast<float>(total));
       UI_ASSERT(progress >= 0. && progress <= 1.0001, "progress=" << progress);
       if(progress >= 1.) progress=1;

       //int pos=static_cast<int>(progress* driftRect.width());
       //QRect cRect=driftRect.adjusted(0,0,-(driftRect.width()-pos),0);
       //painter.fillRect(cRect,server_->vRoot()->stateColour());

       int pos=static_cast<int>(progress* progRect.width());
       QRect cRect=progRect.adjusted(0,0,-(progRect.width()-pos),0);
       painter.fillRect(cRect,QColor(190,190,190)); //server_->vRoot()->stateColour());
   }


   //Progress text - i.e. the server name
   painter.setPen(server_->vRoot()->stateFontColour());
   painter.drawText(progRect.adjusted(2,0,0,0),Qt::AlignLeft | Qt::AlignVCenter,progText);

    //The remaining time to the next update
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QColor(43,97,158));
    //painter.drawText(toNextRect,Qt::AlignLeft | Qt::AlignVCenter,toNextText);

#if 0
    data.prevTime_=server_->secsSinceLastRefresh();
    data.nextTime_=server_->secsTillNextRefresh();

    if(data.prevTime_ >=0)
    {
        if(data.nextTime_ >=0)
        {
            data.prevText_="-" + formatTime(data.prevTime_);

            data.nextText_="-" +formatTime(data.nextTime_);
            data.prog_=(static_cast<float>(data.prevTime_)/static_cast<float>(data.prevTime_+data.nextTime_));
            data.br_.setWidth(fm.width("ABCDE")+fm.width(data.prevText_)+fm.width(data.nextText_)+2*offset);
        }
        else
        {
            data.prevText_="last update: " + formatTime(data.prevTime_);
            data.prog_=0;
            data.br_.setWidth(fm.width(data.prevText_));
        }
        currentRight=data.br_.right();
     }
     else
     {
        hasUpdate=false;
     }

     if(hasUpdate)
     {
        QPainter painter(this);
        renderServerUpdate(&painter,data);
     }

#endif

}


void ServerComLineDisplay::renderServerUpdate(QPainter* painter,const ServerUpdateData& data) const
{
    QFont font(font_);
    font.setPointSize(font_.pointSize()-1);
    QFontMetrics fm(font);
    painter->setFont(font);
    painter->setPen(Qt::black);

    QColor minCol=QColor(198,215,253);
    QColor maxCol=QColor(43,97,158);

    QRect r1=data.br_;
    r1.setWidth(fm.width(data.prevText_));
    painter->setPen(minCol);
    //painter->setPen(Qt::red);
    painter->drawText(r1,Qt::AlignLeft | Qt::AlignVCenter,data.prevText_);

    if(!data.prevText_.isEmpty())
    {
        QRect r2=data.br_;
        r2.setX(data.br_.right()-fm.width(data.nextText_));
        //painter->setPen(QColor(1,128,73));
        painter->setPen(maxCol);
        painter->drawText(r2,Qt::AlignRight | Qt::AlignVCenter,data.nextText_);

        int dh=(data.br_.height()-fm.height()+1)/2;
        QRect r=data.br_.adjusted(r1.width()+4,2*dh,-r2.width()-4,-2*dh);

        int pos=static_cast<int>(data.prog_* r.width());
        QRect rPrev=r.adjusted(0,0,-(r.width()-pos),0);

        QLinearGradient grad;
        grad.setCoordinateMode(QGradient::ObjectBoundingMode);
        grad.setStart(0,0);
        grad.setFinalStop(1,0);
        QColor posCol=interpolate(minCol,maxCol,data.prog_);

        grad.setColorAt(0,minCol);
        grad.setColorAt(1,posCol);
        painter->setPen(Qt::NoPen);
        painter->setBrush(grad);
        painter->drawRect(rPrev);

        painter->setBrush(Qt::NoBrush);
        painter->setPen(QColor(190,190,190));
        painter->drawRect(r);
    }
}


QString ServerComLineDisplay::formatTime(int timeInSec) const
{
    int h=timeInSec/3600;
    int r=timeInSec%3600;
    int m=r/60;
    int s=r%60;

    QTime t(h,m,s);
    if(h > 0)
       return "> " + QString::number(h) + "h";
    else if(m > 0)
       return "> " + QString::number(m) + "m";
    else
       return QString("-%1 s").arg(s, 2, 10, QChar('0'));

    return QString();
}

QColor ServerComLineDisplay::interpolate(QColor c1,QColor c2,float r) const
{
    return QColor::fromRgbF(c1.redF()+r*(c2.redF()-c1.redF()),
                  c1.greenF()+r*(c2.greenF()-c1.greenF()),
                  c1.blueF()+r*(c2.blueF()-c1.blueF()));
}


ServerComActivityLine::ServerComActivityLine(QWidget *parent) :
    QWidget(parent),
    server_(0),
    font_(QFont()),
    fm_(font_)
{
    font_.setPointSize(font_.pointSize()-1);
    fm_=QFontMetrics(font_);

    int width_=200;
    int height_=fm_.height()+4+6;

    timer_=new QTimer(this);

    connect(timer_,SIGNAL(timeout()),
            this,SLOT(update()));

    timer_->setInterval(1000);
    timer_->start();

    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    setMinimumSize(width_,height_);
}

void ServerComActivityLine::setServer(ServerHandler* server)
{
    server_=server;
}

void ServerComActivityLine::paintEvent(QPaintEvent*)
{
#if 0
    if(!server_)
        return;

    int currentRight=0;
    int offset=4;
    int yPadding=2;

    int period,total,drift,toNext;
    bool hasUpdate=server_->updateInfo(period,total,drift,toNext);
    bool hasDrift=(hasUpdate && drift > 0);
    int h=height()-2*yPadding;
    int progHeight=h;
    int driftHeight=0;


    if(hasDrift)
    {
        progHeight=fm_.height()+2;
        driftHeight=h-progHeight;
    }

    //progress rect - with the server name in it
    QRect progRect=QRect(offset,yPadding,
                       fm_.width("ABCDEABDCEABCD"),
                       progHeight);

    QString progText=fm_.elidedText(QString::fromStdString(server_->name()),
                               Qt::ElideRight,progRect.width());

    //drif rect
    QRect driftRect;
    if(hasDrift)
    {
        driftRect=progRect;
        driftRect.setY(yPadding+progHeight);
        driftRect.setHeight(driftHeight);
    }

   //toNext rect
   currentRight=progRect.x()+progRect.width()+offset;

   QString toNextText=QString::number(total) + "s";
   if(hasUpdate)
       toNextText=formatTime(toNext);

   QRect toNextRect=progRect;
   toNextRect.setX(currentRight);
   toNextRect.setWidth(fm_.wi

#endif


}



