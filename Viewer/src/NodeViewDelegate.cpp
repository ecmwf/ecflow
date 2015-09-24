//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "NodeViewDelegate.hpp"

#include <QApplication>
#include <QDebug>
#include <QImageReader>
#include <QPainter>

#include "AbstractNodeModel.hpp"
#include "PropertyMapper.hpp"

static std::vector<std::string> propVec;

NodeViewDelegate::NodeViewDelegate(QWidget *parent) :
    QStyledItemDelegate(parent),
    prop_(0),
	iconSize_(16)
{
    /*//Property
	if(propVec.empty())
	{
		propVec.push_back("view.tree.font");
		propVec.push_back("view.tree.displayChildCount");
	}

	prop_=new PropertyMapper(propVec,this);

    updateSettings();*/

	hoverPen_=QPen(QColor(201,201,201));
	hoverBrush_=QBrush(QColor(250,250,250,210));
	selectPen_=QPen(QColor(125,162,206));
	selectBrush_=QBrush(QColor(193,220,252,110));
	nodePen_=QPen(QColor(180,180,180));
	nodeSelectPen_=QPen(QColor(0,0,0),2);

	lostConnectBgBrush_=QBrush(QColor(150,150,150,150),Qt::Dense7Pattern);
	lostConnectBandBrush_=QBrush(QColor(255,166,0,150));

	QImageReader imgR(":/viewer/warning.svg");
	if(imgR.canRead())
	{
		QFont font;
		QFontMetrics fm(font);
		int size=fm.height()+2;
		imgR.setScaledSize(QSize(size,size));
		QImage img=imgR.read();
		errPix_=QPixmap(QPixmap::fromImage(img));
	}

	attrRenderers_["meter"]=&NodeViewDelegate::renderMeter;
	attrRenderers_["label"]=&NodeViewDelegate::renderLabel;
	attrRenderers_["event"]=&NodeViewDelegate::renderEvent;
	attrRenderers_["var"]=&NodeViewDelegate::renderVar;
	attrRenderers_["genvar"]=&NodeViewDelegate::renderGenvar;
	attrRenderers_["limit"]=&NodeViewDelegate::renderLimit;
	attrRenderers_["limiter"]=&NodeViewDelegate::renderLimiter;
	attrRenderers_["trigger"]=&NodeViewDelegate::renderTrigger;
	attrRenderers_["time"]=&NodeViewDelegate::renderTime;
	attrRenderers_["date"]=&NodeViewDelegate::renderDate;
	attrRenderers_["repeat"]=&NodeViewDelegate::renderRepeat;
	attrRenderers_["late"]=&NodeViewDelegate::renderLate;
}

NodeViewDelegate::~NodeViewDelegate()
{
    if(prop_)
        delete prop_;
}

void NodeViewDelegate::notifyChange(VProperty* p)
{
	updateSettings();
}


QSize NodeViewDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	QSize size=QStyledItemDelegate::sizeHint(option,index);

	QFontMetrics fm(font_);
	int h=fm.height();
	return QSize(size.width(),h+8);
}

void NodeViewDelegate::adjustIconSize()
{
	QFontMetrics fm(font_);
	int h=fm.height();
	iconSize_=h;
	if(iconSize_ % 2 == 1)
		iconSize_+1;

	/*if(h > 14 && h < 19 )
		iconSize_=16;
	else if( h < 22)
		iconSize_=20;
	else if( h < 29)
		iconSize_=24;
	else if (h < 38)
		iconSize=32;
	else if ()*/
}



//========================================================
// data is encoded as a QStringList as follows:
// "meter" name  value min  max colChange
//========================================================

