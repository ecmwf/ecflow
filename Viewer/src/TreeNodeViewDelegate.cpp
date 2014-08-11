//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TreeNodeViewDelegate.hpp"

#include <QApplication>
#include <QDebug>
#include <QPainter>

#include "AbstractNodeModel.hpp"
#include "VParam.hpp"

TreeNodeViewDelegate::TreeNodeViewDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{
	hoverPen_=QPen(QColor(201,201,201));
	hoverBrush_=QBrush(QColor(250,250,250,210));
	selectPen_=QPen(QColor(125,162,206));
	selectBrush_=QBrush(QColor(193,220,252,210));

	/*QString group("itemview_main");
	editPen_=QPen(MvQTheme::colour(group,"edit_pen"));
	editBrush_=MvQTheme::brush(group,"edit_brush");
	hoverPen_=QPen(MvQTheme::colour(group,"hover_pen"));
	hoverBrush_=MvQTheme::brush(group,"hover_brush");
	selectPen_=QPen(MvQTheme::colour(group,"select_pen"));
	selectBrush_=MvQTheme::brush(group,"select_brush");*/

}

void TreeNodeViewDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
		           const QModelIndex& index) const
{
	//Background
	QStyleOptionViewItemV4 vopt(option);
	initStyleOption(&vopt, index);

	const QStyle *style = vopt.widget ? vopt.widget->style() : QApplication::style();
	const QWidget* widget = vopt.widget;

	//Save painter state
	painter->save();

	//Selection
	if(index.column() == 0)
	{
		QRect fullRect=QRect(0,option.rect.y(),painter->device()->width(),option.rect.height());

		if(option.state & QStyle::State_Selected)
		{
			//QRect fillRect=option.rect.adjusted(0,1,-1,-textRect.height()-1);
			painter->fillRect(fullRect,selectBrush_);
			painter->setPen(selectPen_);
			painter->drawLine(fullRect.topLeft(),fullRect.topRight());
			painter->drawLine(fullRect.bottomLeft(),fullRect.bottomRight());
		}
		else if(option.state & QStyle::State_MouseOver)
		{
			//QRect fillRect=option.rect.adjusted(0,1,-1,-1);
			painter->fillRect(fullRect,hoverBrush_);
			painter->setPen(hoverPen_);
			painter->drawLine(fullRect.topLeft(),fullRect.topRight());
			painter->drawLine(fullRect.bottomLeft(),fullRect.bottomRight());
		}

	}

	if(index.column() == 0)
	{
		QVariant tVar=index.data(Qt::DisplayRole);
		QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt, widget);

		if(tVar.type() == QVariant::String)
		{
			QString text=index.data(Qt::DisplayRole).toString();
			renderNode(painter,index,text,textRect,option.rect);
		}
		else if(tVar.type() == QVariant::StringList)
		{
			QStringList lst=tVar.toStringList();
			if(lst.count() > 0)
			{
				switch(lst.at(0).toInt())
				{
				case VParam::MeterAttribute:
					renderMeter(painter,lst,textRect,option.rect);
					break;
				case VParam::LabelAttribute:
					renderLabel(painter,lst,textRect,option.rect);
					break;
				case VParam::EventAttribute:
					renderEvent(painter,lst,textRect,option.rect);
					break;
				case VParam::VarAttribute:
					renderVar(painter,lst,textRect,option.rect);
					break;
				case VParam::GenVarAttribute:
					renderGenVar(painter,lst,textRect,option.rect);
					break;
				case VParam::LimitAttribute:
					renderLimit(painter,lst,textRect,option.rect);
					break;
				case VParam::LimiterAttribute:
					renderLimiter(painter,lst,textRect,option.rect);
					break;
				case VParam::TriggerAttribute:
					renderTrigger(painter,lst,textRect,option.rect);
					break;
				case VParam::TimeAttribute:
					renderTime(painter,lst,textRect,option.rect);
					break;
				case VParam::DateAttribute:
					renderDate(painter,lst,textRect,option.rect);
					break;
				case VParam::RepeatAttribute:
					renderRepeat(painter,lst,textRect,option.rect);
					break;
				case VParam::LateAttribute:
					renderLate(painter,lst,textRect,option.rect);
					break;
				default:
					break;
				}
			}

		}
	}
	else if(index.column() < 3)
	{
		QString text=index.data(Qt::DisplayRole).toString();
		QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt, widget);
		painter->setPen(Qt::black);
		painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

	}

	painter->restore();

	//else
	//	QStyledItemDelegate::paint(painter,option,index);
}

