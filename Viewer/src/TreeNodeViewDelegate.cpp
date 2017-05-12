//============================================================================
// Copyright 2009-2017 ECMWF.
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
#include "FontMetrics.hpp"
#include "IconProvider.hpp"
#include "PropertyMapper.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"

#if 0
static std::vector<std::string> propVec;

static QColor typeFgColourClassic=QColor(Qt::white);
static QColor typeBgColourClassic=QColor(150,150,150);
static QColor childCountColour=QColor(90,91,92);
#endif

struct TreeNodeDelegateBox : public NodeDelegateBox
{
    TreeNodeDelegateBox() {
        topMargin=2;
        bottomMargin=2;
        leftMargin=8;
        rightMargin=2;
        topPadding=0;
        bottomPadding=0;
        leftPadding=2;
        rightPadding=2;
      }
};

struct TreeAttrDelegateBox : public AttrDelegateBox
{
    TreeAttrDelegateBox() {
        topMargin=2;
        bottomMargin=2;
        leftMargin=8;
        rightMargin=2;
        topPadding=0;
        bottomPadding=0;
        leftPadding=2;
        rightPadding=0;
      }
};

TreeNodeViewDelegate::TreeNodeViewDelegate(TreeNodeModel* model,QWidget *parent) :
    TreeNodeViewDelegateBase(model,parent)
{
    nodeBox_=new TreeNodeDelegateBox;
    attrBox_=new TreeAttrDelegateBox;

    nodeBox_->adjust(font_);
    attrBox_->adjust(attrFont_);

    typeFont_=font_;
    typeFont_.setBold(true);
    typeFont_.setPointSize(font_.pointSize()-1);

    updateSettings();
}

TreeNodeViewDelegate::~TreeNodeViewDelegate()
{
}

QSize TreeNodeViewDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex & index ) const
{
    //QSize size=QStyledItemDelegate::sizeHint(option,index);
    //QSize size(100,fontHeight_+8);

	int attLineNum=0;
	if((attLineNum=index.data(AbstractNodeModel::AttributeLineRole).toInt()) > 0)
	{
		if(attLineNum==1)
            return attrBox_->sizeHintCache;
		else
		{
            QFontMetrics fm(attrFont_);
            QStringList lst;
			for(int i=0; i < attLineNum; i++)
				lst << "1";

            //return QSize(100,fm.size(0,lst.join(QString('\n'))).height()+6);
            return QSize(100,labelHeight(attLineNum));

		}
	}

    return nodeBox_->sizeHintCache;
}

void TreeNodeViewDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
		           const QModelIndex& index) const
{
	//Background
    QStyleOptionViewItem vopt(option);
	initStyleOption(&vopt, index);

	//Both the plastique and fusion styles render the tree expand indicator in the middle of the
	//indentation rectangle. This rectangle spans as far as the left hand side of the option rect that the
	//delegate renders into. For large indentations there can be a big gap between the indicator and
	//rendered item. To avoid it we expand the opt rect to the left to get it closer to the
	//indicator as much as possible.

	if(indentation_>0)
        //vopt.rect.setLeft(vopt.rect.x()-indentation_/2 + 10);
        vopt.rect.setLeft(vopt.rect.x()-indentation_/2 + 5);

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

	painter->restore();

	//else
	//	QStyledItemDelegate::paint(painter,option,index);
}
