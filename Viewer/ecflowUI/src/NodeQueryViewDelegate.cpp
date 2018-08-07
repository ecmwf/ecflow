//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeQueryViewDelegate.hpp"

#include <QtGlobal>
#include <QApplication>
#include <QDebug>
#include <QImageReader>
#include <QPainter>

#include <QStyleOptionViewItem>

#include "AbstractNodeModel.hpp"
#include "Animation.hpp"
#include "IconProvider.hpp"
#include "ModelColumn.hpp"
#include "PropertyMapper.hpp"

static std::vector<std::string> propVec;

//Define node renderer properties
struct QueryNodeDelegateBox : public NodeDelegateBox
{
    QueryNodeDelegateBox() {
        topMargin=2;
        bottomMargin=2;
        leftMargin=3;
        rightMargin=0;
        topPadding=0;
        bottomPadding=0;
        leftPadding=2;
        rightPadding=1;
      }
};

//Define attribute renderer properties
struct QueryAttrDelegateBox : public AttrDelegateBox
{
    QueryAttrDelegateBox() {
        topMargin=2;
        bottomMargin=2;
        leftMargin=1;
        rightMargin=0;
        topPadding=0;
        bottomPadding=0;
        leftPadding=0;
        rightPadding=0;
      }
};

NodeQueryViewDelegate::NodeQueryViewDelegate(QWidget *parent)
{
    nodeBox_=new QueryNodeDelegateBox;
    attrBox_=new QueryAttrDelegateBox;

    nodeBox_->adjust(font_);
    attrBox_->adjust(attrFont_);

    borderPen_=QPen(QColor(230,230,230));

	columns_=ModelColumn::def("query_columns");

    //Property
    if(propVec.empty())
    {
        //Base settings
        addBaseSettings(propVec);
    }

    prop_=new PropertyMapper(propVec,this);

    updateSettings();
}

NodeQueryViewDelegate::~NodeQueryViewDelegate()
= default;

void NodeQueryViewDelegate::updateSettings()
{
    //Update the settings handled by the base class
    updateBaseSettings();
}

QSize NodeQueryViewDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	QSize size=QStyledItemDelegate::sizeHint(option,index);
    return {size.width(),nodeBox_->sizeHintCache.height()};
}

void NodeQueryViewDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const
{
    //Background
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QStyleOptionViewItem vopt(option);
#else
    QStyleOptionViewItemV4 vopt(option);
#endif

    initStyleOption(&vopt, index);

    const QStyle *style = vopt.widget ? vopt.widget->style() : QApplication::style();
    const QWidget* widget = vopt.widget;

    //Save painter state
    painter->save();

    QString id=columns_->id(index.column());

    if(id == "path")
    {
    	QString text=index.data(Qt::DisplayRole).toString();
    	renderNode(painter,index,vopt,text);
    }
    else if(id == "status")
    {
    	 renderStatus(painter,index,vopt);
    }

    //Render attributes
    else if(id == "attribute")
    {
    	QVariant va=index.data(Qt::DisplayRole);
    	if(va.type() == QVariant::StringList)
    	{
    		QStringList lst=va.toStringList();
    		if(lst.count() > 0)
    		{
    			QMap<QString,AttributeRendererProc>::const_iterator it=attrRenderers_.find(lst.at(0));
    			if(it != attrRenderers_.end())
    			{
                    QSize size;
                    AttributeRendererProc a=it.value();
                    (this->*a)(painter,lst,vopt,size);
    			}
    		}
    	}
	}

    //rest of the columns
    else
    {
    	QString text=index.data(Qt::DisplayRole).toString();
        QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt,widget);
    	painter->setPen(Qt::black);

    	int rightPos=textRect.right()+1;
    	const bool setClipRect = rightPos > option.rect.right();
    	if(setClipRect)
    	{
    	   painter->save();
    	   painter->setClipRect(option.rect);
    	}

    	/*QVariant alg=index.data(Qt::TextAlignmentRole);
    	if(alg.isValid())
    		painter->drawText(textRect,alg.value<Qt::Alignment>(),text);
    	else*/
    		painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

        if(setClipRect)
    	{
    	   painter->restore();
    	}

    }

    //Render the horizontal border for rows. We only render the top border line.
    //With this technique we miss the bottom border line of the last row!!!
    //QRect fullRect=QRect(0,option.rect.y(),painter->device()->width(),option.rect.height());
    QRect bgRect=option.rect;
    painter->setPen(borderPen_);
    painter->drawLine(bgRect.topLeft(),bgRect.topRight());

    painter->restore();
}

void NodeQueryViewDelegate::renderNode(QPainter *painter,const QModelIndex& index,
                                    const QStyleOptionViewItem& option,QString text) const
{
	bool selected=option.state & QStyle::State_Selected;
    QFontMetrics fm(font_);

    QRect itemRect=option.rect.adjusted(nodeBox_->leftMargin,nodeBox_->topMargin,0,-nodeBox_->bottomMargin);

	//The text rectangle
    QRect textRect = itemRect;

	int textWidth=fm.width(text);
    textRect.setWidth(textWidth+nodeBox_->leftPadding+nodeBox_->rightPadding);

    int currentRight=textRect.x()+textRect.width();

    QList<QPixmap> pixLst;
    QList<QRect> pixRectLst;

    QVariant va=index.data(AbstractNodeModel::IconRole);
    if(va.type() == QVariant::List)
    {
        QVariantList lst=va.toList();
        if(lst.count() >0)
        {
            int xp=currentRight+nodeBox_->iconPreGap;
            int yp=itemRect.center().y()+1-nodeBox_->iconSize/2;
            for(int i=0; i < lst.count(); i++)
            {
                int id=lst[i].toInt();
                if(id != -1)
                {
                    pixLst << IconProvider::pixmap(id,nodeBox_->iconSize);
                    pixRectLst << QRect(xp,yp,nodeBox_->iconSize,nodeBox_->iconSize);
                    xp+=nodeBox_->iconSize+nodeBox_->iconGap;
                }
            }

            if(!pixLst.isEmpty())
            {
                currentRight=xp-nodeBox_->iconGap;
            }
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

	//Draw text
	QColor fg=index.data(Qt::ForegroundRole).value<QColor>();
	painter->setPen(fg);
    painter->drawText(textRect,Qt::AlignHCenter | Qt::AlignVCenter,text);

    if(selected)
    {
        QRect sr=textRect;
        sr.setX(option.rect.x()+nodeBox_->leftMargin);
        renderSelectionRect(painter,sr);
    }

	//Draw icons
	for(int i=0; i < pixLst.count(); i++)
	{
		painter->drawPixmap(pixRectLst[i],pixLst[i]);
	}

	if(setClipRect)
	{
		painter->restore();
	}
}