QSize TreeNodeViewDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	QSize size=QStyledItemDelegate::sizeHint(option,index);
	return size+QSize(0,4);
}

void TreeNodeViewDelegate::renderNode(QPainter *painter,const QModelIndex& index,
		                                   QString text,QRect textRect,QRect optRect) const
{
	QColor bg=index.data(Qt::BackgroundRole).value<QColor>();

	QFont font;
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	int offset=2;

	//QRect fillRect=QRect(option.rect.x(),option.rect.y(),painter->device()->width(),option.rect.height());

	textRect.setWidth(textWidth);

	QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	textRect.moveLeft(textRect.x()+offset);

	if(fillRect.left() < optRect.right())
	{
		if(fillRect.right()>=optRect.right())
			fillRect.setRight(optRect.right());

		painter->fillRect(fillRect,bg);
		painter->setPen(QColor(180,180,180));
		painter->drawRect(fillRect);

		if(textRect.left() < optRect.right())
		{
			if(textRect.right()>=optRect.right())
				textRect.setRight(optRect.right());

			if(bg == QColor(Qt::red))
				painter->setPen(Qt::white);
			else
				painter->setPen(Qt::black);

			painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

			QVariant va=index.data(AbstractNodeModel::IconRole);
			if(va.type() == QVariant::List)
			{
				QVariantList lst=va.toList();
				int xp=fillRect.topRight().x()+5;
				int yp=fillRect.topRight().y();
				for(int i=0; i < lst.count(); i++)
				{
					QPixmap pix=lst[i].value<QPixmap>();
					QRect pixRect(xp,yp,pix.width(),pix.height());
					painter->drawPixmap(pixRect,pix);
					xp+=pix.width();
				}
			}
		}
	}
}

