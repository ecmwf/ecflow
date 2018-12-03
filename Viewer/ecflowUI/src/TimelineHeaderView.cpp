//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TimelineHeaderView.hpp"

#include <QDebug>
#include <QtGlobal>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>

#include "IconProvider.hpp"
#include "TimelineModel.hpp"
#include "UiLog.hpp"
#include "ViewerUtil.hpp"
#include "VNState.hpp"


MainTimelineHeader::MainTimelineHeader(QWidget *parent) : TimelineHeader(parent)
{
    columnType_ << OtherColumn;
    columnType_ << TimelineColumn;
    columnType_ << OtherColumn;
    columnType_ << OtherColumn;
}

NodeTimelineHeader::NodeTimelineHeader(QWidget *parent) : TimelineHeader(parent)
{
    columnType_ << OtherColumn;
    columnType_ << DayColumn;
}

TimelineHeader::TimelineHeader(QWidget *parent) :
    QHeaderView(Qt::Horizontal, parent),
    fm_(QFont()),
    timelineCol_(50,50,50),
    dateTextCol_(33,95,161),
    timeTextCol_(30,30,30),
    timelineFrameBorderCol_(150,150,150),
    timelineFrameSize_(4),
    majorTickSize_(5),
    zoomCol_(224,236,248,190),
    inZoom_(false),
    zoomInAction_(0),
    zoomOutAction_(0),
    submittedMaxDuration_(-1),
    activeMaxDuration_(-1)
{
    setMouseTracking(true);

    setStretchLastSection(true);

    QColor bg0(214,214,214);
    QColor bg1(194,194,194);
    QLinearGradient gr;
    gr.setCoordinateMode(QGradient::ObjectBoundingMode);
    gr.setStart(0,0);
    gr.setFinalStop(0,1);
    gr.setColorAt(0,bg0);
    gr.setColorAt(1,bg1);
    timelineBrush_=QBrush(gr);

    font_= QFont();
    font_.setPointSize(font_.pointSize()-2);
    fm_=QFontMetrics(font_);

    zoomCursor_=QCursor(QPixmap(":/viewer/cursor_zoom.svg"));
}

QSize TimelineHeader::sizeHint() const
{    
    QSize s = QHeaderView::sizeHint(); //size();
    if(hasTimeColumn())
    {
        s.setHeight(timelineFrameSize_ + fm_.height() + 6 + majorTickSize_ + fm_.height() + 6 + timelineFrameSize_);
    }
    return s;
}

void TimelineHeader::mousePressEvent(QMouseEvent *event)
{
    //Start new zoom
    if(isZoomEnabled() &&
       !inZoom_ && event->button() == Qt::LeftButton &&
       isColumnZoomable(event->pos()) && canBeZoomed())       
    {
        zoomStartPos_=event->pos();
    }
    else
    {
        QHeaderView::mousePressEvent(event);
    }
}

void TimelineHeader::mouseMoveEvent(QMouseEvent *event)
{
    //When we enter the timeline section we show a zoom cursor
    bool hasCursor=testAttribute(Qt::WA_SetCursor);

    if(!isZoomEnabled())
    {
        QHeaderView::mouseMoveEvent(event);
        return;
    }

    //When enabled ...

    if(event->buttons().testFlag(Qt::LeftButton))
    {
        int columnIndex=logicalIndexAt(zoomStartPos_);
        int secStart=sectionPosition(columnIndex);
        int secEnd=secStart+sectionSize(columnIndex);

        //If we are in resize mode
        if(!inZoom_ && zoomStartPos_.isNull())
        {
            QHeaderView::mouseMoveEvent(event);
            return;
        }

        //In timeline zoom mode we show a zoom cursor
        if(!hasCursor || cursor().shape() ==  Qt::SplitHCursor )
            setCursor(zoomCursor_);

        inZoom_=true;
        zoomEndPos_=event->pos();

        if(event->pos().x() >= secStart && event->pos().x() <= secEnd)
        {
            if(event->pos().x() < zoomStartPos_.x())
            {
                zoomEndPos_=zoomStartPos_;
            }
        }
        else if(event->pos().x() < secStart)
        {
            zoomEndPos_=zoomStartPos_;
        }
        else //if(event->pos().x() > secEnd)
        {
            zoomEndPos_=event->pos();
            zoomEndPos_.setX(secEnd);
        }

        headerDataChanged(Qt::Horizontal,columnIndex,columnIndex);

        if(columnType_[columnIndex] == TimelineColumn)
        {
            QDateTime sDt=posToDate(zoomStartPos_);
            QDateTime eDt=posToDate(zoomEndPos_);
            if(sDt.isValid() && eDt.isValid())
            {
                Q_EMIT periodBeingZoomed(sDt,eDt);
            }
        }
    }
    else
    {
        //When we enter the timeline section we show a zoom cursor
        if(isColumnZoomable(event->pos()))
        {
            if((!hasCursor || cursor().shape() ==  Qt::SplitHCursor) &&
               (canBeZoomed()))
                setCursor(zoomCursor_);
        }
        //Otherwise remove the cursor unless it is the resize indicator
        else
        {
            if(hasCursor && cursor().shape() !=  Qt::SplitHCursor)
                unsetCursor();

            QHeaderView::mouseMoveEvent(event);
        }
    }
}

