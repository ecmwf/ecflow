//============================================================================
// Copyright 2016 ECMWF.
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

int NodeViewDelegate::lighter_=150;

static std::vector<std::string> propVec;

NodeViewDelegate::NodeViewDelegate(QWidget *parent) :
    QStyledItemDelegate(parent),
    prop_(0),
	iconSize_(16),
	iconGap_(3),
    useStateGrad_(true),
    drawAttrSelectionRect_(false)
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

	grad_.setCoordinateMode(QGradient::ObjectBoundingMode);
	grad_.setStart(0,0);
	grad_.setFinalStop(0,1);

    eventFillBrush_=QBrush(QColor(0,0,255));
    eventFillBrush_=QBrush(QColor(240,240,240));
    meterFillBrush_=QBrush(QColor(0,0,255));
    meterThresholdBrush_=QBrush(QColor(0,0,255));
    limitFillBrush_=QBrush(QColor(0,255,0));

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

void NodeViewDelegate::addBaseSettings(std::vector<std::string>& propVec)
{
    propVec.push_back("view.common.node_gradient");
    propVec.push_back("view.attribute.eventFillColour");
    propVec.push_back("view.attribute.meterFillColour");
    propVec.push_back("view.attribute.meterThresholdColour");
    propVec.push_back("view.attribute.limitFillColour");
}