void NodeViewDelegate::renderMeter(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	if(data.count() != 6)
			return;

	//The data
	int	val=data.at(2).toInt();
	int	min=data.at(3).toInt();
	int	max=data.at(4).toInt();
	//bool colChange=data.at(5).toInt();
    float percent=static_cast<float>(val-min)/static_cast<float>(max-min);
    QString name=data.at(1) + ":";
	QString valStr=data.at(2) + " (" +
            QString::number(100.*percent) + "%)";

	int offset=2;
	//int gap=5;

	//The border rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-1);
	if(option.state & QStyle::State_Selected)
			fillRect.adjust(0,1,0,-1);

	//The status rectangle
	QRect stRect=fillRect.adjusted(offset,fillRect.height()/6,0,-fillRect.height()/6);
	stRect.setWidth(50);

	//The text rectangle
	QFont nameFont=attrFont_;
	nameFont.setBold(true);
	QFontMetrics fm(nameFont);
	int nameWidth=fm.width(name);
	QRect nameRect = stRect;
	nameRect.setLeft(stRect.right()+fm.width('A'));
	nameRect.setWidth(nameWidth);

	//The value rectangle
	QFont valFont=attrFont_;
	fm=QFontMetrics(valFont);
	int valWidth=fm.width(valStr);
	QRect valRect = nameRect;
	valRect.setLeft(nameRect.right()+fm.width('A'));
	valRect.setWidth(valWidth);

	//Adjust the filled rect width
	fillRect.setRight(valRect.right()+offset);

	//Define clipping
	int rightPos=fillRect.right()+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//Draw st rect
	painter->fillRect(stRect,QColor(229,229,229));
	painter->setPen(QColor(180,180,180));
	painter->drawRect(stRect);

    //Draw progress
    QRect progRect=stRect;
    progRect.setWidth(stRect.width()*percent);
    painter->fillRect(progRect,Qt::blue);

	//Draw name
	painter->setPen(Qt::black);
	painter->setFont(nameFont);
	painter->drawText(nameRect,Qt::AlignLeft | Qt::AlignVCenter,name);

	//Draw value
	painter->setPen(Qt::black);
	painter->setFont(valFont);
	painter->drawText(valRect,Qt::AlignLeft | Qt::AlignVCenter,valStr);

	if(setClipRect)
	{
		painter->restore();
	}
}

//========================================================
// data is encoded as a QStringList as follows:
// "label" name  value
//========================================================

void NodeViewDelegate::renderLabel(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	if(data.count() < 2)
			return;

	QString name=data.at(1) + ":";
	QString val;
	if(data.count() > 2)
		val=data.at(2);

	int offset=2;

	//The border rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-1);
	if(option.state & QStyle::State_Selected)
			fillRect.adjust(0,1,0,-1);


	int multiCnt=val.count('\n');

	QRect nameRect;
	QRect valRect;
	QRect multiValRect;

	QFont nameFont=attrFont_;
	nameFont.setBold(true);
	QFont valFont=attrFont_;
	QString multiVal;

	if(multiCnt ==0 )
	{
		//The text rectangle
		QFontMetrics fm(nameFont);
		int nameWidth=fm.width(name);
		nameRect = fillRect.adjusted(offset,0,0,0);
		nameRect.setWidth(nameWidth);

		//The value rectangle
		fm=QFontMetrics(valFont);
		int valWidth=fm.width(val);
		valRect = nameRect;
		valRect.setLeft(nameRect.right()+fm.width('A'));
		valRect.setWidth(valWidth);

		//Adjust the filled rect width
		fillRect.setRight(valRect.right()+offset);
	}
	else
	{
		QStringList vals=val.split('\n');
		val=vals[0];
		vals.removeFirst();
		multiVal=vals.join('\n');

		//The text rectangle
		QFontMetrics fm(nameFont);
		int nameWidth=fm.width(name);
		nameRect = fillRect.adjusted(offset,0,0,0);
		nameRect.setWidth(nameWidth);
		nameRect.setHeight(fm.height()+offset);

		//The value rectangle
		fm=QFontMetrics(valFont);
		int valWidth=fm.width(val);
		valRect = nameRect;
		valRect.setLeft(nameRect.right()+fm.width('A'));
		valRect.setWidth(valWidth);
		nameRect.setHeight(fm.height()+offset);

		//Multiple line
		multiValRect = QRect(nameRect.x(),
				nameRect.bottom()+offset/2,
				fm.width(multiVal),
				fillRect.bottom()-(nameRect.bottom()+offset));
	}

	//Define clipping
	int rightPos=fillRect.right()+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//Draw name
	painter->setPen(Qt::black);
	painter->setFont(nameFont);
	painter->drawText(nameRect,Qt::AlignLeft | Qt::AlignVCenter,name);

	//Draw value
	painter->setPen(Qt::black);
	painter->setFont(valFont);
	painter->drawText(valRect,Qt::AlignLeft | Qt::AlignVCenter,val);

	if(multiCnt > 0)
	{
		painter->setFont(valFont);
		painter->drawText(multiValRect,Qt::AlignLeft | Qt::AlignTop,multiVal);
	}

	if(setClipRect)
	{
		painter->restore();
	}
}


