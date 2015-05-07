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
#include <QImageReader>
#include <QPainter>

#include "AbstractNodeModel.hpp"
#include "Animation.hpp"
#include "VParam.hpp"

TreeNodeViewDelegate::TreeNodeViewDelegate(QWidget *parent) :
    QStyledItemDelegate(parent)
{
	//The parent mist be the view!!!
	animation_=new AnimationHandler(parent);

	hoverPen_=QPen(QColor(201,201,201));
	hoverBrush_=QBrush(QColor(250,250,250,210));
	selectPen_=QPen(QColor(125,162,206));
	selectBrush_=QBrush(QColor(193,220,252,110));
	nodePen_=QPen(QColor(180,180,180));
	nodeSelectPen_=QPen(QColor(0,0,0),2);

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

	attrRenderers_["meter"]=&TreeNodeViewDelegate::renderMeter;
	attrRenderers_["label"]=&TreeNodeViewDelegate::renderLabel;
	attrRenderers_["event"]=&TreeNodeViewDelegate::renderEvent;
	attrRenderers_["var"]=&TreeNodeViewDelegate::renderVar;
	attrRenderers_["genvar"]=&TreeNodeViewDelegate::renderGenvar;
	attrRenderers_["limit"]=&TreeNodeViewDelegate::renderLimit;
	attrRenderers_["limiter"]=&TreeNodeViewDelegate::renderLimiter;
	attrRenderers_["trigger"]=&TreeNodeViewDelegate::renderTrigger;
	attrRenderers_["time"]=&TreeNodeViewDelegate::renderTime;
	attrRenderers_["date"]=&TreeNodeViewDelegate::renderDate;
	attrRenderers_["repeat"]=&TreeNodeViewDelegate::renderRepeat;
	attrRenderers_["late"]=&TreeNodeViewDelegate::renderLate;
}

TreeNodeViewDelegate::~TreeNodeViewDelegate()
{
	delete animation_;
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

	//Selection - we only do it once
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

	//First column (nodes)
	if(index.column() == 0)
	{
		QVariant tVar=index.data(Qt::DisplayRole);
		QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt, widget);

		if(tVar.type() == QVariant::String)
		{
			QString text=index.data(Qt::DisplayRole).toString();
			if(index.data(AbstractNodeModel::ServerRole).toInt() ==0)
			{
				renderServer(painter,index,vopt,text);
			}
			else
			{
				renderNode(painter,index,vopt,text);
			}
		}
		//Render attributes
		else if(tVar.type() == QVariant::StringList)
		{
			QStringList lst=tVar.toStringList();
			if(lst.count() > 0)
			{
				QMap<QString,AttributeRendererProc>::const_iterator it=attrRenderers_.find(lst.at(0));
				if(it != attrRenderers_.end())
				{
					AttributeRendererProc a=it.value();
					(this->*a)(painter,lst,vopt);
				}
			}

		}
	}
	//rest of the columns
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



