//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TableNodeViewDelegate.hpp"

#include <QApplication>
#include <QDebug>
#include <QImageReader>
#include <QPainter>

#include "AbstractNodeModel.hpp"
#include "Animation.hpp"
#include "ModelColumn.hpp"
#include "PropertyMapper.hpp"

static std::vector<std::string> propVec;

TableNodeViewDelegate::TableNodeViewDelegate(QWidget *parent)
{
    columns_=ModelColumn::def("table_columns");

    //Property
    if(propVec.empty())
    {
        propVec.push_back("view.tree.font");
        propVec.push_back("view.tree.displayChildCount");
    }

    prop_=new PropertyMapper(propVec,this);

    updateSettings();

    /*attrRenderers_["meter"]=&TableNodeViewDelegate::renderMeter;
    attrRenderers_["label"]=&TableNodeViewDelegate::renderLabel;
    attrRenderers_["event"]=&TableNodeViewDelegate::renderEvent;
    attrRenderers_["var"]=&TableNodeViewDelegate::renderVar;
    attrRenderers_["genvar"]=&TableNodeViewDelegate::renderGenvar;
    attrRenderers_["limit"]=&TableNodeViewDelegate::renderLimit;
    attrRenderers_["limiter"]=&TableNodeViewDelegate::renderLimiter;
    attrRenderers_["trigger"]=&TableNodeViewDelegate::renderTrigger;
    attrRenderers_["time"]=&TableNodeViewDelegate::renderTime;
    attrRenderers_["date"]=&TableNodeViewDelegate::renderDate;
    attrRenderers_["repeat"]=&TableNodeViewDelegate::renderRepeat;
    attrRenderers_["late"]=&TableNodeViewDelegate::renderLate;*/
}

TableNodeViewDelegate::~TableNodeViewDelegate()
{
}

void TableNodeViewDelegate::updateSettings()
{
   /* if(VProperty* p=prop_->find("view.tree.nodeRectRadius"))
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
    }*/
}

/*QSize TableNodeViewDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QSize size=QStyledItemDelegate::sizeHint(option,index);

    QFontMetrics fm(font_);
    int h=fm.height();
    return QSize(size.width(),h+8);
}*/


void TableNodeViewDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
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

    //Different background for lost connection?
   /* if(index.data(AbstractNodeModel::ConnectionRole).toInt() == 0)
    {
        QRect fullRect=QRect(0,option.rect.y(),painter->device()->width(),option.rect.height());
        painter->fillRect(fullRect,lostConnectBgBrush_);
        QRect bandRect=QRect(0,option.rect.y(),5,option.rect.height());
        painter->fillRect(bandRect,lostConnectBandBrush_);

    }*/

    QString id=columns_->id(index.column());

    //First column (nodes)
    if(id == "icon")
    {
        renderIcons(painter,index,vopt);

        /*QVariant tVar=index.data(Qt::DisplayRole);
        QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt, widget);

        painter->setFont(font_);

        if(tVar.type() == QVariant::String)
        {
            QString text=index.data(Qt::DisplayRole).toString();
            renderNode(painter,index,vopt,text);


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

        }*/
    }
    /*//rest of the columns
    else if(index.column() < 3)
    {
        QString text=index.data(Qt::DisplayRole).toString();
        QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt, widget);
        painter->setPen(Qt::black);
        painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

    }*/

    else
        QStyledItemDelegate::paint(painter,option,index);


    painter->restore();
}

void TableNodeViewDelegate::renderIcons(QPainter *painter,const QModelIndex& index,
                                    const QStyleOptionViewItemV4& option) const
{
    int offset=4;

    QFontMetrics fm(font_);
    int deltaH=(option.rect.height()-(fm.height()+4))/2;

    //The initial filled rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(offset,deltaH,0,-deltaH-1);
    if(option.state & QStyle::State_Selected)
        fillRect.adjust(0,0,0,-0);

    //Adjust the filled rect width
    //fillRect.setRight(textRect.right()+offset);

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


    //Define clipping
    int rightPos=currentRight+1;
    const bool setClipRect = rightPos > option.rect.right();
    if(setClipRect)
    {
        painter->save();
        painter->setClipRect(option.rect);
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

//========================================================
// data is encoded as a QStringList as follows:
// "meter" name  value min  max colChange
//========================================================

void TableNodeViewDelegate::renderMeter(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
    if(data.count() != 6)
            return;

    //The data
    int	val=data.at(2).toInt();
    int	min=data.at(3).toInt();
    int	max=data.at(4).toInt();
    //bool colChange=data.at(5).toInt();
    QString name=data.at(1) + ":";
    QString valStr=data.at(2) + " (" +
            QString::number(100.*static_cast<float>(val)/static_cast<float>(max-min)) + "%)";

    int offset=2;
    //int gap=5;

    //The border rect (we will adjust its  width)
    QRect fillRect=option.rect.adjusted(offset,1,0,-1);
    if(option.state & QStyle::State_Selected)
            fillRect.adjust(0,1,0,-1);

    //The status rectangle
    QRect stRect=fillRect.adjusted(offset,2,0,-2);
    stRect.setWidth(50);

    //The text rectangle
    QFont nameFont=font_;
    nameFont.setBold(true);
    QFontMetrics fm(nameFont);
    int nameWidth=fm.width(name);
    QRect nameRect = stRect;
    nameRect.setLeft(stRect.right()+fm.width('A'));
    nameRect.setWidth(nameWidth);

    //The value rectangle
    QFont valFont=font_;
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

void TableNodeViewDelegate::renderLabel(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
    QFont nameFont=font_;
    nameFont.setBold(true);
    QFontMetrics fm(nameFont);
    int nameWidth=fm.width(name);
    QRect nameRect = fillRect.adjusted(offset,0,0,0);
    nameRect.setWidth(nameWidth);

    //The value rectangle
    QFont valFont=font_;
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

void TableNodeViewDelegate::renderEvent(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
    QFont font=font_;
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




void TableNodeViewDelegate::renderVar(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
    QFont font=font_;
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

void TableNodeViewDelegate::renderGenvar(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
    QFont font=font_;
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

void TableNodeViewDelegate::renderLimit(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
{
    if(data.count() != 4)
            return;

    //The data
    int	val=data.at(2).toInt();
    int	max=data.at(3).toInt();
    QString name=data.at(1) + ":";
    QString valStr=QString::number(val) + "/" + QString::number(max);
    bool drawItem=(max < 21);

    QFontMetrics fm(font_);
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
    QFont nameFont=font_;
    nameFont.setBold(true);
    fm=QFontMetrics(nameFont);
    int nameWidth=fm.width(name);
    QRect nameRect = fillRect.adjusted(0,2,0,-2);
    nameRect.setLeft(fillRect.left()+gap);
    nameRect.setWidth(nameWidth+offset);

    //The value rectangle
    QFont valFont=font_;
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

void TableNodeViewDelegate::renderLimiter(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
    QFont nameFont=font_;
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

void TableNodeViewDelegate::renderTrigger(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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

void TableNodeViewDelegate::renderTime(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
    QFont nameFont=font_;
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

void TableNodeViewDelegate::renderDate(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
    QFont nameFont=font_;
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

void TableNodeViewDelegate::renderRepeat(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
    QFont nameFont=font_;
    nameFont.setBold(true);
    QFontMetrics fm(nameFont);
    int nameWidth=fm.width(name);
    QRect nameRect = fillRect.adjusted(offset,0,0,0);
    nameRect.setWidth(nameWidth);

    //The value rectangle
    QFont valFont=font_;
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

void TableNodeViewDelegate::renderLate(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const
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
