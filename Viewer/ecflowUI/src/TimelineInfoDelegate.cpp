//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TimelineInfoDelegate.hpp"

#include <QtGlobal>
#include <QApplication>
#include <QDebug>
#include <QImageReader>
#include <QPainter>

#include <QStyleOptionViewItem>

#include "IconProvider.hpp"
#include "PropertyMapper.hpp"
#include "TimelineData.hpp"
#include "TimelineInfoWidget.hpp"
#include "VNState.hpp"

static std::vector<std::string> propVec;

//Define node renderer properties
struct TimelineInfoNodeDelegateBox : public NodeDelegateBox
{
     TimelineInfoNodeDelegateBox() {
        topMargin=2;
        bottomMargin=2;
        leftMargin=3;
        rightMargin=0;
        topPadding=0;
        bottomPadding=0;
        leftPadding=2;
        rightPadding=1;
      }
};


TimelineInfoDelegate::TimelineInfoDelegate(QWidget *parent) :
  borderPen_(QColor(230,230,230))
{
    nodeBox_=new TimelineInfoNodeDelegateBox;
    attrBox_=0;

    nodeBox_->adjust(font_);

    //Property
    if(propVec.empty())
    {
        //Base settings
        addBaseSettings(propVec);
    }

    prop_=new PropertyMapper(propVec,this);

    updateSettings();
}

TimelineInfoDelegate::~TimelineInfoDelegate()
{
}

void TimelineInfoDelegate::updateSettings()
{
    //Update the settings handled by the base class
    updateBaseSettings();
}

QSize TimelineInfoDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QSize size=QStyledItemDelegate::sizeHint(option,index);
    return QSize(size.width()+4,nodeBox_->sizeHintCache.height());
}

void TimelineInfoDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const
{
    //Background
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QStyleOptionViewItem vopt(option);
#else
    QStyleOptionViewItemV4 vopt(option);
#endif

    initStyleOption(&vopt, index);

    const QStyle *style = vopt.widget ? vopt.widget->style() : QApplication::style();
    const QWidget* widget = vopt.widget;

    //Save painter state
    painter->save();

    if(index.column() == 1)
    {
         renderStatus(painter,index,vopt);
    }

    //rest of the columns
    else
    {      
        QString text=index.data(Qt::DisplayRole).toString();
        QRect textRect = vopt.rect;
        textRect.setX(textRect.x()+4);
        QFontMetrics fm(font_);
        textRect.setWidth(fm.width(text));

        painter->setPen(Qt::black);

        int rightPos=textRect.right()+1;
        const bool setClipRect = rightPos > option.rect.right();
        if(setClipRect)
        {
           painter->save();
           painter->setClipRect(option.rect);
        }

        painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

        if(setClipRect)
        {
           painter->restore();
        }

    }

    int cellMode=index.data(Qt::UserRole).toInt();

    if(cellMode == 3)
    {
        QColor fg(230,230,230,180);
        if(fg.isValid())
            painter->fillRect(vopt.rect,fg);
    }

    //Render the horizontal border for rows. We only render the top border line.
    //With this technique we miss the bottom border line of the last row!!!
    //QRect fullRect=QRect(0,option.rect.y(),painter->device()->width(),option.rect.height());
    QRect bgRect=option.rect;
    painter->setPen(borderPen_);
    painter->drawLine(bgRect.topLeft(),bgRect.topRight());

    //darker top border
    if(cellMode == 1)
    {
        painter->setPen(QColor(180,180,180));
        painter->drawLine(bgRect.topLeft(),bgRect.topRight());
    }
    //darker top border
    else if(cellMode == 2)
    {
        painter->setPen(QColor(180,180,180));
        painter->drawLine(bgRect.bottomLeft(),bgRect.bottomRight());
    }

    painter->restore();
}

//======================================================================
//
// TimelineInfoDailyDelegate
//
//======================================================================