void TreeNodeViewDelegate::renderServer(QPainter *painter,const QModelIndex& index,
		                                   const QStyleOptionViewItemV4& option,QString text) const
{
	int offset=2;
	int currentRight=0;

	//The initial filled rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-2);
	if(option.state & QStyle::State_Selected)
		fillRect.adjust(0,1,0,-1);

	//Pixmap rect
	//QRect pixRect = QRect(fillRect.left()+offset,
	//		fillRect.top()+(fillRect.height()-serverPix_.height())/2,
	//		serverPix_.width(),
	//		serverPix_.height());

	//The text rectangle
	QRect textRect = fillRect;
	textRect.setLeft(fillRect.left()+offset);
	QFont font;

	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	textRect.setWidth(textWidth);

	//Adjust the filled rect width
	fillRect.setRight(textRect.right()+offset);

	currentRight=fillRect.right();

	//The pixmap (optional)
	QRect pixRect;
	bool hasPix=false;
	hasPix=(index.data(AbstractNodeModel::IconRole).toString() == "d");

	if(hasPix)
	{
		pixRect = QRect(fillRect.right()+fm.width('A'),
				fillRect.top()+(fillRect.height()-errPix_.height())/2,
				errPix_.width(),
				errPix_.height());

		hasPix=true;
		currentRight=pixRect.right();
	}

	//The info rectangle (optional)
	QRect infoRect;
	QString infoTxt=index.data(AbstractNodeModel::InfoRole).toString();
	bool hasInfo=(infoTxt.isEmpty() == false);
	QFont infoFont;

	if(hasInfo)
	{
		//infoFont.setBold(true);
		fm=QFontMetrics(infoFont);

		int infoWidth=fm.width(infoTxt);
		infoRect = textRect;
		infoRect.setLeft(currentRight+fm.width('A'));
		infoRect.setWidth(infoWidth);
		currentRight=infoRect.right();
	}

	//The load icon (optional)
	QRect loadRect;
	bool hasLoad=index.data(AbstractNodeModel::LoadRole).toBool();
	Animation* an=0;

	//Update load animation
	if(hasLoad)
	{
		an=animation_->find(Animation::ServerLoadType,true);

		loadRect = QRect(currentRight+fm.width('A'),
						fillRect.top()+(fillRect.height()-an->scaledSize().height())/2,
						an->scaledSize().width(),
						an->scaledSize().height());

		currentRight=loadRect.right();

		//Add this index to the animations
		an->addTarget(index);
	}
	//Stops load animation
	else
	{
		if((an=animation_->find(Animation::ServerLoadType,false)) != NULL)
			an->removeTarget(index);
	}

	//The node number (optional)
	QRect numRect;
	QString numTxt;
	QVariant va=index.data(AbstractNodeModel::NodeNumRole);
	bool hasNum=(va.isNull() == false);
	QFont numFont;

	if(hasNum)
	{
		numTxt="(" + QString::number(va.toInt()) + ")";

		numFont.setBold(true);
		fm=QFontMetrics(numFont);

		int numWidth=fm.width(numTxt);
		numRect = textRect;
		numRect.setLeft(currentRight+fm.width('A'));
		numRect.setWidth(numWidth);
		currentRight=numRect.right();
	}

	//Define clipping
	int rightPos=currentRight+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//Draw bg rect
	QColor bg=index.data(Qt::BackgroundRole).value<QColor>();
	painter->fillRect(fillRect,bg);
	painter->setPen((option.state & QStyle::State_Selected)?nodeSelectPen_:nodePen_);
	painter->drawRect(fillRect);

	//Draw text
	if(bg == QColor(Qt::red))
			painter->setPen(Qt::white);
	else
			painter->setPen(Qt::black);

	painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

	//Draw pixmap if needed
	if(hasPix)
	{
		painter->drawPixmap(pixRect,errPix_);
	}

	//Draw info
	if(hasInfo)
	{
		painter->setPen(Qt::black);
		painter->setFont(infoFont);
		painter->drawText(infoRect,Qt::AlignLeft | Qt::AlignVCenter,infoTxt);
	}

	//Draw number
	if(hasNum)
	{
		painter->setPen(Qt::black);
		painter->setFont(numFont);
		painter->drawText(numRect,Qt::AlignLeft | Qt::AlignVCenter,numTxt);
	}

	//Draw load animation
	if(hasLoad)
	{
		Animation* an=animation_->find(Animation::ServerLoadType,false);
		if(an)
		{
			painter->drawPixmap(loadRect,an->currentPixmap());
		}
	}

	if(setClipRect)
	{
		painter->restore();
	}
}


void TreeNodeViewDelegate::renderNode(QPainter *painter,const QModelIndex& index,
        							const QStyleOptionViewItemV4& option,QString text) const
{
	int offset=2;

	//The initial filled rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-2);
	if(option.state & QStyle::State_Selected)
		fillRect.adjust(0,1,0,-1);

	//The text rectangle
	QRect textRect = fillRect.adjusted(offset,0,0,0);

	QFont font;
	QFontMetrics fm(font);
	int textWidth=fm.width(text);
	textRect.setWidth(textWidth);

	//Adjust the filled rect width
	fillRect.setRight(textRect.right()+offset);

	int currentRight=fillRect.right();

	//Icons area
	QList<QPixmap> pixLst;
	QList<QRect> pixRectLst;
	QVariant va=index.data(AbstractNodeModel::IconRole);
	if(va.type() == QVariant::List)
	{
			QVariantList lst=va.toList();
			int xp=currentRight+5;
			int yp=fillRect.top();
			for(int i=0; i < lst.count(); i++)
			{
				pixLst << lst[i].value<QPixmap>();
				pixRectLst << QRect(xp,yp,pixLst.back().width(),pixLst.back().height());
				xp+=pixLst.back().width();
			}

			if(!pixRectLst.isEmpty())
				currentRight=pixRectLst.back().right();
	}

	//The node number (optional)
	QRect numRect;
	QString numTxt;
	va=index.data(AbstractNodeModel::NodeNumRole);
	bool hasNum=(va.isNull() == false);
	QFont numFont;

	if(hasNum)
	{
		numTxt="(" + QString::number(va.toInt()) + ")";
		numFont.setBold(true);
		fm=QFontMetrics(numFont);

		int numWidth=fm.width(numTxt);
		numRect = textRect;
		numRect.setLeft(currentRight+fm.width('A'));
		numRect.setWidth(numWidth);
		currentRight=numRect.right();
	}

	//Define clipping
	int rightPos=currentRight+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	//Draw bg rect
	QColor bg=index.data(Qt::BackgroundRole).value<QColor>();
	painter->fillRect(fillRect,bg);
	painter->setPen((option.state & QStyle::State_Selected)?nodeSelectPen_:nodePen_);
	painter->drawRect(fillRect);

	//Draw text
	if(bg == QColor(Qt::red))
			painter->setPen(Qt::white);
	else
			painter->setPen(Qt::black);

	painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

	//Draw icons
	for(int i=0; i < pixLst.count(); i++)
	{
		painter->drawPixmap(pixRectLst[i],pixLst[i]);
	}

	//Draw number
	if(hasNum)
	{
		painter->setPen(QColor(120,120,120));
		painter->setFont(numFont);
		painter->drawText(numRect,Qt::AlignLeft | Qt::AlignVCenter,numTxt);
	}

	if(setClipRect)
	{
		painter->restore();
	}
}

