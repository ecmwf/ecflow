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
#include <QLinearGradient>
#include <QPainter>

#include "AbstractNodeModel.hpp"
#include "Animation.hpp"
#include "IconProvider.hpp"
#include "PropertyMapper.hpp"

static std::vector<std::string> propVec;

TreeNodeViewDelegate::TreeNodeViewDelegate(QWidget *parent) :
    nodeRectRad_(0),
    drawChildCount_(true),
    nodeStyle_(ClassicNodeStyle),
	indentation_(0)
{
	attrFont_=font_;
	attrFont_.setPointSize(8);

	serverInfoFont_=font_;

	serverInfoFont_=font_;
	serverNumFont_.setBold(true);

	suiteNumFont_=font_;
	suiteNumFont_.setBold(true);

	abortedReasonFont_=font_;
	abortedReasonFont_.setBold(true);

	adjustIconSize();

	//Property
	if(propVec.empty())
	{
		propVec.push_back("view.tree.nodeFont");
		propVec.push_back("view.tree.attributeFont");
		propVec.push_back("view.tree.display_child_count");
        propVec.push_back("view.common.node_style");
        propVec.push_back("view.common.node_gradient");
	}

    prop_=new PropertyMapper(propVec,this);

    updateSettings();

	//The parent must be the view!!!
	animation_=new AnimationHandler(parent);
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
	if(VProperty* p=prop_->find("view.common.node_style"))
    {
        if(p->value().toString() == "classic")
            nodeStyle_=ClassicNodeStyle;
        else
            nodeStyle_=BoxAndTextNodeStyle;
    }
    if(VProperty* p=prop_->find("view.common.node_gradient"))
    {
        useStateGrad_=p->value().toBool();
    }
        
    if(VProperty* p=prop_->find("view.tree.nodeRectRadius"))
	{
		nodeRectRad_=p->value().toInt();
	}
    if(VProperty* p=prop_->find("view.tree.nodeFont"))
    {
    	QFont newFont=p->value().value<QFont>();

    	if(font_ != newFont )
    	{
    		font_=newFont;
    		serverInfoFont_=font_;
    		serverNumFont_.setFamily(font_.family());
    		serverNumFont_.setPointSize(font_.pointSize());
    		suiteNumFont_.setFamily(font_.family());
    		suiteNumFont_.setPointSize(font_.pointSize());
    		abortedReasonFont_.setFamily(font_.family());
			abortedReasonFont_.setPointSize(font_.pointSize());
    		adjustIconSize();

    		Q_EMIT sizeHintChangedGlobal();
    	}
    }
    if(VProperty* p=prop_->find("view.tree.attributeFont"))
    {
    	QFont newFont=p->value().value<QFont>();

    	if(attrFont_ != newFont)
    	{
    		attrFont_=newFont;
    		Q_EMIT sizeHintChangedGlobal();
    	}
    }

	if(VProperty* p=prop_->find("view.tree.display_child_count"))
	{
		drawChildCount_=p->value().toBool();
	}
}

QSize TreeNodeViewDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	QSize size=QStyledItemDelegate::sizeHint(option,index);

	int attLineNum=0;
	if((attLineNum=index.data(AbstractNodeModel::AttributeLineRole).toInt()) > 0)
	{
		QFontMetrics fm(attrFont_);
		int h=fm.height();
		if(attLineNum==1)
			return QSize(size.width(),h+6);
		else
		{
			QStringList lst;
			for(int i=0; i < attLineNum; i++)
				lst << "1";

			return QSize(size.width(),fm.size(0,lst.join(QString('\n'))).height()+6);
		}
	}

	QFontMetrics fm(font_);
	int h=fm.height();
	return QSize(size.width(),h+8);
}


void TreeNodeViewDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
		           const QModelIndex& index) const
{
	//Background
	QStyleOptionViewItemV4 vopt(option);
	initStyleOption(&vopt, index);

	//Both the plastique and fusion styles render the tree expand indicator in the middle of the
	//indentation rectangle. This rectangle spans as far as the left hand side of the option rect that the
	//delegate renders into. For large indentations there can be a big gap between the indicator and
	//rendered item. To avoid it we expand the opt rect to the left to get it closer to the
	//indicator as much as possible.

	if(indentation_>0)
		vopt.rect.setLeft(vopt.rect.x()-indentation_/2 + 10);

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
		QRect fullRect=QRect(0,vopt.rect.y(),painter->device()->width(),vopt.rect.height());
		painter->fillRect(fullRect,lostConnectBgBrush_);
		QRect bandRect=QRect(0,vopt.rect.y(),5,vopt.rect.height());
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
	bool selected=option.state & QStyle::State_Selected;
    int offset=4;

	QFontMetrics fm(font_);
	int deltaH=(option.rect.height()-(fm.height()+4))/2;

	//The initial filled rect (we will adjust its  width)
	//QRect fillRect=option.rect.adjusted(offset,1,0,-2);
	QRect itemRect=option.rect.adjusted(2*offset,deltaH,0,-deltaH-1);
	if(option.state & QStyle::State_Selected)
		itemRect.adjust(0,0,0,0);

	int currentRight=itemRect.left();

	QRect stateRect;
	QRect fillRect;
	QRect halfRect;

	if(nodeStyle_ == BoxAndTextNodeStyle)
	{
		stateRect=QRect(itemRect.x(),itemRect.y(),itemRect.height(),itemRect.height());
		currentRight=stateRect.right()+offset;
	}
	else
	{
		fillRect=itemRect;
	}

	//The text rectangle
	QRect textRect=itemRect;
	textRect.setLeft(currentRight+offset);
	int textWidth=fm.width(text);
	textRect.setWidth(textWidth);

	//Adjust the filled rect width
	if(nodeStyle_ == ClassicNodeStyle)
	{
		fillRect.setRight(textRect.right()+offset);

		halfRect=fillRect;
		halfRect.setY(halfRect.center().y());
		halfRect.adjust(1,0,-1,-1);

		currentRight=fillRect.right();
	}
	else
	{
		currentRight=textRect.right();
	}


	//Icons area
	QList<QPixmap> pixLst;
	QList<QRect> pixRectLst;

	QVariant va=index.data(AbstractNodeModel::IconRole);
	if(va.type() == QVariant::List)
	{
		QVariantList lst=va.toList();
		int xp=currentRight+5;
		int yp=itemRect.center().y()-iconSize_/2;
		for(int i=0; i < lst.count(); i++)
		{
			int id=lst[i].toInt();
			if(id != -1)
			{
				pixLst << IconProvider::pixmap(id,iconSize_);
				pixRectLst << QRect(xp,yp,pixLst.back().width(),pixLst.back().height());
				xp+=pixLst.back().width()+iconGap_;
			}
		}

		if(!pixRectLst.isEmpty())
			currentRight=pixRectLst.back().right();
	}

	//The pixmap (optional)
	bool hasPix=false;

	QRect pixRect;

	/*
	bool hasPix=(index.data(AbstractNodeModel::IconRole).toString() == "d");

	if(hasPix)
	{
		pixRect = QRect(currentRight+fm.width('A'),
				fillRect.top()+(itemRect.height()-errPix_.height())/2,
				errPix_.width(),
				errPix_.height());

		hasPix=true;
		currentRight=pixRect.right();
	}
	*/

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
						itemRect.top()+(itemRect.height()-an->scaledSize().height())/2,
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

	//Draw state/node rect
	QColor bg=index.data(Qt::BackgroundRole).value<QColor>();
    QColor fg=index.data(Qt::ForegroundRole).value<QColor>();
    renderNodeCell(painter,bg,QColor(),fg,stateRect,fillRect,QRect(),textRect,text,selected);
    

	//Draw icons
	for(int i=0; i < pixLst.count(); i++)
	{
		painter->drawPixmap(pixRectLst[i],pixLst[i]);
	}

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
	
    bool selected=option.state & QStyle::State_Selected;
    int offset=4;

	QFontMetrics fm(font_);
	int deltaH=(option.rect.height()-(fm.height()+4))/2;

	//The initial filled rect (we will adjust its  width)
	QRect itemRect=option.rect.adjusted(2*offset,deltaH,0,-deltaH-1);
	if(option.state & QStyle::State_Selected)
		itemRect.adjust(0,0,0,-0);

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

	int currentRight=itemRect.left();

	QRect stateRect;
	QRect fillRect;
	QRect realRect;

	if(nodeStyle_ == BoxAndTextNodeStyle)
	{
		stateRect=QRect(itemRect.x(),itemRect.y(),itemRect.height(),itemRect.height());
		currentRight=stateRect.right();

		if(hasRealBg)
		{
			int realW=stateRect.width()/4;
			realRect=QRect(stateRect.right()-realW,stateRect.top(),realW+1,stateRect.height());
			currentRight=realRect.right()+2;
		}

		currentRight+=offset;
	}
	else
	{
		fillRect=itemRect;
	}


	//The text rectangle
	QRect textRect=itemRect;
	textRect.setLeft(currentRight+offset);
	int textWidth=fm.width(text);
	textRect.setWidth(textWidth);

	//Adjust the filled rect width
	if(nodeStyle_ == ClassicNodeStyle)
	{
		fillRect.setRight(textRect.right()+offset+1);

		currentRight=fillRect.right();

		if(hasRealBg)
		{
			realRect=QRect(fillRect.right()+1,fillRect.top(),6,fillRect.height());
			currentRight=realRect.right()+2;
		}
	}
	else
	{
		currentRight=textRect.right();
	}

	//Icons area
	QList<QPixmap> pixLst;
	QList<QRect> pixRectLst;

	QVariant va=index.data(AbstractNodeModel::IconRole);
	if(va.type() == QVariant::List)
	{
			QVariantList lst=va.toList();
			int xp=currentRight+5;
			int yp=itemRect.center().y()-iconSize_/2;
			for(int i=0; i < lst.count(); i++)
			{
				int id=lst[i].toInt();
				if(id != -1)
				{
					pixLst << IconProvider::pixmap(id,iconSize_);
					pixRectLst << QRect(xp,yp,pixLst.back().width(),pixLst.back().height());
					xp+=pixLst.back().width()+iconGap_;
				}
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
			QFontMetrics fmNum(suiteNumFont_);

			int numWidth=fmNum.width(numTxt);
			numRect = textRect;
			numRect.setLeft(currentRight+fmNum.width('A'));
			numRect.setWidth(numWidth);
			currentRight=numRect.right();
		}
	}

	//The aborted reason
	QRect reasonRect;
	QString reasonTxt=index.data(AbstractNodeModel::AbortedReasonRole).toString();
	if(reasonTxt.contains('\n'))
		reasonTxt=reasonTxt.split("\n").first();

	bool hasReason=(!reasonTxt.isEmpty());
	if(hasReason)
	{
		QFontMetrics fmReason(abortedReasonFont_);
		reasonRect = textRect;
		reasonRect.setLeft(currentRight+fmReason.width('A'));
		reasonTxt=fmReason.elidedText(reasonTxt,Qt::ElideRight,220);
		reasonRect.setWidth(fmReason.width(reasonTxt));
		currentRight=reasonRect.right();
	}

	//Define clipping
	int rightPos=currentRight+1;
	const bool setClipRect = rightPos > option.rect.right();
	if(setClipRect)
	{
		painter->save();
		painter->setClipRect(option.rect);
	}

	QColor fg=index.data(Qt::ForegroundRole).value<QColor>();
	renderNodeCell(painter,bg,realBg,fg,stateRect,fillRect,realRect,textRect,text,selected);

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

	//Draw aborted reason
	if(hasReason)
	{
		painter->setPen(bg);
		painter->setFont(abortedReasonFont_);
		painter->drawText(reasonRect,Qt::AlignLeft | Qt::AlignVCenter,reasonTxt);
	}

	if(setClipRect)
	{
		painter->restore();
	}
}

