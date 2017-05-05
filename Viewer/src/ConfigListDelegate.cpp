//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ConfigListDelegate.hpp"

#include <QApplication>
#include <QDebug>
#include <QPainter>

//========================================================
//
// VariableViewDelegate
//
//========================================================

ConfigListDelegate::ConfigListDelegate(int iconSize,int maxWidth,QWidget *parent) :
	QStyledItemDelegate(parent),
	iconSize_(iconSize),
	maxTextWidth_(maxWidth),
	margin_(2),
	gap_(5)
{
}

void ConfigListDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const
{
    QStyleOptionViewItem vopt(option);
    initStyleOption(&vopt, index);   

    QPixmap pix=index.data(Qt::DecorationRole).value<QPixmap>();

    //Save painter state
    painter->save();

    //The background rect
    QRect bgRect=option.rect.adjusted(0,1,0,-1);

    //Paint selection. This should be transparent.
    if(option.state & QStyle::State_Selected)
    {
        painter->fillRect(bgRect,QColor(200,222,250));
    }

    //pixmap
    QRect pixRect(bgRect.center().x()-iconSize_/2,bgRect.top()+margin_,iconSize_,iconSize_);
    painter->drawPixmap(pixRect,pix);

    //text
    QString text=index.data(Qt::DisplayRole).toString();
    QFont f;
    f.setBold(true);
    QFontMetrics fm(f);
    int textW=fm.width(text);
    QRect textRect(bgRect.center().x()-textW/2,pixRect.bottom()+gap_,textW,fm.height());

    painter->drawText(textRect,Qt::AlignHCenter| Qt::AlignVCenter,text);

	//Restore painter state
    painter->restore();


}

QSize ConfigListDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	QSize size=QStyledItemDelegate::sizeHint(option,index);

	int w=(maxTextWidth_ > iconSize_)?maxTextWidth_:iconSize_;
	w+=4;

	QFont f;
	QFontMetrics fm(f);
	size=QSize(w,2*margin_+iconSize_+gap_+fm.height()+2);

    return size;
}


