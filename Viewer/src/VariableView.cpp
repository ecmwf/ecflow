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
#include <QImageReader>
#include <QPainter>

#include "IconProvider.hpp"
#include "VariableModel.hpp"
#include "VParam.hpp"

//========================================================
//
// VariableViewDelegate
//
//========================================================

VariableDelegate::VariableDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{
	selectPen_=QPen(QColor(8,117,182));
    selectBrush_=QBrush(QColor(200,222,250));
    borderPen_=QPen(QColor(230,230,230));

    QImageReader imgR(":/viewer/padlock.svg");
    if(imgR.canRead())
    {
    	QFont font;
    	QFontMetrics fm(font);
    	int size=fm.height()+2;
    	imgR.setScaledSize(QSize(size,size));
    	QImage img=imgR.read();
    	lockPix_=QPixmap(QPixmap::fromImage(img));
    }
}

void VariableDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const
{
    QStyleOptionViewItemV4 vopt(option);
    initStyleOption(&vopt, index);

    const QStyle *style = vopt.widget ? vopt.widget->style() : QApplication::style();
    const QWidget* widget = vopt.widget;

    //This indicates that the item is a parent item (node or server),
    // hence it has branch controls.
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

    //Paint item highlight!! It is taken from the UserRole
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
    	textRect.setLeft(option.rect.x()-17);

    	bool locked=index.data(VariableModel::ReadOnlyRole).toBool();
    	if(locked)
    	{
    		QPixmap lockPix=IconProvider::lockPixmap(textRect.height()-6);

    		QRect lockRect(textRect.left()-4-lockPix.width(),
    				       textRect.top()+(textRect.height()-lockPix.height())/2,
    			           lockPix.width(),lockPix.height());

    		painter->drawPixmap(lockRect,lockPix);
    	}
    }
    
    if(index.column() == 1)
    {
        textRect.adjust(2,0,2,0);
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

	size+=QSize(0,2);

    return size;
}



//========================================================
//
// VariableView
//
//========================================================

VariableView::VariableView(QWidget* parent) : TreeView(parent)
{
	setProperty("var","1");

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
//render the branch for the group items (nodes or server).
void VariableView::drawBranches(QPainter* painter,const QRect& rect,const QModelIndex& index ) const
{   
    if(!index.parent().isValid() && index.column()==0)
	{ 
		//We need to fill the branch area here. We cannot do it in the delegate
		//because when the delegate is called the branch control is already
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
