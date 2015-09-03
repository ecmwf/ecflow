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
#include "PropertyMapper.hpp"

static std::vector<std::string> propVec;

TreeNodeViewDelegate::TreeNodeViewDelegate(QWidget *parent) :
    nodeRectRad_(0),
    drawChildCount_(true)
{
	//Property
	if(propVec.empty())
	{
		propVec.push_back("view.tree.font");
		propVec.push_back("view.tree.displayChildCount");
	}

    prop_=new PropertyMapper(propVec,this);

    updateSettings();

	//The parent must be the view!!!
	animation_=new AnimationHandler(parent);

    /*hoverPen_=QPen(QColor(201,201,201));
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
    }*/

    /*attrRenderers_["meter"]=&TreeNodeViewDelegate::renderMeter;
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
    attrRenderers_["late"]=&TreeNodeViewDelegate::renderLate;*/
}

TreeNodeViewDelegate::~TreeNodeViewDelegate()
{
	delete animation_;
    //delete prop_;
}

/*void TreeNodeViewDelegate::notifyChange(VProperty* p)
{
	updateSettings();
}*/

void TreeNodeViewDelegate::updateSettings()
{
	if(VProperty* p=prop_->find("view.tree.nodeRectRadius"))
	{
		nodeRectRad_=p->value().toInt();
	}
	if(VProperty* p=prop_->find("view.tree.font"))
	{
		font_=p->value().value<QFont>();

		serverInfoFont_=font_;
		serverNumFont_=font_;
		serverNumFont_.setBold(true);
		suiteNumFont_=font_;
		suiteNumFont_.setBold(true);

	}
	if(VProperty* p=prop_->find("view.tree.displayChildCount"))
	{
		drawChildCount_=p->value().toBool();
	}
}

/*QSize TreeNodeViewDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	QSize size=QStyledItemDelegate::sizeHint(option,index);

	QFontMetrics fm(font_);
	int h=fm.height();
	return QSize(size.width(),h+8);
}*/


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
	/*if(index.column() == 0)
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
	}*/

	if(index.data(AbstractNodeModel::ConnectionRole).toInt() == 0)
	{
		QRect fullRect=QRect(0,option.rect.y(),painter->device()->width(),option.rect.height());
		painter->fillRect(fullRect,lostConnectBgBrush_);
		QRect bandRect=QRect(0,option.rect.y(),5,option.rect.height());
		painter->fillRect(bandRect,lostConnectBandBrush_);

	}

	//First column (nodes)
	if(index.column() == 0)
	{
		QVariant tVar=index.data(Qt::DisplayRole);
		QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt, widget);

		painter->setFont(font_);

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