void TreeNodeViewDelegate::renderNodeCell(QPainter *painter,QColor bg,QColor realBg,QColor fg,
                                        QRect stateRect,QRect fillRect,QRect realRect,QRect textRect, QString text,
                                        bool selected) const
{    
    QColor bgLight=bg.lighter(lighter_);
    QColor borderCol=bg.darker(125);

    QBrush bgBrush;
    if(useStateGrad_)
    {
        grad_.setColorAt(0,bgLight);
        grad_.setColorAt(1,bg);
        bgBrush=QBrush(grad_);
    }
    else
        bgBrush=QBrush(bg);

    
    //Draw state/node rect
    if(nodeStyle_ == BoxAndTextNodeStyle)
    {
        painter->setBrush(bgBrush);
        painter->setPen(borderCol);
        painter->drawRect(stateRect);
    }
    else
    {
        painter->setBrush(bgBrush);
        painter->setPen(selected?nodeSelectPen_:QPen(borderCol));
        painter->drawRect(fillRect);
    }

    painter->setBrush(Qt::NoBrush);

    if(!realRect.isEmpty())
    {
        if(useStateGrad_)
        {
        	QColor realBgLight=realBg.lighter(lighter_);
            grad_.setColorAt(0,realBgLight);
            grad_.setColorAt(1,realBg);
            bgBrush=QBrush(grad_);
        }
        else
            bgBrush=QBrush(realBg);

        painter->fillRect(realRect,bgBrush);

        QColor realBorderCol=realBg.darker(125);
        if(nodeStyle_ == BoxAndTextNodeStyle)
            painter->setPen(realBorderCol);
        else
            painter->setPen(selected?nodeSelectPen_:QPen(realBorderCol));

        painter->drawRect(realRect);
    }
    //Draw text
    painter->setPen(fg);
    painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

    //selection
    if(nodeStyle_ == BoxAndTextNodeStyle && selected)
    {
        painter->setPen(nodeSelectPen_);
        QRect selRect=stateRect;
        selRect.setRight(textRect.right()+2);
        painter->drawRect(selRect);
    }
}
