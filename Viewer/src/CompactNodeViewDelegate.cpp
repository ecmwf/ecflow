//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "CompactNodeViewDelegate.hpp"

#include <QApplication>
#include <QDebug>
#include <QImageReader>
#include <QLinearGradient>
#include <QPainter>

#include "AbstractNodeModel.hpp"
#include "Animation.hpp"
#include "IconProvider.hpp"
#include "PropertyMapper.hpp"
#include "ServerHandler.hpp"
#include "TreeNodeModel.hpp"
#include "UiLog.hpp"

class CompactNodeDelegateBox : public NodeDelegateBox
{
public:

    CompactNodeDelegateBox() : textTopCorrection(0), textBottomCorrection(0)
     {
         topMargin=2;
         bottomMargin=2;
         leftMargin=1;
         rightMargin=2;
         topPadding=0;
         bottomPadding=0;
         leftPadding=2;
         rightPadding=2;
     }

     int realFontHeight;
     int textTopCorrection;
     int textBottomCorrection;

     void adjust(const QFont& f)
     {
         FontMetrics fm(f);
         realFontHeight=fm.realHeight();
         textTopCorrection=fm.topPaddingForCentre();
         textBottomCorrection=fm.bottomPaddingForCentre();
         fontHeight=fm.height();

         if(textTopCorrection > 1)
         {
             textTopCorrection-=2;
             realFontHeight+=2;
         }

         height=realFontHeight+topPadding+bottomPadding;
         fullHeight=height+topMargin+bottomMargin;
         sizeHintCache=QSize(100,fullHeight);
         spacing=fm.width('A')*3/4;

         int h=static_cast<int>(static_cast<float>(fm.height())*0.7);
         iconSize=h;
         if(iconSize % 2 == 1)
             iconSize+=1;

         iconGap=1;
         if(iconSize > 16)
             iconGap=2;

         iconPreGap=fm.width('A')/2;
     }

     QRect adjustTextRect(const QRect& rIn) const
     {
         //Q_ASSERT(rIn.height() == fontHeight);
         QRect r=rIn;
         r.setY(r.y()-textTopCorrection);
         r.setHeight(fontHeight);
         return r;
     }

     QRect adjustSelectionRect(const QRect& optRect) const {
         QRect r=optRect;
         return r;
     }
};

class CompactAttrDelegateBox : public AttrDelegateBox
{
public:
    CompactAttrDelegateBox() : textTopCorrection(0), textBottomCorrection(0)
     {
         topMargin=2;
         bottomMargin=2;
         leftMargin=1;
         rightMargin=2;
         topPadding=0;
         bottomPadding=0;
         leftPadding=1;
         rightPadding=2;
     }

     int realFontHeight;
     int textTopCorrection;
     int textBottomCorrection;

     void adjust(const QFont& f)
     {
         FontMetrics fm(f);
         realFontHeight=fm.realHeight();
         textTopCorrection=fm.topPaddingForCentre();
         textBottomCorrection=fm.bottomPaddingForCentre();
         fontHeight=fm.height();

         height=realFontHeight+topPadding+bottomPadding;
         fullHeight=height+topMargin+bottomMargin;
         sizeHintCache=QSize(100,fullHeight);
         spacing=fm.width('A')*3/4;
     }

     QRect adjustTextRect(const QRect& rIn) const
     {
         QRect r=rIn;
         r.setY(r.y()-textTopCorrection+1);
         r.setHeight(fontHeight);
         return r;
     }

     QRect adjustSelectionRect(const QRect& optRect) const {
         QRect r=optRect;
         return r;
     }
};

CompactNodeViewDelegate::CompactNodeViewDelegate(TreeNodeModel* model,QWidget *parent) :
    TreeNodeViewDelegateBase(model,parent)
{
    nodeBox_=new CompactNodeDelegateBox;
    attrBox_=new CompactAttrDelegateBox;

    nodeBox_->adjust(font_);
    attrBox_->adjust(attrFont_);

    updateSettings();
}

CompactNodeViewDelegate::~CompactNodeViewDelegate()
{
}

bool CompactNodeViewDelegate::isSingleHeight(int h) const
{
    return (h==nodeBox_->fullHeight || h == attrBox_->fullHeight);
}

QSize CompactNodeViewDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex & index ) const
{
#if 0
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

            return QSize(100,fm.size(0,lst.join(QString('\n'))).height()+6);
        }
    }

    return nodeBox_->sizeHintCache;
#endif

    return nodeBox_->sizeHintCache;
}

//This has to be extremely fast
void CompactNodeViewDelegate::sizeHint(const QModelIndex& index,int& w,int& h) const
{
    QVariant tVar=index.data(Qt::DisplayRole);

    h=nodeBox_->fullHeight;

    //For nodes we compute the exact size of visual rect
    if(tVar.type() == QVariant::String)
    {
        QString text=index.data(Qt::DisplayRole).toString();
        if(index.data(AbstractNodeModel::ServerRole).toInt() ==0)
        {
            widthHintServer(index,w,text);
        }
        else
        {
            w=nodeWidth(index,text);
        }
    }
    //For attributes we do not need the exact width since they do not have children so
    //there is nothing on their right in the view. We compute their proper size when
    //they are first rendered. However the exact height must be known at this stage!
    else if(tVar.type() == QVariant::StringList)
    {
        //Each attribute has this height except the multiline labels
        h=attrBox_->fullHeight;

        //It is a big enough hint for the width.
        w=300;

        //For multiline labels we need to cimpute the height
        int attLineNum=0;
        if((attLineNum=index.data(AbstractNodeModel::AttributeLineRole).toInt()) > 1)
        {
            h=labelHeight(attLineNum);
        }
    }
}

int CompactNodeViewDelegate::paintItem(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const
{
    int width=0;

    //Background
    QStyleOptionViewItem vopt(option);
    initStyleOption(&vopt, index);

    const QStyle *style = vopt.widget ? vopt.widget->style() : QApplication::style();
    const QWidget* widget = vopt.widget;

    //Save painter state
    painter->save();

    if(index.data(AbstractNodeModel::ConnectionRole).toInt() == 0)
    {
        QRect fullRect=QRect(0,vopt.rect.y(),painter->device()->width(),vopt.rect.height());
        painter->fillRect(fullRect,lostConnectBgBrush_);
        QRect bandRect=QRect(0,vopt.rect.y(),5,vopt.rect.height());
        painter->fillRect(bandRect,lostConnectBandBrush_);
    }

    QVariant tVar=index.data(Qt::DisplayRole);
    painter->setFont(font_);

    if(tVar.type() == QVariant::String)
    {
        QString text=index.data(Qt::DisplayRole).toString();
        if(index.data(AbstractNodeModel::ServerRole).toInt() ==0)
        {
            width=renderServer(painter,index,vopt,text);
        }
        else
        {
            width=renderNode(painter,index,vopt,text);
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
                width=(this->*a)(painter,lst,vopt);
            }
            //if(width==0)
            //    width=300;
        }

    }

    painter->restore();

    return width;

    //else
    //	QStyledItemDelegate::paint(painter,option,index);
}