//========================================================
// data is encoded as a QStringList as follows:
// "meter" name  value min  max colChange
//========================================================

void TreeNodeViewDelegate::renderMeter(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	if(data.count() != 6)
			return;

	//The data
	int	val=data.at(2).toInt();
	int	min=data.at(3).toInt();
	int	max=data.at(4).toInt();
	bool colChange=data.at(5).toInt();
	QString name=data.at(1) + ":";
	QString valStr=data.at(2) + " (" +
			QString::number(100.*static_cast<float>(val)/static_cast<float>(max-min)) + "%)";

	int offset=2;
	int gap=5;

	//The border rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,1,0,-1);
	if(option.state & QStyle::State_Selected)
			fillRect.adjust(0,1,0,-1);

	//The status rectangle
	QRect stRect=fillRect.adjusted(offset,2,0,-2);
	stRect.setWidth(50);

	//The text rectangle
	QFont nameFont;
	nameFont.setBold(true);
	QFontMetrics fm(nameFont);
	int nameWidth=fm.width(name);
	QRect nameRect = stRect;
	nameRect.setLeft(stRect.right()+fm.width('A'));
	nameRect.setWidth(nameWidth);

	//The value rectangle
	QFont valFont;
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

void TreeNodeViewDelegate::renderLabel(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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

	//The text rectangle
	QFont nameFont;
	nameFont.setBold(true);
	QFontMetrics fm(nameFont);
	int nameWidth=fm.width(name);
	QRect nameRect = fillRect.adjusted(offset,0,0,0);
	nameRect.setWidth(nameWidth);

	//The value rectangle
	QFont valFont;
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


//========================================================
// data is encoded as a QStringList as follows:
// "event" name  value
//========================================================

void TreeNodeViewDelegate::renderEvent(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
	QFont font;
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




void TreeNodeViewDelegate::renderVar(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
	QFont font;
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

void TreeNodeViewDelegate::renderGenvar(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
	QFont font;
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

void TreeNodeViewDelegate::renderLimit(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	if(data.count() != 4)
			return;

	//The data
	int	val=data.at(2).toInt();
	int	max=data.at(3).toInt();
	QString name=data.at(1) + ":";
	QString valStr=QString::number(val) + "/" + QString::number(max);
	bool drawItem=(max < 21);

	QFontMetrics fm(QApplication::font());
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
	QFont nameFont;
	nameFont.setBold(true);
	fm=QFontMetrics(nameFont);
	int nameWidth=fm.width(name);
	QRect nameRect = fillRect.adjusted(0,2,0,-2);
	nameRect.setLeft(fillRect.left()+gap);
	nameRect.setWidth(nameWidth+offset);

	//The value rectangle
	QFont valFont;
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

void TreeNodeViewDelegate::renderLimiter(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
	QFont nameFont;
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

void TreeNodeViewDelegate::renderTrigger(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	/*if(data.count() !=3)
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
	}*/
}

void TreeNodeViewDelegate::renderTime(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
	QFont nameFont;
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

void TreeNodeViewDelegate::renderDate(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
	QFont nameFont;
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

void TreeNodeViewDelegate::renderRepeat(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
	QFont nameFont;
	nameFont.setBold(true);
	QFontMetrics fm(nameFont);
	int nameWidth=fm.width(name);
	QRect nameRect = fillRect.adjusted(offset,0,0,0);
	nameRect.setWidth(nameWidth);

	//The value rectangle
	QFont valFont;
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

void TreeNodeViewDelegate::renderLate(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	/*if(data.count() !=2)
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
	}*/
}