TimelineInfoDailyDelegate::TimelineInfoDailyDelegate(TimelineInfoDailyModel *model,QWidget *parent) :
    QStyledItemDelegate(parent),
    model_(model),
    fm_(QFont()),
    borderPen_(QPen(QColor(216,216,216))),
    topPadding_(2),
    bottomPadding_(2)
{
    Q_ASSERT(model_);

    fm_=QFontMetrics(font_);

    //Property
    if(propVec.empty())
    {
        propVec.push_back("view.table.font");

        //Base settings
        //addBaseSettings(propVec);
    }

    prop_=new PropertyMapper(propVec,this);

    updateSettings();
}

TimelineInfoDailyDelegate::~TimelineInfoDailyDelegate()
{
}

void TimelineInfoDailyDelegate::notifyChange(VProperty* p)
{
    updateSettings();
}

void TimelineInfoDailyDelegate::updateSettings()
{
    if(VProperty* p=prop_->find("view.table.font"))
    {
        QFont newFont=p->value().value<QFont>();

        if(font_ != newFont)
        {
            font_=newFont;
            fm_=QFontMetrics(font_);
            //Q_EMIT sizeHintChangedGlobal();
        }
    }

    //Update the settings handled by the base class
    //updateBaseSettings();
}

QSize TimelineInfoDailyDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QSize size=QStyledItemDelegate::sizeHint(option,index);
    return QSize(size.width(),fm_.height() + topPadding_ + bottomPadding_);
    return size;
}


void TimelineInfoDailyDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const
{
    //Background
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QStyleOptionViewItem vopt(option);
#else
    QStyleOptionViewItemV4 vopt(option);
#endif

    initStyleOption(&vopt, index);

    const QStyle *style = vopt.widget ? vopt.widget->style() : QApplication::style();
    const QWidget* widget = vopt.widget;

    bool selected=option.state & QStyle::State_Selected;

    //Save painter state
    painter->save();

    if(index.column() == 1)
    {
        renderTimeline(painter,option,index);
    }

    //The path column
    else
    {
        QRect bgRect=option.rect;
        painter->fillRect(bgRect,QColor(244,244,245));
        painter->setPen(borderPen_);
        painter->drawLine(bgRect.x()+bgRect.width()-1,bgRect.top(),
                          bgRect.x()+bgRect.width()-1,bgRect.bottom());

        QString text=index.data(Qt::DisplayRole).toString();
        //QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt,widget);
        QRect textRect = bgRect.adjusted(2,topPadding_,-3,-bottomPadding_);
        text=fm_.elidedText(text,Qt::ElideMiddle,textRect.width());
        textRect.setWidth(fm_.width(text));
        painter->setFont(font_);
        painter->setPen(Qt::black);
        painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

        if(selected)
        {
            QRect sr=textRect;
            sr.setX(option.rect.x()+1);
            sr.setY(option.rect.y()+2);
            sr.setBottom(option.rect.bottom()-2);
            sr.setRight(textRect.x()+textRect.width()+1);
            painter->setPen(QPen(Qt::black,2));
            painter->drawRect(sr);
        }
    }

    //Render the horizontal border for rows. We only render the top border line.
    //With this technique we miss the bottom border line of the last row!!!
    //QRect fullRect=QRect(0,option.rect.y(),painter->device()->width(),option.rect.height());
    QRect bgRect=option.rect;
    painter->setPen(borderPen_);
    painter->drawLine(bgRect.topLeft(),bgRect.topRight());

    painter->restore();
}