void TimelineHeader::mouseReleaseEvent(QMouseEvent *event)
{
    if(inZoom_)
    {
         int columnIndex=logicalIndexAt(zoomStartPos_);

         if(columnType_[columnIndex] == TimelineColumn)
            doPeriodZoom();
    }
    else
    {
        QHeaderView::mouseReleaseEvent(event);
    }
}

void TimelineHeader::doPeriodZoom()
{
    QDateTime sDt=posToDate(zoomStartPos_);
    QDateTime eDt=posToDate(zoomEndPos_);
    zoomStartPos_=QPoint();
    zoomEndPos_=QPoint();
    inZoom_=false;
    if(sDt.isValid() && eDt.isValid())
    {
        setPeriodCore(sDt,eDt,true);
        Q_EMIT periodSelected(sDt,eDt);
    }
    else
    {
        Q_EMIT periodBeingZoomed(startDate_,endDate_);
    }

    setZoomDisabled();
}


bool TimelineHeader::canBeZoomed() const
{
    if(hasZoomableColumn())
    {
        return (endDate_.toMSecsSinceEpoch()-startDate_.toMSecsSinceEpoch()) > 60*1000;
    }
    return false;
}

void TimelineHeader::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    painter->save();
    //QHeaderView::paintSection(painter, rect, logicalIndex);
    //painter->restore();


    /*QPixmap customPix(":viewer/filter_decor.svg");
    QRect cbRect(0,0,12,12);
    cbRect.moveCenter(QPoint(rect.right()-16-6,rect.center().y()));
    customButton_[logicalIndex].setRect(cbRect);
    painter->drawPixmap(cbRect,pix);*/

    if (!rect.isValid())
        return;

     QStyleOptionHeader opt;
     initStyleOption(&opt);
     QStyle::State state = QStyle::State_None;
     if(isEnabled())
        state |= QStyle::State_Enabled;
     if(window()->isActiveWindow())
        state |= QStyle::State_Active;

    // if(isSortIndicatorShown() && sortIndicatorSection() == logicalIndex)
    //        opt.sortIndicator = (sortIndicatorOrder() == Qt::AscendingOrder)
    //                            ? QStyleOptionHeader::SortDown : QStyleOptionHeader::SortUp;

     // setup the style options structure
     //QVariant textAlignment = model->headerData(logicalIndex, d->orientation,
     //                                                 Qt::TextAlignmentRole);
     opt.rect = rect;
     opt.section = logicalIndex;
     opt.state |= state;
     //opt.textAlignment = Qt::Alignment(textAlignment.isValid()
     //                                     ? Qt::Alignment(textAlignment.toInt())
     //                                     : d->defaultAlignment);

     //opt.text = model()->headerData(logicalIndex, Qt::Horizontal),
     //                                    Qt::DisplayRole).toString();

     QVariant foregroundBrush;
     if (foregroundBrush.canConvert<QBrush>())
         opt.palette.setBrush(QPalette::ButtonText, qvariant_cast<QBrush>(foregroundBrush));

     QPointF oldBO = painter->brushOrigin();
     QVariant backgroundBrush;
     if (backgroundBrush.canConvert<QBrush>())
     {
            opt.palette.setBrush(QPalette::Button, qvariant_cast<QBrush>(backgroundBrush));
            opt.palette.setBrush(QPalette::Window, qvariant_cast<QBrush>(backgroundBrush));
            painter->setBrushOrigin(opt.rect.topLeft());
     }

    // the section position
    int visual = visualIndex(logicalIndex);
    Q_ASSERT(visual != -1);

    if (count() == 1)
        opt.position = QStyleOptionHeader::OnlyOneSection;
    else if (visual == 0)
        opt.position = QStyleOptionHeader::Beginning;
    else if (visual == count() - 1)
        opt.position = QStyleOptionHeader::End;
    else
        opt.position = QStyleOptionHeader::Middle;

    opt.orientation = Qt::Horizontal;

    // the selected position
    /*bool previousSelected = d->isSectionSelected(logicalIndex(visual - 1));
        bool nextSelected =  d->isSectionSelected(logicalIndex(visual + 1));
        if (previousSelected && nextSelected)
            opt.selectedPosition = QStyleOptionHeader::NextAndPreviousAreSelected;
        else if (previousSelected)
            opt.selectedPosition = QStyleOptionHeader::PreviousIsSelected;
        else if (nextSelected)
            opt.selectedPosition = QStyleOptionHeader::NextIsSelected;
        else
            opt.selectedPosition = QStyleOptionHeader::NotAdjacent;
    */

    // draw the section
    style()->drawControl(QStyle::CE_Header, &opt, painter, this);
    painter->setBrushOrigin(oldBO);

    painter->restore();

    int rightPos=rect.right();

    if(columnType_[logicalIndex] == TimelineColumn)
    {
        renderTimeline(rect,painter,logicalIndex);
    }
    else if(columnType_[logicalIndex] == DayColumn)
    {
        renderDay(rect, painter,logicalIndex);
    }
    else
    {
        QString text=model()->headerData(logicalIndex,Qt::Horizontal).toString();
        QRect textRect=rect;
        textRect.setRight(rightPos-5);
        painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter," " + text);
    }
}