//========================================================
// data is encoded as a QStringList as follows:
// "event" name  value
//========================================================

void NodeViewDelegate::renderEvent(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	if(data.count() < 2)
		return;

	QString name=data.at(1);
	bool val=false;
	if(data.count() > 2) val=(data.at(2) == "1");
	QColor cCol=(val)?(Qt::blue):(Qt::gray);

	int offset=2;

	//The border rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-1);
	if(option.state & QStyle::State_Selected)
			fillRect.adjust(0,1,0,-1);

	//The control rect
	int ch=(fillRect.height()-4 < 10)?(fillRect.height()-4):8;
	QRect cRect=fillRect.adjusted(offset,(fillRect.height()-ch)/2,
			                       0,-(fillRect.height()-ch)/2);
	cRect.setWidth(cRect.height());

	//The text rectangle
	QFont font=attrFont_;
	QFontMetrics fm(font);
	int nameWidth=fm.width(name);
	QRect nameRect = fillRect.adjusted(offset,0,0,0);
	nameRect.setLeft(cRect.right()+2*offset);
	nameRect.setWidth(nameWidth);

	//Adjust the filled rect width
	fillRect.setRight(nameRect.right()+offset);

	//Define clipping
	int rightPos=fillRect.right()+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//Draw control
	painter->setPen(Qt::black);
	painter->setBrush(cCol);
	painter->drawRect(cRect);

	//Draw name
	painter->setPen(Qt::black);
	painter->drawText(nameRect,Qt::AlignLeft | Qt::AlignVCenter,name);

	if(setClipRect)
	{
		painter->restore();
	}
}




void NodeViewDelegate::renderVar(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	QString text;

	if(data.count() >1)
		text+=data.at(1) + "=";
	if(data.count() > 2)
		text+=data.at(2);

	int offset=2;

	//The border rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-1);
	if(option.state & QStyle::State_Selected)
				fillRect.adjust(0,1,0,-1);

	//The text rectangle
	QFont font=attrFont_;
	//nameFont.setBold(true);
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	QRect textRect = fillRect.adjusted(offset,0,0,0);
	textRect.setWidth(textWidth);

	//Adjust the filled rect width
	fillRect.setRight(textRect.right()+offset);

	//Define clipping
	int rightPos=fillRect.right()+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//Draw text
	painter->setPen(Qt::black);
	painter->setFont(font);
	painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

	if(setClipRect)
	{
		painter->restore();
	}
}

void NodeViewDelegate::renderGenvar(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	QString text;

	if(data.count() >1)
		text+=data.at(1) + "=";
	if(data.count() > 2)
		text+=data.at(2);

	int offset=2;

	//The border rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-1);
	if(option.state & QStyle::State_Selected)
				fillRect.adjust(0,1,0,-1);

	//The text rectangle
	QFont font=attrFont_;
	//nameFont.setBold(true);
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	QRect textRect = fillRect.adjusted(offset,0,0,0);
	textRect.setWidth(textWidth);

	//Adjust the filled rect width
	fillRect.setRight(textRect.right()+offset);

	//Define clipping
	int rightPos=fillRect.right()+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//Draw text
	painter->setPen(Qt::blue);
	painter->setFont(font);
	painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

	if(setClipRect)
	{
		painter->restore();
	}
}