void TreeNodeViewDelegate::renderServer(QPainter *painter,const QModelIndex& index,
		                                   const QStyleOptionViewItemV4& option,QString text) const
{
	int offset=4;
	int currentRight=0;

	QFontMetrics fm(font_);
	int deltaH=(option.rect.height()-(fm.height()+4))/2;

	//The initial filled rect (we will adjust its  width)
	//QRect fillRect=option.rect.adjusted(offset,1,0,-2);
	QRect fillRect=option.rect.adjusted(offset,deltaH,0,-deltaH-1);
	if(option.state & QStyle::State_Selected)
		fillRect.adjust(0,0,0,0);


	//Pixmap rect
	//QRect pixRect = QRect(fillRect.left()+offset,
	//		fillRect.top()+(fillRect.height()-serverPix_.height())/2,
	//		serverPix_.width(),
	//		serverPix_.height());

	//The text rectangle
	QRect textRect = fillRect;
	textRect.setLeft(fillRect.left()+offset);

	int textWidth=fm.width(text);
	textRect.setWidth(textWidth);

	//Adjust the filled rect width
	fillRect.setRight(textRect.right()+offset);

	QRect halfRect=fillRect;
	halfRect.setY(halfRect.center().y());
	halfRect.adjust(1,0,-1,-1);

	currentRight=fillRect.right();

	//The pixmap (optional)
	QRect pixRect;

	bool hasPix=(index.data(AbstractNodeModel::IconRole).toString() == "d");

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

	if(hasInfo)
	{
		//infoFont.setBold(true);
		fm=QFontMetrics(serverInfoFont_);

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
	bool hasNum=false;

	if(drawChildCount_)
	{
		QVariant va=index.data(AbstractNodeModel::NodeNumRole);
		hasNum=(va.isNull() == false);
		if(hasNum)
		{
			numTxt="(" + QString::number(va.toInt()) + ")";

			fm=QFontMetrics(serverNumFont_);

			int numWidth=fm.width(numTxt);
			numRect = textRect;
			numRect.setLeft(currentRight+fm.width('A'));
			numRect.setWidth(numWidth);
			currentRight=numRect.right();
		}
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
	QColor borderCol=bg.darker(125);
	painter->fillRect(fillRect,bg);
	//painter->setPen((option.state & QStyle::State_Selected)?nodeSelectPen_:nodePen_);
	painter->setPen((option.state & QStyle::State_Selected)?nodeSelectPen_:QPen(borderCol));
	painter->drawRect(fillRect);

	//Draw shading
	QColor shCol= bg.darker(108);
	painter->fillRect(halfRect,shCol);

	//painter->fillRect(fillRect,bg);
	//painter->setPen((option.state & QStyle::State_Selected)?nodeSelectPen_:nodePen_);
	//painter->drawRect(fillRect);

	//Draw text
	QColor fg=index.data(Qt::ForegroundRole).value<QColor>();
	painter->setPen(fg);
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
		painter->setFont(serverInfoFont_);
		painter->drawText(infoRect,Qt::AlignLeft | Qt::AlignVCenter,infoTxt);
	}

	//Draw number
	if(hasNum)
	{
		painter->setPen(Qt::black);
		painter->setFont(serverNumFont_);
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
	int offset=4;

	QFontMetrics fm(font_);
	int deltaH=(option.rect.height()-(fm.height()+4))/2;

	//The initial filled rect (we will adjust its  width)
	QRect fillRect=option.rect.adjusted(offset,deltaH,0,-deltaH-1);
	if(option.state & QStyle::State_Selected)
		fillRect.adjust(0,0,0,-0);

	//get the colours
	QColor bg,realBg;
	QVariant bgVa=index.data(Qt::BackgroundRole);
	bool hasRealBg=false;
	if(bgVa.type() == QVariant::List)
	{
		QVariantList lst=bgVa.toList();
		if(lst.count() ==  2)
		{
			hasRealBg=true;
			bg=lst[0].value<QColor>();
			realBg=lst[1].value<QColor>();
		}
	}
	else
	{
		bg=bgVa.value<QColor>();
	}

	//The text rectangle
	QRect textRect = fillRect.adjusted(offset,0,0,0);

	int textWidth=fm.width(text);
	textRect.setWidth(textWidth);

	//Adjust the filled rect width
	fillRect.setRight(textRect.right()+offset);

	QRect halfRect=fillRect;
	halfRect.setY(fillRect.center().y());
	halfRect.adjust(1,0,-1,-1);

	int currentRight=fillRect.right();

	QRect realRect;
	if(hasRealBg)
	{
		realRect=QRect(fillRect.right()+1,fillRect.top(),6,fillRect.height());
		currentRight=realRect.right()+2;
	}

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
	bool hasNum=false;

	if(drawChildCount_)
	{
		va=index.data(AbstractNodeModel::NodeNumRole);
		hasNum=(va.isNull() == false);

		if(hasNum)
		{
			numTxt="(" + QString::number(va.toInt()) + ")";
			fm=QFontMetrics(suiteNumFont_);

			int numWidth=fm.width(numTxt);
			numRect = textRect;
			numRect.setLeft(currentRight+fm.width('A'));
			numRect.setWidth(numWidth);
			currentRight=numRect.right();
		}
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
	//QColor bg=index.data(Qt::BackgroundRole).value<QColor>();
	QColor borderCol=bg.darker(125);
	painter->fillRect(fillRect,bg);
	//painter->setPen((option.state & QStyle::State_Selected)?nodeSelectPen_:nodePen_);
	painter->setPen((option.state & QStyle::State_Selected)?nodeSelectPen_:QPen(borderCol));
	painter->drawRect(fillRect);

	//Draw shading
	QColor shCol= bg.darker(108);
	painter->fillRect(halfRect,shCol);

	if(hasRealBg)
	{
		QColor realBorderCol=realBg.darker(125);
		painter->fillRect(realRect,realBg);
		painter->setPen((option.state & QStyle::State_Selected)?nodeSelectPen_:QPen(realBorderCol));
		painter->drawRect(realRect);
	}


	//Draw text
	QColor fg=index.data(Qt::ForegroundRole).value<QColor>();
	painter->setPen(fg);
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
		painter->setFont(suiteNumFont_);
		painter->drawText(numRect,Qt::AlignLeft | Qt::AlignVCenter,numTxt);
	}

	if(setClipRect)
	{
		painter->restore();
	}
}