void TimelineHeader::renderTimeline(const QRect& rect,QPainter* painter,int logicalIndex) const
{
    //painter->fillRect(rect.adjusted(0,0,0,-1),timelineFrameBgCol_);

    //The timeline area bounded by the frame
    QRect pRect=rect.adjusted(0,timelineFrameSize_,0,-timelineFrameSize_);

    //Special appearance in for the timeline area
    painter->fillRect(pRect,timelineBrush_);

    painter->setPen(QPen(timelineFrameBorderCol_));
    painter->drawLine(QPoint(rect.left(),pRect.top()),
                      QPoint(rect.right(),pRect.top()));

    painter->drawLine(QPoint(rect.left(),pRect.bottom()),
                      QPoint(rect.right(),pRect.bottom()));

    if(inZoom_)
    {
        QRect zRect=pRect; //rect.adjusted(0,zoomFrameTop,0,-zoomFrameBottom-1);
        zRect.setLeft(zoomStartPos_.x());
        zRect.setRight(zoomEndPos_.x());
        painter->fillRect(zRect,zoomCol_);
        painter->setPen(zoomCol_.darker(140));
        painter->drawRect(zRect.adjusted(0,1,0,-2));
    }

    int w=sectionSize(logicalIndex);

    //period in secs
    qint64 startSec=startDate_.toMSecsSinceEpoch()/1000;
    qint64 endSec=endDate_.toMSecsSinceEpoch()/1000;
    qint64 period=endSec-startSec;

    int minorTick=1; //in secs (it is a delta)
    int majorTick=1;  //in secs (it is a delta)
    qint64 firstTick=1; //in secs since epoch

    int hLineY=pRect.center().y()-majorTickSize_/2-1;
    int timeTextGap=3; //the gap between the top of the time text and the bottom of the major tick in pixels
    int majorTickTop=hLineY;
    int majorTickBottom=hLineY+majorTickSize_; //pRect.bottom()-fm_.height()-timeTextGap;
    int minorTickTop=hLineY;
    int minorTickBottom=majorTickBottom-3;
    int dateTextY= hLineY - (hLineY-pRect.y() - fm_.height())/2 - fm_.height() + 1;
    int timeTextY= majorTickBottom + (pRect.bottom()-majorTickBottom - fm_.height())/2 - 1;

    int timeItemW=fm_.width("223:442");
    int dateItemW=fm_.width("2229 May22");

    QList<int> majorTickSec;

    if(period < 60)
    {
        majorTickSec << 1 << 5 << 10 << 20 << 30;
    }
    else if(period < 600)
    {
        majorTickSec << 10 << 15 << 20 << 30 << 60 << 5*60 << 10*60;
    }
    if(period < 3600)
    {
        majorTickSec << 60 << 2*60 << 3*60 << 4*60 << 5*60 << 10*60 << 20*60 << 30*60;
    }
    if(period < 12*3600)
    {
        majorTickSec << 15*60 << 30*60 << 3600 << 2*3600 << 3*3600 << 4*3600 << 6*3600;
    }
    else if(period <= 86400)
    {
        majorTickSec << 3600 << 2*3600 << 3*3600 << 4*3600 << 6*3600 << 12*3600;
    }
    else if(period < 7*86400)
    {
        majorTickSec << 3600 << 2*3600 << 3*3600 << 4*3600 << 6*3600 << 12*3600 << 24*3600;
    }
    else if(period < 14*86400)
    {
        majorTickSec << 12*3600 << 24*3600 << 36*3600 << 48*3600 << 72*3600 << 96*3600;
    }
    else if(period < 28*86400)
    {
        majorTickSec << 86400 << 2*86400 << 3*86400 << 4*86400 << 5*86400 << 10*86400;
    }
    else if(period < 60*86400)
    {
        majorTickSec << 86400 << 2*86400 << 3*86400 << 4*86400 << 5*86400 << 10*86400 << 20*86400;
    }
    else if(365 * 86400)
    {
        majorTickSec << 5*86400 << 10*86400 << 20*86400 << 30*86400 << 60*86400 << 90*86400 << 180*86400;
    }
    else
    {
        majorTickSec << 30*86400 << 60*86400 << 90*86400 << 180*86400 << 365*86400;
    }

    Q_FOREACH(int mts,majorTickSec)
    {
       majorTick=mts;
       int majorTickNum=period/majorTick;
       int cover=timeItemW*majorTickNum;
       int diff=w-cover;
       if(diff > 100)
       {
           break;
       }
    }

    minorTick=majorTick/4;
    if(minorTick==0)
        minorTick = majorTick;

    firstTick=(startSec/minorTick)*minorTick+minorTick;

    //Find label positions for days
    QList<QPair<int,QString> > dateLabels;
    int dayNum=startDate_.date().daysTo(endDate_.date());

    if(dayNum == 0)
    {
        int xp=secToPos((startSec+endSec)/2-startSec,rect);
        dateLabels << qMakePair(xp,startDate_.toString("dd MMM"));
    }
    else
    {
        QDate  nextDay=startDate_.date().addDays(1);
        QDate  lastDay=endDate_.date();
        qint64 nextSec=QDateTime(nextDay).toMSecsSinceEpoch()/1000;

        if((nextSec-startSec) < 3600)
        {
            int xp=secToPos((startSec+nextSec)/2-startSec,rect);
            dateLabels << qMakePair(xp,nextDay.toString("dd MMM"));
        }

        int dayFreq=1;
        QList<int> dayFreqLst;
        dayFreqLst << 1 << 2 << 3 << 5 << 10 << 20 << 30 << 60 << 90 << 120 << 180 << 365;
        Q_FOREACH(int dfv,dayFreqLst)
        {
            if((dayNum/dfv)*dateItemW < w-100)
            {
                dayFreq=dfv;
                break;
            }
        }

        QDate firstDay=nextDay;
        for(QDate d=firstDay; d < lastDay; d=d.addDays(dayFreq))
        {
            int xp=secToPos((QDateTime(d).toMSecsSinceEpoch()/1000 +
            QDateTime(d.addDays(1)).toMSecsSinceEpoch()/1000)/2-startSec,rect);
            dateLabels << qMakePair(xp,d.toString("dd MMM"));
        }

        if(QDateTime(lastDay).toMSecsSinceEpoch()/1000 < endSec)
        {
            int xp=secToPos((QDateTime(lastDay).toMSecsSinceEpoch()/1000+endSec)/2 - startSec,rect);
            dateLabels << qMakePair(xp,lastDay.toString("dd MMM"));
        }
    }

    painter->save();
    painter->setClipRect(rect);

    //Draw date labels
    painter->setPen(dateTextCol_);
    for(int i=0; i < dateLabels.count(); i++)
    {
        int xp=dateLabels[i].first;
        int textW=fm_.width(dateLabels[i].second);
        //int yp=rect.bottom()-1-fm.height();
        painter->setFont(font_);
        painter->drawText(QRect(xp-textW/2, dateTextY,textW,fm_.height()),
                          Qt::AlignHCenter | Qt::AlignVCenter,dateLabels[i].second);
    }

    //horizontal line
    painter->setPen(timelineCol_);
    painter->drawLine(rect.x(),hLineY,rect.right(),hLineY);

    qint64 actSec=firstTick;
    Q_ASSERT(actSec >= startSec);
    painter->setPen(timeTextCol_);

    while(actSec <= endSec)
    {
        int xp=secToPos(actSec-startSec,rect);

        //draw major tick + label
        if(actSec % majorTick == 0)
        {
            painter->drawLine(xp,majorTickTop,xp,majorTickBottom);

            QString s;
            if(majorTick < 60)
            {
                s=QDateTime::fromMSecsSinceEpoch(actSec*1000,Qt::UTC).toString("H:mm:ss");
            }
            else
            {
                s=QDateTime::fromMSecsSinceEpoch(actSec*1000,Qt::UTC).toString("H:mm");
            }

            int textW=fm_.width(s);
            painter->setFont(font_);
            painter->drawText(QRect(xp-textW/2, timeTextY,textW,fm_.height()),
                              Qt::AlignHCenter | Qt::AlignVCenter,s);
        }
        //draw minor tick
        else
        {
            painter->drawLine(xp,minorTickTop,xp,minorTickBottom);
        }

        actSec+=minorTick;
    }

    painter->restore();
}

