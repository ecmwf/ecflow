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

CompactNodeViewDelegate::CompactNodeViewDelegate(TreeNodeModel* model,QWidget *parent) :
    TreeNodeViewDelegate(parent),
    model_(model)
{
}

CompactNodeViewDelegate::~CompactNodeViewDelegate()
{
}

bool CompactNodeViewDelegate::isSingleHeight(int h) const
{
    return (h==nodeBox_.fullHeight || h == attrBox_.fullHeight);
}

QSize CompactNodeViewDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex & index ) const
{
    //QSize size=QStyledItemDelegate::sizeHint(option,index);
    //QSize size(100,fontHeight_+8);

    int attLineNum=0;
    if((attLineNum=index.data(AbstractNodeModel::AttributeLineRole).toInt()) > 0)
    {
        if(attLineNum==1)
            return attrBox_.sizeHintCache;
        else
        {
            QFontMetrics fm(attrFont_);
            QStringList lst;
            for(int i=0; i < attLineNum; i++)
                lst << "1";

            return QSize(100,fm.size(0,lst.join(QString('\n'))).height()+6);
        }
    }

    return nodeBox_.sizeHintCache;
}

void CompactNodeViewDelegate::sizeHint(const QModelIndex& index,int& w,int& h) const
{
    QVariant tVar=index.data(Qt::DisplayRole);

    h=nodeBox_.fullHeight;

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
    //Render attributes
    else if(tVar.type() == QVariant::StringList)
    {
        h=attrBox_.fullHeight;

        /*QStringList lst=tVar.toStringList();
        if(lst.count() > 0)
        {
            QMap<QString,AttributeRendererProc>::const_iterator it=attrRenderers_.find(lst.at(0));
            if(it != attrRenderers_.end())
            {
                AttributeRendererProc a=it.value();
                (this->*a)(painter,lst,vopt);
            }
        }*/
        w=300;
        //h=20;
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

void CompactNodeViewDelegate::widthHintServer(const QModelIndex& index,int& itemWidth, QString text) const
{
    ServerHandler* server=static_cast<ServerHandler*>(index.data(AbstractNodeModel::ServerPointerRole).value<void*>());
    Q_ASSERT(server);

    int offset=nodeBox_.leftMargin;
    QFontMetrics fm(font_);

    //The initial filled rect. We only care of the width
    QRect itemRect(offset,0,10,10);
    int currentRight=itemRect.left();

    NodeShape stateShape;

    NodeText nodeText;
    nodeText.text_=text;
    int textWidth=fm.width(text);

    if(nodeStyle_ == BoxAndTextNodeStyle)
    {
        currentRight+=itemRect.height()+offset+textWidth;
    }
    else
    {
        currentRight+=textWidth+nodeBox_.leftPadding+nodeBox_.rightPadding;
    }

    //Icons area
    int iconNum=model_->iconNum(server->vRoot());
    if(iconNum > 0)
    {
        currentRight+=4+iconNum*iconSize_+(iconNum-1)*iconGap_;
    }

    //The info rectangle (optional)
    QString infoTxt=index.data(AbstractNodeModel::InfoRole).toString();
    bool hasInfo=(infoTxt.isEmpty() == false);

    if(hasInfo)
    {
        fm=QFontMetrics(serverInfoFont_);
        int infoWidth=fm.width(infoTxt);
        currentRight+=fm.width('A')+infoWidth;
    }

    //The load icon (optional)
    QRect loadRect;
    bool hasLoad=index.data(AbstractNodeModel::LoadRole).toBool();
    Animation* an=0;

    //Update load animation
    if(hasLoad)
    {
        an=animation_->find(Animation::ServerLoadType,true);
        currentRight=fm.width('A')+an->scaledSize().width();
    }
    //Stops load animation
    else
    {
        //if((an=animation_->find(Animation::ServerLoadType,false)) != NULL)
        //    an->removeTarget(server->vRoot());
    }

    //The node number (optional)
    QString numTxt;
    bool hasNum=false;

    if(drawChildCount_)
    {
        QVariant va=index.data(AbstractNodeModel::NodeNumRole);
        hasNum=(va.isNull() == false);
        if(hasNum)
        {
            numTxt="(" + QString::number(va.toInt()) + ")";
            QFontMetrics fmNum(serverNumFont_);
            int numWidth=fmNum.width(numTxt);
            currentRight+=fmNum.width('A')/2+numWidth;
        }
    }

    itemWidth=currentRight+1;
}

int CompactNodeViewDelegate::nodeWidth(const QModelIndex& index,QString text) const
{
    VNode* node=static_cast<VNode*>(index.data(AbstractNodeModel::NodePointerRole).value<void*>());
    Q_ASSERT(node);

    int offset=nodeBox_.leftMargin;
    QFontMetrics fm(font_);

    //The initial filled rect. We only care about the width!
    QRect itemRect(offset,0,10,10);
    int currentRight=itemRect.left();

    bool hasRealBg=node->isSuspended();

    //Node type
    QFontMetrics fmType(typeFont_);
    int typeWidth=fmType.width(" S");

    //The text rectangle
    int textWidth=fm.width(text);

    if(nodeStyle_ == BoxAndTextNodeStyle)
    {
        int realW=itemRect.height()/4;

        //state box
        currentRight+=itemRect.height();
        if(hasRealBg)
        {
            currentRight+=1;
        }
        currentRight+=2;

        //node name
        currentRight+=textWidth+nodeBox_.leftPadding+nodeBox_.rightPadding;
    }
    //Classic style
    else
    {
        //node type box
        if(drawNodeType_)
        {
            currentRight+=typeWidth+2;
        }

        //node name + state
        currentRight+=textWidth+nodeBox_.leftPadding+nodeBox_.rightPadding;

        if(hasRealBg)
        {
            int realW=6;
            currentRight+=realW+1;
        }
    }

    //Icons area
    int pixNum=model_->iconNum(node);
    if(pixNum > 0)
    {
        currentRight+=4+pixNum*iconSize_ + (pixNum-1)*iconGap_;
    }

    //The node number (optional)
    if(drawChildCount_)
    {
        if(node->isTopLevel())
        {
            QVariant va=index.data(AbstractNodeModel::NodeNumRole);
            if(va.isNull() == false)
            {
                QString numTxt="(" + va.toString() + ")";
                QFontMetrics fmNum(suiteNumFont_);
                int numWidth=fmNum.width(numTxt);
                currentRight+=fmNum.width('A')/2+numWidth;
            }
        }
    }

    //The aborted reason
    if(node->isAborted())
    {
        QString reasonTxt=QString::fromStdString(node->abortedReason());
        if(reasonTxt.contains('\n'))
            reasonTxt=reasonTxt.split("\n").first();

        bool hasReason=(!reasonTxt.isEmpty());
        if(hasReason)
        {
            QFontMetrics fmReason(abortedReasonFont_);
            reasonTxt=fmReason.elidedText(reasonTxt,Qt::ElideRight,220);
            currentRight+=fmReason.width('A')/2+fmReason.width(reasonTxt);
        }
    }

    return currentRight+1;
}