void NodeViewDelegate::renderLimit(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	if(data.count() != 4)
			return;

	//The data
	int	val=data.at(2).toInt();
	int	max=data.at(3).toInt();
	QString name=data.at(1) + ":";
	QString valStr=QString::number(val) + "/" + QString::number(max);
	bool drawItem=(max < 21);

	QFontMetrics fm(attrFont_);
	int offset=2;
	int gap=fm.width('A');
	int itemSize=6;
	QColor itemEmptyCol(Qt::gray);
	QColor itemCol(Qt::green);

	//The border rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-1);
	if(option.state & QStyle::State_Selected)
			fillRect.adjust(0,1,0,-1);

	//The text rectangle
	QFont nameFont=attrFont_;
	nameFont.setBold(true);
	fm=QFontMetrics(nameFont);
	int nameWidth=fm.width(name);
	QRect nameRect = fillRect.adjusted(0,2,0,-2);
	nameRect.setLeft(fillRect.left()+gap);
	nameRect.setWidth(nameWidth+offset);

	//The value rectangle
	QFont valFont=attrFont_;
	fm=QFontMetrics(valFont);
	int valWidth=fm.width(valStr);
	QRect valRect = nameRect;
	valRect.setLeft(nameRect.right()+gap);
	valRect.setWidth(valWidth+offset);

	int xItem;
	if(drawItem)
	{
		xItem=valRect.right()+gap;
		fillRect.setRight(xItem+max*(itemSize+offset)+offset);
	}
	else
	{
		//Adjust the filled rect width
		fillRect.setRight(valRect.right()+offset);
	}

	//Define clipping
	int rightPos=fillRect.right()+1;
	const bool setClipRect = rightPos > option.rect.right();

	if(setClipRect || drawItem)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//Draw name	painter->setPen(Qt::black);
	painter->setFont(nameFont);
	painter->drawText(nameRect,Qt::AlignLeft | Qt::AlignVCenter,name);

	//Draw value
	painter->setPen(Qt::black);
	painter->setFont(valFont);
	painter->drawText(valRect,Qt::AlignLeft | Qt::AlignVCenter,valStr);

	//Draw items
	if(drawItem)
	{
		painter->setRenderHint(QPainter::Antialiasing,true);
		painter->setBrush(itemCol);
		int yItem=fillRect.center().y()-itemSize/2;
		for(int i=0; i < max; i++)
		{
			if(i==val)
			{
				painter->setBrush(itemEmptyCol);
			}
			painter->drawEllipse(xItem,yItem,itemSize,itemSize);
			xItem+=offset+itemSize;
		}
		painter->setRenderHint(QPainter::Antialiasing,false);
	}

	if(setClipRect || drawItem)
	{
		painter->restore();
	}
}

void NodeViewDelegate::renderLimiter(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	if(data.count() != 3)
			return;

	QString name="inlimit " + data.at(2) +":" +data.at(1);

	int offset=2;

	//The border rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-1);
	if(option.state & QStyle::State_Selected)
			fillRect.adjust(0,1,0,-1);

	//The text rectangle
	QFont nameFont=attrFont_;
	//nameFont.setBold(true);
	QFontMetrics fm(nameFont);
	int nameWidth=fm.width(name);
	QRect nameRect = fillRect.adjusted(offset,0,0,0);
	nameRect.setWidth(nameWidth);

	//Adjust the filled rect width
	fillRect.setRight(nameRect.right()+offset);

	//Define clipping
	int rightPos=fillRect.right()+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//Draw name
	painter->setPen(Qt::black);
	painter->setFont(nameFont);
	painter->drawText(nameRect,Qt::AlignLeft | Qt::AlignVCenter,name);

	if(setClipRect)
	{
		painter->restore();
	}
}

void NodeViewDelegate::renderTrigger(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	if(data.count() !=3)
			return;

	QString	text=data.at(2);

	int offset=2;

	//The border rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-1);
	if(option.state & QStyle::State_Selected)
		fillRect.adjust(0,1,0,-1);

	//The text rectangle
	QFont font=attrFont_;
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	QRect textRect = fillRect.adjusted(offset,0,0,0);
	textRect.setWidth(textWidth);

	//Adjust the filled rect width
	fillRect.setRight(textRect.right()+offset);

	//Define clipping
	int rightPos=fillRect.right()+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//draw rect
	painter->setBrush(QColor(230,230,230));
	painter->setPen(QColor(150,150,150));
	painter->drawRect(fillRect);

	//Draw text
	painter->setPen(Qt::black);
	painter->setFont(font);
	painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

	if(setClipRect)
	{
		painter->restore();
	}
}