void TimelineHeader::renderDay(const QRect& rect,QPainter* painter,int logicalIndex) const
{
    //painter->fillRect(rect.adjusted(0,0,0,-1),timelineFrameBgCol_);

    //The timeline area bounded by the frame
    QRect pRect=rect.adjusted(0,timelineFrameSize_,0,-timelineFrameSize_);

    //Special appearance for the timeline area
    painter->fillRect(pRect,timelineBrush_);

    painter->setPen(QPen(timelineFrameBorderCol_));
    painter->drawLine(QPoint(rect.left(),pRect.top()),
                      QPoint(rect.right(),pRect.top()));

    painter->drawLine(QPoint(rect.left(),pRect.bottom()),
                      QPoint(rect.right(),pRect.bottom()));


    int w=rect.width();  //sectionSize(TimelineModel::TimelineColumn);

    //period in secs
    qint64 startSec=0;
    qint64 endSec=86400;
    qint64 period=endSec-startSec;

    int minorTick=1; //in secs (it is a delta)
    int majorTick=1;  //in secs (it is a delta)
    qint64 firstTick=1; //in secs since epoch

    int hLineY=pRect.center().y()-majorTickSize_/2-1;
    int majorTickTop=hLineY;
    int majorTickBottom=hLineY+majorTickSize_; //pRect.bottom()-fm_.height()-timeTextGap;
    int minorTickTop=hLineY;
    int minorTickBottom=majorTickBottom-3;
    int timeTextY= majorTickBottom + (pRect.bottom()-majorTickBottom - fm_.height())/2 - 1;

    int timeItemW=fm_.width("223:442");

    QList<int> majorTickSec;

    if(period < 600)
    {
        majorTickSec << 60 ;
    }
    if(period < 1800)
    {
        majorTickSec << 5*60;
    }
    else if(period < 7200)
    {
        majorTickSec << 10*60 << 15*60 << 30*60;
    }
    if(period < 12*3600)
    {
        majorTickSec << 15*60 << 30*60 << 60*60 << 120*60 << 180*60;
    }
    else
    {
        majorTickSec << 30*60 << 3600 << 2*3600 << 3*3600 << 4*3600;
    }

    Q_FOREACH(int mts,majorTickSec)
    {
       majorTick=mts;
       int majorTickNum=period/majorTick;
       int cover=timeItemW*majorTickNum;
       int diff=w-cover;
       if(diff > 100)
       {
           break;
       }
    }

    minorTick=majorTick/4;
    if(minorTick==0)
        minorTick = majorTick;

    firstTick=(startSec/minorTick)*minorTick;
    if(firstTick< startSec)
        firstTick+=minorTick;

    //Find label positions for days
    QList<QPair<int,QString> > dateLabels;

    painter->save();
    painter->setClipRect(rect);

    //horizontal line
    painter->setPen(timelineCol_);
    painter->drawLine(rect.x(),hLineY,rect.right(),hLineY);

    qint64 actSec=firstTick;
    Q_ASSERT(actSec >= startSec);
    painter->setPen(timeTextCol_);

    while(actSec <= endSec)
    {
        int xp=secToPos(actSec-startSec,rect,period);

        //draw major tick + label
        if(actSec % majorTick == 0)
        {
            painter->drawLine(xp,majorTickTop,xp,majorTickBottom);

            QString s;
            if(majorTick < 60)
            {
                s=QDateTime::fromMSecsSinceEpoch(actSec*1000,Qt::UTC).toString("H:mm:ss");
            }
            else
            {
                s=QDateTime::fromMSecsSinceEpoch(actSec*1000,Qt::UTC).toString("H:mm");
            }

            int textW=fm_.width(s);
            painter->setFont(font_);

            if( xp-textW/2 < rect.x())
            {
                painter->drawText(QRect(rect.x()+1, timeTextY,textW,fm_.height()),
                              Qt::AlignLeft | Qt::AlignVCenter,s);
            }
            else if( xp+textW/2+2 >= rect.x()+rect.width())
            {
                painter->drawText(QRect(rect.x()+rect.width()-textW-1, timeTextY,textW,fm_.height()),
                              Qt::AlignRight | Qt::AlignVCenter,s);
            }
            else
            {
                painter->drawText(QRect(xp-textW/2, timeTextY,textW,fm_.height()),
                              Qt::AlignHCenter | Qt::AlignVCenter,s);
            }
        }
        //draw minor tick
        else
        {
            painter->drawLine(xp,minorTickTop,xp,minorTickBottom);
        }

        actSec+=minorTick;
    }

    painter->restore();
}


