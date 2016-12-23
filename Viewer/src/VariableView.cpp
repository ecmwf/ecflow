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
#include <QItemSelectionModel>
#include <QPainter>
#include <QHeaderView>

#include "IconProvider.hpp"
#include "VariableModel.hpp"
#include "VParam.hpp"

//========================================================
//
// VariableViewDelegate
//
//========================================================

VariableDelegate::VariableDelegate(QTreeView *parent) : QStyledItemDelegate(parent), view_(parent)
{
    selectPen_=QPen(QColor(8,117,182));
    selectBrush_=QBrush(QColor(65,139,212));
    selectBrushBlock_=QBrush(QColor(48,102,178));
    borderPen_=QPen(QColor(230,230,230));
    genVarPixId_=IconProvider::add(":/viewer/genvar.svg","genvar");
    shadowGenVarPixId_=IconProvider::add(":/viewer/genvar_shadow.svg","genvar_shadow");
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

    bool selected=option.state & QStyle::State_Selected;

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
    else if(!selected)
    {
    	//Paint the item background
    	QColor bg=index.data(Qt::BackgroundRole).value<QColor>();
    	if(bg.isValid())
    	{
            painter->fillRect(bgRect,bg);
    	}
        //alternating row colour?
        else {}
    }

    //Paint selection. This should be transparent.
    if(selected)
    {
    	//The selection rect
        QRect selectRect;
        if(hasChild)
        {
            selectRect=bgRect.adjusted(0,1,0,-1);
            painter->fillRect(selectRect,selectBrushBlock_);
        }
        else
        {
            selectRect=bgRect.adjusted(0,1,0,-1);

            //For the first column we extend the selection
            //rect to left edge.
            if(index.column()==0)
            {
                selectRect.setX(0);
            }
            painter->fillRect(selectRect,selectBrush_);

        }
    }

    //Render the horizontal border for rows. We only render the top border line.
    //With this technique we miss the bottom border line of the last row!!!
    painter->setPen(borderPen_);
    painter->drawLine(bgRect.topLeft(),bgRect.topRight());

    if(index.column() == 0 && !hasChild)
    {
    	painter->drawLine(bgRect.topRight(),bgRect.bottomRight());
    }

    //Display text
    QString text=index.data(Qt::DisplayRole).toString();
    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt, widget);   
    QFont f;
    QFontMetrics fm(f);
    textRect.setWidth(fm.width(text));

    if(index.column() == 0 && !hasChild)
    {
        bool hasLock=false;
        QPixmap lockPix;
        QRect lockRect;
        bool hasGen=false;
        QPixmap genPix;
        QRect genRect;

        textRect.setLeft(option.rect.x()-17);
    	bool locked=index.data(VariableModel::ReadOnlyRole).toBool();
    	if(locked)
    	{
            hasLock=true;
            lockPix=IconProvider::lockPixmap(textRect.height()-6);

            lockRect=QRect(textRect.left()-4-lockPix.width(),
    				       textRect.top()+(textRect.height()-lockPix.height())/2,
    			           lockPix.width(),lockPix.height());
    	}

        bool gen=index.data(VariableModel::GenVarRole).toBool();    
        if(gen && genVarPixId_ >= 0)
        {
            int pixId=genVarPixId_;
            if(index.data(VariableModel::ShadowRole).toBool())
            {
                pixId=shadowGenVarPixId_;
            }
            if(pixId >=0)
            {
                hasGen=true;

                genPix=IconProvider::pixmap(pixId,textRect.height()-4);
                genRect=QRect(textRect.left(),
                          textRect.top()+(textRect.height()-genPix.height())/2,
                          genPix.width(), genPix.height());

                textRect.moveLeft(genRect.right()+4);
            }
        }

        if(textRect.right()+1 > option.rect.right())
        {
            painter->setClipRect(option.rect.adjusted(-option.rect.left(),0,0,0));
        }
        QColor fg;
        if(option.state & QStyle::State_Selected)
        {
            fg=Qt::white;
        }
        else
        {
            fg=index.data(Qt::ForegroundRole).value<QColor>();
            if(!fg.isValid())
                fg=Qt::black;
        }

        painter->setPen(fg);

        painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

        if(hasLock)
            painter->drawPixmap(lockRect,lockPix);

        if(hasGen)
           painter->drawPixmap(genRect,genPix);

    }
    else if(index.column() == 0)
    {
        QColor fg=index.data(Qt::ForegroundRole).value<QColor>();
        if(!fg.isValid())
            fg=Qt::black;
        painter->setPen(fg);

        QRegExp rx("^(.+)\\s(\\S+)$");
        //QRegExp rx("inherited from (\\S+) (\\S+)");
        if(rx.indexIn(text) > -1 && rx.captureCount() == 2)
        {
            QFont f;
            f.setPointSize(f.pointSize()-1);
            QFontMetrics fm(f);
            QString txt1=rx.cap(1);
            textRect.setWidth(fm.width(txt1));
            painter->setFont(f);
            painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,txt1);
            textRect.setLeft(textRect.right()+fm.width("D"));
            text=rx.cap(2);
        }

        QFont fBold;
        fBold.setPointSize(fBold.pointSize()-1);
        fBold.setBold(true);
        QFontMetrics fmBold(fBold);
        textRect.setWidth(fmBold.width(text + "a"));
        painter->setFont(fBold);

        painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

    }
    else if(index.column() == 1)
    {
        textRect.adjust(2,0,2,0);

        QColor fg;
        if(selected)
        {
            fg=Qt::white;
        }
        else
        {
            fg=index.data(Qt::ForegroundRole).value<QColor>();
            if(!fg.isValid())
              fg=Qt::black;
        }

        painter->setPen(fg);
        painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
    }    

    //Restore painter state
    painter->restore();
}

QSize VariableDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	QSize size=QStyledItemDelegate::sizeHint(option,index);

	size+=QSize(0,2);

    //We need to starcth the second column to the right edge of the viewport
    if(index.column() == 1)
    {
        int w1=view_->header()->sectionSize(0);
        int w=view_->viewport()->size().width();
        if(w1+size.width() < w)
            size.setWidth(w-w1+1);
    }
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

    delegate_=new VariableDelegate(this);
    setItemDelegate(delegate_);

	setRootIsDecorated(true);
	setAllColumnsShowFocus(true);
	setUniformRowHeights(true);
    //setAlternatingRowColors(true);
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

        if(selectionModel()->rowIntersectsSelection(index.row(),QModelIndex()))
        {
            painter->fillRect(rect.adjusted(0,1,0,-1),delegate_->selectBrushBlock_);
        }
        else
        {
            painter->fillRect(rect.adjusted(0,1,0,0),index.data(Qt::BackgroundRole).value<QColor>());
        }

		//Draw the branch with the default method
		QTreeView::drawBranches(painter,rect,index);
	}
}