void NodeViewDelegate::renderTime(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	if(data.count() != 2)
			return;

	QString name=data.at(1);

	int offset=2;

	//The border rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-1);
	if(option.state & QStyle::State_Selected)
			fillRect.adjust(0,1,0,-1);

	//The text rectangle
	QFont nameFont=attrFont_;
	//nameFont.setBold(true);
	QFontMetrics fm(nameFont);
	int nameWidth=fm.width(name);
	QRect nameRect = fillRect.adjusted(offset,0,0,0);
	nameRect.setWidth(nameWidth);

	//Adjust the filled rect width
	fillRect.setRight(nameRect.right()+offset);

	//Define clipping
	int rightPos=fillRect.right()+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//Draw name
	painter->setPen(Qt::black);
	painter->setFont(nameFont);
	painter->drawText(nameRect,Qt::AlignLeft | Qt::AlignVCenter,name);

	if(setClipRect)
	{
		painter->restore();
	}
}

void NodeViewDelegate::renderDate(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	if(data.count() != 2)
			return;

	QString name=data.at(1);

	int offset=2;

	//The border rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-1);
	if(option.state & QStyle::State_Selected)
			fillRect.adjust(0,1,0,-1);

	//The text rectangle
	QFont nameFont=attrFont_;
	//nameFont.setBold(true);
	QFontMetrics fm(nameFont);
	int nameWidth=fm.width(name);
	QRect nameRect = fillRect.adjusted(offset,0,0,0);
	nameRect.setWidth(nameWidth);

	//Adjust the filled rect width
	fillRect.setRight(nameRect.right()+offset);

	//Define clipping
	int rightPos=fillRect.right()+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//Draw name
	painter->setPen(Qt::black);
	painter->setFont(nameFont);
	painter->drawText(nameRect,Qt::AlignLeft | Qt::AlignVCenter,name);

	if(setClipRect)
	{
		painter->restore();
	}
}

//========================================================
// data is encoded as a QStringList as follows:
// "repeat" name  value
//========================================================

void NodeViewDelegate::renderRepeat(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	if(data.count() != 3)
			return;

	QString name=data.at(1) + ":";
	QString val=data.at(2);

	int offset=2;

	//The border rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-1);
	if(option.state & QStyle::State_Selected)
			fillRect.adjust(0,1,0,-1);

	//The text rectangle
	QFont nameFont=attrFont_;
	nameFont.setBold(true);
	QFontMetrics fm(nameFont);
	int nameWidth=fm.width(name);
	QRect nameRect = fillRect.adjusted(offset,0,0,0);
	nameRect.setWidth(nameWidth);

	//The value rectangle
	QFont valFont=attrFont_;
	fm=QFontMetrics(valFont);
	int valWidth=fm.width(val);
	QRect valRect = nameRect;
	valRect.setLeft(nameRect.right()+fm.width('A'));
	valRect.setWidth(valWidth);

	//Adjust the filled rect width
	fillRect.setRight(valRect.right()+offset);

	//Define clipping
	int rightPos=fillRect.right()+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//Draw name
	painter->setPen(Qt::black);
	painter->setFont(nameFont);
	painter->drawText(nameRect,Qt::AlignLeft | Qt::AlignVCenter,name);

	//Draw value
	painter->setPen(Qt::black);
	painter->setFont(valFont);
	painter->drawText(valRect,Qt::AlignLeft | Qt::AlignVCenter,val);

	if(setClipRect)
	{
		painter->restore();
	}
}

void NodeViewDelegate::renderLate(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{

	if(data.count() != 2)
			return;

	QString name="late: " + data.at(1);

	int offset=2;

	//The border rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-1);
	if(option.state & QStyle::State_Selected)
			fillRect.adjust(0,1,0,-1);

	//The text rectangle
	QFont nameFont=attrFont_;
	QFontMetrics fm(nameFont);
	int nameWidth=fm.width(name);
	QRect nameRect = fillRect.adjusted(offset,0,0,0);
	nameRect.setWidth(nameWidth);

	//Adjust the filled rect width
	fillRect.setRight(nameRect.right()+offset);

	//Define clipping
	int rightPos=fillRect.right()+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//Draw name
	painter->setPen(Qt::black);
	painter->setFont(nameFont);
	painter->drawText(nameRect,Qt::AlignLeft | Qt::AlignVCenter,name);

	if(setClipRect)
	{
		painter->restore();
	}
}