int TimelineHeader::secToPos(qint64 t,QRect rect) const
{
    //qint64 sd=startDate_.toMSecsSinceEpoch()/1000;
    qint64 period=(endDate_.toMSecsSinceEpoch()-startDate_.toMSecsSinceEpoch())/1000;
    return rect.x() + static_cast<int>(static_cast<float>(t)/static_cast<float>(period)*static_cast<float>(rect.width()));
}

QDateTime TimelineHeader::posToDate(QPoint pos) const
{
    int logicalIndex=logicalIndexAt(pos);
    if(logicalIndex == -1)
        return QDateTime();

    int xp=sectionPosition(logicalIndex);
    int w=sectionSize(logicalIndex);

    if(w <= 0 || pos.x() < xp)
        return QDateTime();

    float r=static_cast<float>(pos.x()-xp)/static_cast<float>(w);
    if(r < 0 || r > 1)
        return QDateTime();

    //qint64 sd=startDate_.toMSecsSinceEpoch()/1000;
    qint64 period=(endDate_.toMSecsSinceEpoch()-startDate_.toMSecsSinceEpoch());

    return startDate_.addMSecs(r*period);
}

void TimelineHeader::setZoomActions(QAction* zoomInAction,QAction* zoomOutAction)
{
    if(!zoomInAction_)
    {
        zoomInAction_=zoomInAction;
        zoomOutAction_=zoomOutAction;

        connect(zoomInAction_,SIGNAL(toggled(bool)),
                this,SLOT(slotZoomState(bool)));

        connect(zoomOutAction_,SIGNAL(triggered(bool)),
                this,SLOT(slotZoomOut(bool)));

        checkActionState();
    }
}