void TreeNodeViewDelegate::renderMeter(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const
{
	if(data.count() != 6)
			return;

	QString text=data.at(1);
	int	val=data.at(2).toInt();
	int	min=data.at(3).toInt();
	int	max=data.at(4).toInt();
	bool colChange=data.at(5).toInt();
	text+=": " + data.at(2);

	QFont font;
	font.setBold(true);
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	int offset=2;
	QSize stSize(50,10);
	int gap=5;

	QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	fillRect.setWidth(offset+stSize.width()+gap+textWidth+offset);

	QRect stRect(fillRect.x()+offset,fillRect.y()+(fillRect.height()-stSize.height())/2,
				     stSize.width(),stSize.height());

	textRect.setWidth(textWidth);
	//QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	textRect.moveLeft(stRect.right()+gap);

	if(fillRect.left() < optRect.right())
	{
		if(fillRect.right()>=optRect.right())
					fillRect.setRight(optRect.right());

		painter->fillRect(stRect,QColor(229,229,229));
		painter->setPen(QColor(180,180,180));
		painter->drawRect(stRect);

		if(textRect.left() < optRect.right())
		{
			if(textRect.right()>=optRect.right())
				textRect.setRight(optRect.right());

			painter->setPen(Qt::black);
			painter->setFont(font);

			painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
		}
	}
}

void TreeNodeViewDelegate::renderLabel(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const
{
	QString text;

	if(data.count() >1)
		text+=data.at(1) + ":";
	if(data.count() > 2)
		text+=" " + data.at(2);

	QFont font;
	font.setBold(true);
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	int offset=2;

	textRect.setWidth(textWidth);
	QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	textRect.moveLeft(textRect.x()+offset);

	if(fillRect.left() < optRect.right())
	{
		if(fillRect.right()>=optRect.right())
					fillRect.setRight(optRect.right());

		painter->fillRect(fillRect,QColor(229,229,229));
		painter->setPen(QColor(180,180,180));
		painter->drawRect(fillRect);

		if(textRect.left() < optRect.right())
		{
			if(textRect.right()>=optRect.right())
				textRect.setRight(optRect.right());

			painter->setPen(Qt::black);
			painter->setFont(font);

			painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
		}
	}
}

void TreeNodeViewDelegate::renderEvent(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const
{
	QString text;
	bool val=false;

	if(data.count() >1)
		text=data.at(1);
	if(data.count() > 2)
		val=(data.at(2) == "1");

	QFont font;
	font.setBold(true);
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	int offset=2;
	int stSize=10;
	int gap=5;

	QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	fillRect.setWidth(offset+stSize+gap+textWidth+offset);

	QRect stRect(fillRect.x()+offset,fillRect.y()+(fillRect.height()-stSize)/2,
				     stSize,stSize);

	textRect.setWidth(textWidth);
	//QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	textRect.moveLeft(stRect.right()+gap);

	if(fillRect.left() < optRect.right())
	{
		if(fillRect.right()>=optRect.right())
					fillRect.setRight(optRect.right());

		painter->fillRect(stRect,QColor(229,229,229));
		painter->setPen(QColor(180,180,180));
		painter->drawRect(stRect);

		if(textRect.left() < optRect.right())
		{
			if(textRect.right()>=optRect.right())
				textRect.setRight(optRect.right());

			painter->setPen(Qt::black);
			//painter->setFont(font);

			painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
		}
	}
}




void TreeNodeViewDelegate::renderVar(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const
{
	QString text;

	if(data.count() >1)
		text+=data.at(1) + "=";
	if(data.count() > 2)
		text+=data.at(2);

	QFont font;
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	int offset=2;

	textRect.setWidth(textWidth);
	QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	textRect.moveLeft(textRect.x()+offset);

	if(fillRect.left() < optRect.right())
	{
		if(textRect.left() < optRect.right())
		{
			if(textRect.right()>=optRect.right())
				textRect.setRight(optRect.right());

			painter->setPen(Qt::black);

			painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
		}
	}
}

void TreeNodeViewDelegate::renderGenVar(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const
{
	QString text;

	if(data.count() >1)
		text+=data.at(1) + "=";
	if(data.count() > 2)
		text+=data.at(2);

	QFont font;
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	int offset=2;

	textRect.setWidth(textWidth);
	QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	textRect.moveLeft(textRect.x()+offset);

	if(fillRect.left() < optRect.right())
	{
		if(textRect.left() < optRect.right())
		{
			if(textRect.right()>=optRect.right())
				textRect.setRight(optRect.right());

			painter->setPen(Qt::blue);

			painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
		}
	}
}

void TreeNodeViewDelegate::renderLimit(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const
{
	if(data.count() != 4)
			return;

	QString text=data.at(1);
	int	val=data.at(2).toInt();
	int	limit=data.at(3).toInt();
	text+=": " + data.at(2) + "/" + data.at(3);

	QFont font;
	font.setBold(true);
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	int offset=2;
	QSize stSize(10,10);
	int gap=5;

	QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	fillRect.setWidth(offset+textWidth+gap+stSize.width()*limit+offset);

	textRect.setWidth(textWidth);
	textRect.moveLeft(fillRect.left()+offset);

	QRect stRect(textRect.right()+gap,fillRect.y()+(fillRect.height()-stSize.height())/2,
				     stSize.width(),stSize.height());


	if(fillRect.left() < optRect.right())
	{
		if(fillRect.right()>=optRect.right())
					fillRect.setRight(optRect.right());

		//painter->fillRect(stRect,QColor(229,229,229));
		//painter->setPen(QColor(180,180,180));
		//painter->drawRect(stRect);

		if(textRect.left() < optRect.right())
		{
			if(textRect.right()>=optRect.right())
				textRect.setRight(optRect.right());

			painter->setPen(Qt::black);
			painter->setFont(font);

			painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

			for(int i=0; i < limit; i++)
				painter->drawRect(stRect.adjusted(i*stRect.width(),0,0,0));

		}
	}
}

void TreeNodeViewDelegate::renderLimiter(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const
{
	if(data.count() !=2)
			return;

	QString	text="inlimit " + data.at(1);

	QFont font;
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	int offset=2;

	textRect.setWidth(textWidth);
	QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	textRect.moveLeft(textRect.x()+offset);

	if(fillRect.left() < optRect.right())
	{
		if(textRect.left() < optRect.right())
		{
			if(textRect.right()>=optRect.right())
				textRect.setRight(optRect.right());

			painter->setPen(Qt::black);

			painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
		}
	}
}

void TreeNodeViewDelegate::renderTrigger(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const
{
	if(data.count() !=3)
			return;

	QString	text=data.at(2);

	QFont font;
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	int offset=2;

	textRect.setWidth(textWidth);
	QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	textRect.moveLeft(textRect.x()+offset);

	if(fillRect.left() < optRect.right())
	{
		if(textRect.left() < optRect.right())
		{
			if(textRect.right()>=optRect.right())
				textRect.setRight(optRect.right());

			painter->setPen(Qt::black);

			painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
		}
	}
}

void TreeNodeViewDelegate::renderTime(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const
{
	if(data.count() !=2)
			return;

	QString	text=data.at(1);

	QFont font;
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	int offset=2;

	textRect.setWidth(textWidth);
	QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	textRect.moveLeft(textRect.x()+offset);

	if(fillRect.left() < optRect.right())
	{
		if(textRect.left() < optRect.right())
		{
			if(textRect.right()>=optRect.right())
				textRect.setRight(optRect.right());

			painter->setPen(Qt::black);

			painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
		}
	}
}

void TreeNodeViewDelegate::renderDate(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const
{
	if(data.count() !=2)
			return;

	QString	text=data.at(1);

	QFont font;
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	int offset=2;

	textRect.setWidth(textWidth);
	QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	textRect.moveLeft(textRect.x()+offset);

	if(fillRect.left() < optRect.right())
	{
		if(textRect.left() < optRect.right())
		{
			if(textRect.right()>=optRect.right())
				textRect.setRight(optRect.right());

			painter->setPen(Qt::black);

			painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
		}
	}
}

void TreeNodeViewDelegate::renderRepeat(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const
{
	if(data.count() !=2)
			return;

	QString	text=data.at(1);

	QFont font;
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	int offset=2;

	textRect.setWidth(textWidth);
	QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	textRect.moveLeft(textRect.x()+offset);

	if(fillRect.left() < optRect.right())
	{
		if(textRect.left() < optRect.right())
		{
			if(textRect.right()>=optRect.right())
				textRect.setRight(optRect.right());

			painter->setPen(Qt::black);

			painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
		}
	}
}

void TreeNodeViewDelegate::renderLate(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const
{
	if(data.count() !=2)
			return;

	QString	text=data.at(1);

	QFont font;
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	int offset=2;

	textRect.setWidth(textWidth);
	QRect fillRect=textRect.adjusted(-offset,1,2*offset,-2);
	textRect.moveLeft(textRect.x()+offset);

	if(fillRect.left() < optRect.right())
	{
		if(textRect.left() < optRect.right())
		{
			if(textRect.right()>=optRect.right())
				textRect.setRight(optRect.right());

			painter->setPen(Qt::black);

			painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
		}
	}
}
