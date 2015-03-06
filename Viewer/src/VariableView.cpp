//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VariableView.hpp"

#include <QApplication>
#include <QDebug>
#include <QPainter>

#include "VParam.hpp"

//========================================================
//
// VariableViewDelegate
//
//========================================================

VariableDelegate::VariableDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{
    selectPen_=QPen(QColor(125,162,206));
    selectBrush_=QBrush(QColor(193,220,252,110));
    borderPen_=QPen(QColor(180,180,180));
}

void VariableDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const
{
    QStyleOptionViewItemV4 vopt(option);
    initStyleOption(&vopt, index);

    const QStyle *style = vopt.widget ? vopt.widget->style() : QApplication::style();
    const QWidget* widget = vopt.widget;

    //This indicates that the item is not a parent item (node or server),
    //but a variable, hence it does not have branch controls.
    bool hasChild=!index.parent().isValid();

    //Save painter state
    painter->save();

    //The background rect
    QRect bgRect=option.rect;

    //For variables in the first column we want to extend the item
    //rect to the left for the background painting.
    if(index.column()==0 && !hasChild)
    {
    	bgRect.setX(0);
    }

    //Paint item highlight!!
    QColor highCol=index.data(Qt::UserRole).value<QColor>();
    if(highCol.isValid())
    {
    	painter->fillRect(bgRect.adjusted(1,1,-1,-1),highCol);
    }
    //otherwise paint item background!!
    else
    {
    	//Paint the item background
    	QColor bg=index.data(Qt::BackgroundRole).value<QColor>();
    	if(bg.isValid())
    	{
    		painter->fillRect(bgRect,bg);
    	}
    }

    //Paint selection. This should be transparent.
    if(!hasChild && (option.state & QStyle::State_Selected))
    {
    	//The selection rect
    	QRect selectRect=option.rect.adjusted(0,1,0,-1);

    	//For the first column we extend the selection
    	//rect to left edge.
    	if(index.column()==0)
    	{
    		selectRect.setX(0);
    	}

    	//QRect fillRect=option.rect.adjusted(0,1,-1,-textRect.height()-1);
        painter->fillRect(selectRect,selectBrush_);
        //painter->setPen(selectPen_);
        //painter->drawLine(fullRect.topLeft(),fullRect.topRight());
        //painter->drawLine(fullRect.bottomLeft(),fullRect.bottomRight());
    }

    //Render the horizontal border for rows. We only render the top border line.
    //With this technique we miss the bottom border line of the last row!!!
    QRect fullRect=QRect(0,option.rect.y(),painter->device()->width(),option.rect.height());
    painter->setPen(borderPen_);
    painter->drawLine(bgRect.topLeft(),bgRect.topRight());

    if(index.column() == 0 && !hasChild)
    {
    	painter->drawLine(bgRect.topRight(),bgRect.bottomRight());
    }

    //Display text
    QString text=index.data(Qt::DisplayRole).toString();
    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt, widget);

    //For variables in the first column we move the text to the left.
    if(index.column() == 0 && !hasChild)
    {
    	textRect.moveLeft(option.rect.x()-17);
    }

    QColor fg=index.data(Qt::ForegroundRole).value<QColor>();
    if(!fg.isValid())
    	   fg=Qt::black;

    painter->setPen(fg);
    painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

    //Restore painter state
    painter->restore();

}

QSize VariableDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	QSize size=QStyledItemDelegate::sizeHint(option,index);

	size+=QSize(0,4);

    return size;
}



//========================================================
//
// VariableView
//
//========================================================

VariableView::VariableView(QWidget* parent) : QTreeView(parent)
{
	VariableDelegate *delegate=new VariableDelegate(this);
	setItemDelegate(delegate);

	setRootIsDecorated(true);
	setAllColumnsShowFocus(true);
	setUniformRowHeights(true);
	setAlternatingRowColors(true);
    setSortingEnabled(true);

	//Context menu
	setContextMenuPolicy(Qt::ActionsContextMenu);
}

//We do not want to render the branches for the variable items. We only
//render the branch for the node items (nodes or server).
void VariableView::drawBranches(QPainter* painter,const QRect& rect,const QModelIndex& index ) const
{
	if(!index.parent().isValid())
	{
		//We need to fill the branch area here. We cannot do it in the delegate
		//because when the delegate is called the the branch control is already
		//rendered, so the delegate would just overpaint it!!!
		painter->fillRect(rect,index.data(Qt::BackgroundRole).value<QColor>());

		//Draw the branch with the default method
		QTreeView::drawBranches(painter,rect,index);
	}
}

/*
void VariableView::slotSelectItem(const QModelIndex&)
{

}

void VariableView::reload(VInfo_ptr info)
{
	//model_->setData(info);
	//expandAll();
}
*/