bool TimelineHeader::isZoomEnabled() const
{
    return (zoomInAction_ && zoomInAction_->isEnabled())?(zoomInAction_->isChecked()):false;
}

void TimelineHeader::setZoomDisabled()
{
    if(zoomInAction_)
        zoomInAction_->setChecked(false);
}

void TimelineHeader::slotZoomState(bool)
{
    Q_ASSERT(zoomInAction_);
    if(!zoomInAction_->isChecked())
    {
        bool hasCursor=testAttribute(Qt::WA_SetCursor);
        if(hasCursor && cursor().shape() !=  Qt::SplitHCursor)
                unsetCursor();
    }

    //headerDataChanged(Qt::Horizontal,0,TimelineModel::TimelineColumn);
}

void TimelineHeader::slotZoomOut(bool)
{
    if(!inZoom_ && !isZoomEnabled())
    {
        if(zoomHistory_.count() >= 2)
        {
            zoomHistory_.pop();
            QDateTime sDt=zoomHistory_.top().first;
            QDateTime eDt=zoomHistory_.top().second;
            if(sDt.isValid() && eDt.isValid())
            {
                setPeriodCore(sDt,eDt,false);
                Q_EMIT periodSelected(sDt,eDt);
            }

            checkActionState();
        }
    }
}

