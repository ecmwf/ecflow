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

    QColor bg=index.data(Qt::UserRole).value<QColor>();
    if(bg.isValid())
        painter->fillRect(vopt.rect,bg);

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

    //Render the horizontal border for rows. We only render the top border line.
    //With this technique we miss the bottom border line of the last row!!!
    //QRect fullRect=QRect(0,option.rect.y(),painter->device()->width(),option.rect.height());
    QRect bgRect=option.rect;
    painter->setPen(borderPen_);
    painter->drawLine(bgRect.topLeft(),bgRect.topRight());

    painter->restore();
}