void NodeViewDelegate::updateBaseSettings()
{
    if(VProperty* p=prop_->find("view.common.node_gradient"))
    {
        useStateGrad_=p->value().toBool();
    }
    if(VProperty* p=prop_->find("view.attribute.eventFillColour"))
    {
        eventFillBrush_=QBrush(p->value().value<QColor>());
    }
    if(VProperty* p=prop_->find("view.attribute.meterFillColour"))
    {
        QLinearGradient gr;
        gr.setCoordinateMode(QGradient::ObjectBoundingMode);
        gr.setStart(0,0);
        gr.setFinalStop(0,1);
        QColor c1=p->value().value<QColor>();
        gr.setColorAt(0,c1);
        gr.setColorAt(1,c1.lighter(110));
        meterFillBrush_=QBrush(gr);
    }
    if(VProperty* p=prop_->find("view.attribute.meterThresholdColour"))
    {
        QLinearGradient gr;
        gr.setCoordinateMode(QGradient::ObjectBoundingMode);
        gr.setStart(0,0);
        gr.setFinalStop(0,1);
        QColor c1=p->value().value<QColor>();
        gr.setColorAt(0,c1);
        gr.setColorAt(1,c1.lighter(110));
        meterThresholdBrush_=QBrush(gr);
    }
    if(VProperty* p=prop_->find("view.attribute.limitFillColour"))
    {
        limitFillBrush_=QBrush(p->value().value<QColor>());
    }

    //limit pixmaps
    QFontMetrics fm(attrFont_);
    int itemSize=static_cast<int>(static_cast<float>(fm.ascent())*0.9);

    QImage img(itemSize,itemSize,QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter painter(&img);
    //painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setPen(QPen(QColor(70,70,70),0));
    painter.setBrush(limitFillBrush_);
    painter.drawEllipse(1,1,itemSize-2,itemSize-2);
    limitFillPix_=QPixmap::fromImage(img);
    painter.fillRect(QRect(QPoint(0,0),img.size()),Qt::transparent);
    painter.setBrush(QColor(240,240,240));
    painter.drawEllipse(1,1,itemSize-2,itemSize-2);
    limitEmptyPix_=QPixmap::fromImage(img);
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
	int h=fm.ascent()+fm.descent()/2;
	iconSize_=h;
	if(iconSize_ % 2 == 1)
        iconSize_+=1;

	iconGap_=iconSize_/5;
	if(iconGap_ < 3)
		iconGap_=3;
	if(iconGap_ > 10)
		iconGap_=10;

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

void NodeViewDelegate::renderSelectionRect(QPainter* painter,QRect r) const
{
    painter->setPen(nodeSelectPen_);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(r);
}

void NodeViewDelegate::renderStatus(QPainter *painter,const QModelIndex& index,
                                    const QStyleOptionViewItemV4& option) const
{
    int offset=4;

    QFontMetrics fm(font_);
    int deltaH=(option.rect.height()-(fm.height()+4))/2;

    //The initial filled rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(offset,deltaH,-offset,-deltaH-1);
    if(option.state & QStyle::State_Selected)
        fillRect.adjust(0,0,0,-0);

    int currentRight=fillRect.right();

    //The text rectangle
    QString text=index.data(Qt::DisplayRole).toString();
    int textWidth=fm.width(text);
    QRect textRect = fillRect.adjusted(offset,0,0,0);
    textRect.setWidth(textWidth);

    if(textRect.right() > currentRight)
    	currentRight=textRect.right();

    //Define clipping
    int rightPos=currentRight+1;
    const bool setClipRect = rightPos > option.rect.right();
    if(setClipRect)
    {
    	painter->save();
    	QRect cr=option.rect.adjusted(0,0,-offset,0);
    	painter->setClipRect(cr);
    }

    //Fill rect
    QColor bg=index.data(Qt::BackgroundRole).value<QColor>();
    QColor bgLight=bg.lighter(lighter_);
    QBrush bgBrush;
    if(useStateGrad_)
    {
       grad_.setColorAt(0,bgLight);
       grad_.setColorAt(1,bg);
       bgBrush=QBrush(grad_);
    }
    else
       bgBrush=QBrush(bg);

    painter->fillRect(fillRect,bgBrush);

    //Draw text
    painter->setFont(font_);
    painter->setPen(Qt::black);
    painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

    if(setClipRect)
    {
        painter->restore();
    }
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
    int threshold=data.at(5).toInt();

    bool selected=option.state & QStyle::State_Selected;

    //float percent=static_cast<float>(val-min)/static_cast<float>(max-min);
    //float thresholdPercent=static_cast<float>(threshold-min)/static_cast<float>(max-min);

    QString name=data.at(1) + ":";
    QString valStr=data.at(2); // + " (" + QString::number(100.*percent) + "%)";

    int frontOffset=8;
    int offset=2;
	//int gap=5;

	//The border rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(frontOffset,1,0,-1);
    if(selected)
        fillRect.adjust(0,1,0,-1);

    QFontMetrics fm(attrFont_);

    //The status rectangle
    int stHeight=static_cast<int>(static_cast<float>(fm.ascent())*0.9);
    int stHeightDiff=(fillRect.height()-stHeight)/2;
    QRect stRect=fillRect.adjusted(offset,stHeightDiff,
                                   0,-(fillRect.height()-stHeight-stHeightDiff));
	stRect.setWidth(50);

	//The text rectangle
	QFont nameFont=attrFont_;
	nameFont.setBold(true);
    fm=QFontMetrics(nameFont);
	int nameWidth=fm.width(name);
    QRect nameRect = fillRect;
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

    //Fill st rect
    painter->fillRect(stRect,QColor(229,229,229));   

    //Draw progress
    if(max > min)
    {
        QRect progRect=stRect;

        float valPercent=static_cast<float>(val-min)/static_cast<float>(max-min);
        if(threshold > min && threshold < max && val > threshold)
        {
            int progWidth=static_cast<int>(static_cast<float>(stRect.width())*valPercent);
            if(val < max)
            {
                progRect.setWidth(progWidth);
            }
            painter->fillRect(progRect,meterThresholdBrush_);

            float thresholdPercent=static_cast<float>(threshold-min)/static_cast<float>(max-min);
            progWidth=static_cast<int>(static_cast<float>(stRect.width())*thresholdPercent);
            progRect.setWidth(progWidth);
            painter->fillRect(progRect,meterFillBrush_);
        }
        else
        {
            int progWidth=static_cast<int>(static_cast<float>(stRect.width())*valPercent);
            if(val < max)
            {
                progRect.setWidth(progWidth);
            }
            painter->fillRect(progRect,meterFillBrush_);
        }
    }

    //Draw st rect border
    if(max > min)
    {
        painter->setPen(QColor(140,140,140));
    }
    else
    {
        painter->setPen(QPen(QColor(140,140,140),Qt::DotLine));
    }
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(stRect);

    //Draw name
	painter->setPen(Qt::black);
	painter->setFont(nameFont);
	painter->drawText(nameRect,Qt::AlignLeft | Qt::AlignVCenter,name);

	//Draw value
	painter->setPen(Qt::black);
	painter->setFont(valFont);
	painter->drawText(valRect,Qt::AlignLeft | Qt::AlignVCenter,valStr);

    if(selected && drawAttrSelectionRect_)
    {
        QRect sr=stRect;
        sr.setRight(valRect.right());
        renderSelectionRect(painter,sr.adjusted(-3,-2,2,3));
    }

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

    int frontOffset=8;
    int offset=2;

    bool selected=option.state & QStyle::State_Selected;

	//The border rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(frontOffset,1,0,-1);
    if(selected)
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

		multiVal=vals.join(QString('\n'));

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

    if(selected && drawAttrSelectionRect_)
    {
        QRect sr=nameRect;
        sr.setRight(valRect.right());
        if(multiCnt > 0)
        {
            sr.setRight(multiValRect.right());
            sr.setBottom(multiValRect.bottom());
        }

        renderSelectionRect(painter,sr.adjusted(-3,0,2,0));
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

    int frontOffset=8;
    int offset=2;

    bool selected=option.state & QStyle::State_Selected;

	//The border rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(frontOffset,1,0,-1);
    if(selected)
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
    nameRect.setLeft(cRect.right()+fm.width('a'));
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
    painter->setBrush((val)?eventFillBrush_:eventBgBrush_);
	painter->drawRect(cRect);

	//Draw name
	painter->setPen(Qt::black);
	painter->setFont(font);
	painter->drawText(nameRect,Qt::AlignLeft | Qt::AlignVCenter,name);

    if(selected && drawAttrSelectionRect_)
    {
        QRect sr=cRect;
        sr.setRight(nameRect.right());
        renderSelectionRect(painter,sr.adjusted(-3,-3,2,4));
    }

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
    int frontOffset=8;

    bool selected=option.state & QStyle::State_Selected;

	//The border rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(frontOffset,1,0,-1);
    if(selected)
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

    if(selected && drawAttrSelectionRect_)
    {
        QRect sr=textRect;
        renderSelectionRect(painter,sr.adjusted(-3,0,2,0));
    }

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
    int frontOffset=8;

    bool selected=option.state & QStyle::State_Selected;

	//The border rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(frontOffset,1,0,-1);
    if(selected)
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

    if(selected && drawAttrSelectionRect_)
    {
        QRect sr=textRect;
        renderSelectionRect(painter,sr.adjusted(-3,0,2,0));
    }

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
    int frontOffset=8;

    int itemOffset=0;
	int gap=fm.width('A');
    int itemSize=limitFillPix_.width();

    bool selected=option.state & QStyle::State_Selected;

	//The border rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(frontOffset,1,0,-1);
    if(selected)
        fillRect.adjust(0,1,0,-1);

	//The text rectangle
	QFont nameFont=attrFont_;
	nameFont.setBold(true);
	fm=QFontMetrics(nameFont);
	int nameWidth=fm.width(name);
    QRect nameRect = fillRect.adjusted(offset,2,0,-2);
    //nameRect.setLeft(fillRect.left());
    nameRect.setWidth(nameWidth);

	//The value rectangle
	QFont valFont=attrFont_;
	fm=QFontMetrics(valFont);
	int valWidth=fm.width(valStr);
	QRect valRect = nameRect;
	valRect.setLeft(nameRect.right()+gap);
    valRect.setWidth(valWidth);

	int xItem;
	if(drawItem)
	{
		xItem=valRect.right()+gap;
		fillRect.setRight(xItem+max*(itemSize+itemOffset)+itemOffset);
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
		int yItem=option.rect.y()+(option.rect.height()-itemSize)/2;
		for(int i=0; i < max; i++)
		{	 
            if(i >= val)
			{

                painter->drawPixmap(xItem,yItem,itemSize,itemSize,limitEmptyPix_);
			}
            else
            {
                painter->drawPixmap(xItem,yItem,itemSize,itemSize,limitFillPix_);
            }


			xItem+=itemOffset+itemSize;
		}		
	}

    if(selected && drawAttrSelectionRect_)
    {
        QRect sr=nameRect;
        sr.setRight(valRect.right());
        renderSelectionRect(painter,sr.adjusted(-3,-1,3,1));
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
    int frontOffset=8;

    bool selected=option.state & QStyle::State_Selected;

	//The border rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(frontOffset,1,0,-1);
    if(selected)
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

    if(selected && drawAttrSelectionRect_)
    {
        QRect sr=nameRect;
        renderSelectionRect(painter,sr.adjusted(-3,0,2,0));
    }

	if(setClipRect)
	{
		painter->restore();
	}
}

void NodeViewDelegate::renderTrigger(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	if(data.count() !=3)
        return;

    int triggerType=data[1].toInt();

	QString	text=data.at(2);

    int offset=2;
    int frontOffset=8;

    bool selected=option.state & QStyle::State_Selected;

	//The border rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(frontOffset,2,0,-2);
    if(selected)
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
    if(triggerType==0)
        painter->setPen(Qt::black);
    else
        painter->setPen(Qt::blue);

    painter->setFont(font);
	painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

    if(selected && drawAttrSelectionRect_)
    {
        QRect sr=textRect;
        renderSelectionRect(painter,sr.adjusted(-3,0,2,0));
    }

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
    int frontOffset=8;

    bool selected=option.state & QStyle::State_Selected;

	//The border rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(frontOffset,1,0,-1);
    if(selected)
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

    if(selected && drawAttrSelectionRect_)
    {
        QRect sr=nameRect;
        renderSelectionRect(painter,sr.adjusted(-3,0,2,0));
    }

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
    int frontOffset=8;

    bool selected=option.state & QStyle::State_Selected;

	//The border rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(frontOffset,1,0,-1);
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

    if(selected && drawAttrSelectionRect_)
    {
        QRect sr=nameRect;
        renderSelectionRect(painter,sr.adjusted(-3,0,2,0));
    }

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
    if(data.count() !=  7)
        return;

    QString type=data.at(1);
    QString name=data.at(2);
    QString val=data.at(3);
    QString start=data.at(4);
    QString end=data.at(5);
    QString step=data.at(6);

    int offset=2;
    int frontOffset=8;

    bool selected=option.state & QStyle::State_Selected;

	//The border rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(frontOffset,1,0,-1);
    if(selected)
        fillRect.adjust(0,1,0,-1);

    if(type == "day")
    {
        QFont nameFont=attrFont_;
        QFontMetrics fm(nameFont);
        name="day=" + step;
        int nameWidth=fm.width(name);
        QRect nameRect = fillRect.adjusted(offset,0,0,0);
        nameRect.setWidth(nameWidth);

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

        if(selected && drawAttrSelectionRect_)
        {
            QRect sr=nameRect;
            renderSelectionRect(painter,sr.adjusted(-3,0,2,0));
        }

        if(setClipRect)
        {
            painter->restore();
        }
    }
    else
    {
        QString endDot;
        if(start == val)
        {
            name+="=";
            endDot="...";
        }
        else if(end == val)
        {
            name+="=...";
            endDot="";
        }
        else
        {
            name+="=...";
            endDot="...";
        }

        //The name rectangle
        QFont nameFont=attrFont_;
        QFontMetrics fm(nameFont);
        int nameWidth=fm.width(name);
        QRect nameRect = fillRect.adjusted(offset,0,0,0);
        nameRect.setWidth(nameWidth);

        //The value rectangle
        QFont valFont=attrFont_;
        valFont.setBold(true);
        fm=QFontMetrics(valFont);
        int valWidth=fm.width(val);
        QRect valRect = nameRect;
        if(name.endsWith("..."))
            valRect.setLeft(nameRect.right() + fm.width('A')/2);
        else
            valRect.setLeft(nameRect.right() + fm.width(' ')/2);

        valRect.setWidth(valWidth);

        //Adjust the filled rect width
        fillRect.setRight(valRect.right()+offset);

        //End ...
        QRect dotRect;
        if(!endDot.isEmpty())
        {
            fm=QFontMetrics(nameFont);
            int dotWidth=fm.width("...");
            dotRect = valRect;
            dotRect.setLeft(valRect.right()+fm.width('A')/2);
            dotRect.setWidth(dotWidth);

            //Adjust the filled rect width
            fillRect.setRight(dotRect.right()+offset);
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

        //Draw end dots
        if(!endDot.isEmpty())
        {
            painter->setPen(Qt::black);
            painter->setFont(nameFont);
            painter->drawText(dotRect,Qt::AlignLeft | Qt::AlignVCenter,"...");
        }

        if(selected && drawAttrSelectionRect_)
        {
            QRect sr=nameRect;
            sr.setRight(valRect.right());
            if(!endDot.isEmpty())
                sr.setRight(dotRect.right());

            renderSelectionRect(painter,sr.adjusted(-2,0,2,0));
        }

        if(setClipRect)
        {
            painter->restore();
        }
     }
}

void NodeViewDelegate::renderLate(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
	if(data.count() != 2)
        return;

	QString name="late: " + data.at(1);

    int offset=2;
    int frontOffset=8;

    bool selected=option.state & QStyle::State_Selected;

	//The border rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(frontOffset,1,0,-1);
    if(selected)
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

    if(selected && drawAttrSelectionRect_)
    {
        QRect sr=nameRect;
        renderSelectionRect(painter,sr.adjusted(-3,0,2,0));
    }

	if(setClipRect)
	{
		painter->restore();
	}
}