void TimelineHeader::checkActionState()
{
    if(zoomInAction_)
    {
        zoomOutAction_->setEnabled(zoomHistory_.count() >= 2);
        zoomInAction_->setEnabled(canBeZoomed());
    }
}

void TimelineHeader::setStartDate(QDateTime t)
{
    startDate_=t;
    zoomHistory_.clear();
    zoomHistory_.push(qMakePair<QDateTime,QDateTime>(startDate_,endDate_));
    checkActionState();
}

void TimelineHeader::setEndDate(QDateTime t)
{
    endDate_=t;
    zoomHistory_.clear();
    zoomHistory_.push(qMakePair<QDateTime,QDateTime>(startDate_,endDate_));
    checkActionState();
}

void TimelineHeader::setPeriod(QDateTime t1,QDateTime t2)
{
    zoomHistory_.clear();
    setPeriodCore(t1,t2,true);
}

void TimelineHeader::setPeriodCore(QDateTime t1,QDateTime t2,bool addToHistory)
{
    startDate_=t1;
    endDate_=t2;
    if(addToHistory)
    {
        zoomHistory_.push(qMakePair<QDateTime,QDateTime>(startDate_,endDate_));
    }
    checkActionState();
}

bool TimelineHeader::hasZoomableColumn() const
{
    for(int i=0; i < columnType_.count(); i++)
    {
        if((columnType_[i] == TimelineColumn || columnType_[i] == DayColumn) &&
           !isSectionHidden(i))
            return true;
    }
    return false;
}

bool TimelineHeader::hasTimeColumn() const
{
    for(int i=0; i < columnType_.count(); i++)
    {
        if((columnType_[i] == TimelineColumn || columnType_[i] == DayColumn) &&
           !isSectionHidden(i))
            return true;
    }
    return false;
}

bool TimelineHeader::isColumnZoomable(QPoint pos) const
{
    int logicalIndex=logicalIndexAt(pos);
    if(logicalIndex != -1)
        return columnType_[logicalIndex] == TimelineColumn;

    return false;
}

int TimelineHeader::secToPos(qint64 t,QRect rect,qint64 period) const
{
    return rect.x() + static_cast<int>(static_cast<float>(t)/static_cast<float>(period)*static_cast<float>(rect.width()));
}

void TimelineHeader::setMaxDurations(int submittedDuration,int activeDuration)
{
    submittedMaxDuration_=submittedDuration;
    activeMaxDuration_=activeDuration;
}

void TimelineHeader::viewModeChanged()
{
#if 0
    if(!hasZoomableColumn()) //????
    {
        zoomHistory_.clear();
    }
    checkActionState();
#endif
}