void TimelineInfoDailyDelegate::renderTimeline(QPainter *painter,const QStyleOptionViewItem& option,const QModelIndex& index) const
{
    if(!index.isValid())
        return;

    TimelineItem *item=model_->data();
    if(!item)
        return;

    //bool selected=option.state & QStyle::State_Selected;

    int leftEdge=option.rect.x();
    int rightEdge=option.rect.x()+option.rect.width();
    int xpPrev=leftEdge-2;
    int xpNext=rightEdge+2;
    int extendedRight=-1;

    unsigned int day=index.data().toUInt();

    for(size_t i=0; i < item->size(); i++)
    {
        if(item->start_[i] < day)
            continue;
        if(item->start_[i] >= day + 86400)
            break;

        bool hasGrad=false;
        bool lighter=false;
        int xpLeft=0,xpRight=0;
        QColor fillCol;
        int xp=timeToPos(option.rect,item->start_[i]);

        if(i > 0 && item->start_[i-1] < day)
        {
            if(VNState* vn=VNState::find(item->status_[i-1]))
            {
                hasGrad=true;
                xpLeft=leftEdge;
                xpRight=xp;
                fillCol=vn->colour();
                lighter=(vn->name() == "complete");
                drawCell(painter,QRect(xpLeft,option.rect.y(),xpRight-xpLeft+1,option.rect.height()-1),
                         fillCol,hasGrad,lighter);
            }
        }

        if(VNState* vn=VNState::find(item->status_[i]))
        {
            fillCol=vn->colour();
            lighter=(vn->name() == "complete");
            if(i < item->size()-1)
            {
                if(item->start_[i+1] < day+86400)
                {
                    hasGrad=true;
                    xpLeft=xp;
                    xpRight=timeToPos(option.rect,item->start_[i+1]);
                }
                else
                {
                    hasGrad=false;
                    xpLeft=xp;
                    xpRight=rightEdge;
                }
            }
            else
            {
                hasGrad=false;
                xpLeft=xp;
                xpRight=rightEdge;
            }

            //small rects are extended and use no gradient
            if(xpRight-xpLeft < 5)
            {
                hasGrad=false;
                xpRight=xpLeft+4;
                if(extendedRight == xpRight)
                {
                    xpLeft+=2;
                    xpRight+=2;
                }
                else if(extendedRight > xpRight)
                {
                    xpLeft=extendedRight-2;
                    xpRight=extendedRight;
                }
                extendedRight=xpRight;
            }
            else
            {
                if(extendedRight > xpLeft)
                {
                    xpLeft=extendedRight;
                }
                extendedRight = -1;
            }

            drawCell(painter,QRect(xpLeft,option.rect.y(),xpRight-xpLeft+1,option.rect.height()-1),
                      fillCol,hasGrad,lighter);
        }
    }
}

void TimelineInfoDailyDelegate::drawCell(QPainter *painter,QRect r,QColor fillCol,bool hasGrad,bool lighter) const
{
    QColor endCol;
    if(lighter)
    {
        fillCol.light(130);
        fillCol.setAlpha(110);
    }
    else
    {
        fillCol.dark(110);
    }

    QBrush fillBrush(fillCol);
    if(hasGrad)
    {
        QLinearGradient gr;
        gr.setCoordinateMode(QGradient::ObjectBoundingMode);
        gr.setStart(0,0);
        gr.setFinalStop(1,0);
        gr.setColorAt(0,fillCol);
        fillCol.setAlpha(lighter?40:128);
        gr.setColorAt(1,fillCol);
        fillBrush=QBrush(gr);
    }

    painter->fillRect(r,fillBrush);
}

int TimelineInfoDailyDelegate::timeToPos(QRect r,unsigned int time) const
{
    unsigned int start=0;  //TimelineItem::fromQDateTime(startDate_);
    unsigned int end=86400; //TimelineItem::fromQDateTime(endDate_);

    time=time%86400;

    if(time < start)
        return r.x()-2;

    if(time >= end)
        return r.x()+r.width()+2;

    if(start >= end)
        return r.x()-2;

    return r.x()+static_cast<float>(time-start)*static_cast<float>(r.width())/static_cast<float>((end-start));

}

#if 0
void TimelineInfoDailyDelegate::setStartDate(QDateTime t)
{
    startDate_=t;
}

void TimelineInfoDailyDelegate::setEndDate(QDateTime t)
{
    endDate_=t;
}

void TimelineInfoDailyDelegate::setPeriod(QDateTime t1,QDateTime t2)
{
    startDate_=t1;
    endDate_=t2;
}
#endif
