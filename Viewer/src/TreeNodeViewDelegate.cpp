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
#include "Palette.hpp"
#include "PropertyMapper.hpp"
#include "ServerHandler.hpp"
#include "TreeNodeModel.hpp"
#include "UiLog.hpp"

static std::vector<std::string> propVec;

static QColor typeFgColourClassic=QColor(Qt::white);
static QColor typeBgColourClassic=QColor(150,150,150);
static QColor childCountColour=QColor(90,91,92);


struct NodeShape
{
    QColor col_;
    QPolygon shape_;
};

struct NodeText
{
    QColor fgCol_;
    QColor bgCol_;
    QRect br_;
    QString text_;
};

struct ServerUpdateData
{
    QRect br_;
    int prevTime_;
    int nextTime_;
    QString prevText_;
    QString nextText_;
    float prog_;
};


class TreeNodeDelegateBox : public NodeDelegateBox
{
public:

    TreeNodeDelegateBox() : textTopCorrection(0), textBottomCorrection(0)
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

class TreeAttrDelegateBox : public AttrDelegateBox
{
public:
    TreeAttrDelegateBox() : textTopCorrection(0), textBottomCorrection(0)
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

     QRect adjustTextBgRect(const QRect& rIn) const
     {
         QRect r=rIn;
         return r.adjusted(0,-1,0,1);
     }

     QRect adjustSelectionRect(const QRect& optRect) const {
         QRect r=optRect;
         return r.adjusted(0,-selectRm.topOffset(),0,-selectRm.bottomOffset());
     }

     QRect adjustSelectionRectNonOpt(const QRect& optRect) const {
         return adjustSelectionRect(optRect);
     }
};



TreeNodeViewDelegate::TreeNodeViewDelegate(TreeNodeModel* model,QWidget *parent) :
    nodeRectRad_(0),
    drawChildCount_(true),
    nodeStyle_(ClassicNodeStyle),
    drawNodeType_(true),
    bgCol_(Qt::white),
    model_(model)
{

    drawAttrSelectionRect_=true;

    attrFont_=font_;
    attrFont_.setPointSize(8);
    serverInfoFont_=font_;
    suiteNumFont_=font_;
    abortedReasonFont_=font_;
    abortedReasonFont_.setBold(true);
    typeFont_=font_;
    typeFont_.setBold(true);
    typeFont_.setPointSize(font_.pointSize()-1);

    //Property
    if(propVec.empty())
    {
        propVec.push_back("view.tree.nodeFont");
        propVec.push_back("view.tree.attributeFont");
        propVec.push_back("view.tree.display_child_count");
        propVec.push_back("view.tree.displayNodeType");
        propVec.push_back("view.tree.background");
        propVec.push_back("view.common.node_style");

        //Base settings
        addBaseSettings(propVec);
    }

    prop_=new PropertyMapper(propVec,this);

    //WARNING: updateSettings() cannot be called from here because
    //nodeBox_ and attrBox_ are only created in the derived classes. So it muse be
    //called from those derived classes.

    //The parent must be the view!!!
    animation_=new AnimationHandler(parent);

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
    delete animation_;
}

void TreeNodeViewDelegate::updateSettings()
{
    Q_ASSERT(nodeBox_);
    Q_ASSERT(attrBox_);

    if(VProperty* p=prop_->find("view.common.node_style"))
    {
        if(p->value().toString() == "classic")
            nodeStyle_=ClassicNodeStyle;
        else
            nodeStyle_=BoxAndTextNodeStyle;
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
            nodeBox_->adjust(font_);
            serverInfoFont_=font_;
            serverNumFont_.setFamily(font_.family());
            serverNumFont_.setPointSize(font_.pointSize()-1);
            suiteNumFont_.setFamily(font_.family());
            suiteNumFont_.setPointSize(font_.pointSize()-1);
            abortedReasonFont_.setFamily(font_.family());
            abortedReasonFont_.setPointSize(font_.pointSize());
            typeFont_.setFamily(font_.family());
            typeFont_.setPointSize(font_.pointSize()-1);

            Q_EMIT sizeHintChangedGlobal();
        }
    }
    if(VProperty* p=prop_->find("view.tree.attributeFont"))
    {
        QFont newFont=p->value().value<QFont>();

        if(attrFont_ != newFont)
        {
            attrFont_=newFont;
            attrBox_->adjust(attrFont_);
            Q_EMIT sizeHintChangedGlobal();
        }
    }

    if(VProperty* p=prop_->find("view.tree.display_child_count"))
    {
        drawChildCount_=p->value().toBool();
    }

    if(VProperty* p=prop_->find("view.tree.displayNodeType"))
    {
        drawNodeType_=p->value().toBool();
    }

    if(VProperty* p=prop_->find("view.tree.background"))
    {
        bgCol_=p->value().value<QColor>();
    }

    //Update the settings handled by the base class
    updateBaseSettings();
}

bool TreeNodeViewDelegate::isSingleHeight(int h) const
{
    return (h==nodeBox_->fullHeight || h == attrBox_->fullHeight);
}

QSize TreeNodeViewDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex & index ) const
{
    return nodeBox_->sizeHintCache;
}

//This has to be extremely fast
void  TreeNodeViewDelegate::sizeHint(const QModelIndex& index,int& w,int& h) const
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

        //For multiline labels we need to compute the height
        int attLineNum=0;
        if((attLineNum=index.data(AbstractNodeModel::AttributeLineRole).toInt()) > 1)
        {
            h=labelHeight(attLineNum);
        }
    }
}

void TreeNodeViewDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index,QSize& size) const
{
    size=QSize(0,0);

    //Background
    QStyleOptionViewItem vopt(option);
    initStyleOption(&vopt, index);

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
        int width=0;
        QString text=index.data(Qt::DisplayRole).toString();
        if(index.data(AbstractNodeModel::ServerRole).toInt() ==0)
        {
            width=renderServer(painter,index,vopt,text);
        }
        else
        {
            width=renderNode(painter,index,vopt,text);
        }

        size=QSize(width,nodeBox_->fullHeight);
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
                (this->*a)(painter,lst,vopt,size);
            }
            //if(width==0)
            //    width=300;
        }

    }

    painter->restore();

    //else
    //	QStyledItemDelegate::paint(painter,option,index);
}



int TreeNodeViewDelegate::renderServer(QPainter *painter,const QModelIndex& index,
                                           const QStyleOptionViewItem& option,QString text) const
{
    ServerHandler* server=static_cast<ServerHandler*>(index.data(AbstractNodeModel::ServerPointerRole).value<void*>());
    Q_ASSERT(server);

    int totalWidth=0;
    bool selected=option.state & QStyle::State_Selected;
    //int offset=nodeBox_->leftMargin;
    QFontMetrics fm(font_);

    //The initial filled rect (we will adjust its  width)
    QRect itemRect=option.rect.adjusted(nodeBox_->leftMargin,nodeBox_->topMargin,0,-nodeBox_->bottomMargin);

#if 0
    painter->setPen(QColor(190,190,190));
    painter->drawLine(0,option.rect.y()+1,painter->device()->width(),option.rect.y()+1);
    painter->drawLine(0,option.rect.bottom()-1,painter->device()->width(),option.rect.bottom()-1);
#endif
#if 0

    QRect progRect(0,itemRect.y()-deltaH,painter->device()->width(),itemRect.height()+2*deltaH);
    progRect.setWidth(painter->device()->width());
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QColor(190,190,190));
    painter->drawRect(progRect);
    painter->setBrush(QColor(230,230,230));
    painter->drawRect(progRect.adjusted(0,0,-progRect.width()*0.4,0));
#endif

    int currentRight=itemRect.x();

    NodeShape stateShape;
    stateShape.col_=index.data(Qt::BackgroundRole).value<QColor>();

    NodeText nodeText;
    nodeText.text_=text;
    int textWidth=fm.width(text);

    if(nodeStyle_ == BoxAndTextNodeStyle)
    {
        stateShape.shape_=QPolygon(QRect(itemRect.x(),itemRect.y(),itemRect.height(),itemRect.height()));
        QRect shBr=stateShape.shape_.boundingRect();
        currentRight=shBr.x()+shBr.width()+2;
        nodeText.br_=QRect(currentRight,itemRect.y(),textWidth+nodeBox_->leftPadding, itemRect.height());
        nodeText.fgCol_=QColor(Qt::black);
        currentRight=nodeText.br_.x()+nodeText.br_.width();
    }
    else
    {
        stateShape.shape_=QPolygon(QRect(itemRect.x(),itemRect.y(),textWidth+nodeBox_->leftPadding+nodeBox_->rightPadding,itemRect.height()));
        nodeText.br_=QRect(itemRect.x()+nodeBox_->leftPadding,itemRect.y(),textWidth, itemRect.height());
        nodeText.fgCol_=index.data(Qt::ForegroundRole).value<QColor>();
        QRect shBr=stateShape.shape_.boundingRect();
        currentRight=shBr.x()+shBr.width();;
    }

    //Refresh timer
   /* bool hasTimer=true;
    QRect timeRect(currentRight+offset,itemRect.y()-1,itemRect.height()+2,itemRect.height()+2);
    currentRight=timeRect.right();*/

    //Icons area
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

    //The pixmap (optional)
    bool hasPix=false;
    QRect pixRect;

    //The info rectangle (optional)
    QRect infoRect;
    QString infoTxt=index.data(AbstractNodeModel::InfoRole).toString();
    bool hasInfo=(infoTxt.isEmpty() == false);

    if(hasInfo)
    {
        //infoFont.setBold(true);
        fm=QFontMetrics(serverInfoFont_);

        int infoWidth=fm.width(infoTxt);
        infoRect = nodeText.br_;
        infoRect.setLeft(currentRight+fm.width('A'));
        infoRect.setWidth(infoWidth);
        currentRight=infoRect.x()+infoRect.width();
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

        currentRight=loadRect.x()+loadRect.width();

        //Add this index to the animations
        //There is no guarantee that this index will be valid in the future!!!
        an->addTarget(server->vRoot());
    }
    //Stops load animation
    else
    {
        if((an=animation_->find(Animation::ServerLoadType,false)) != NULL)
            an->removeTarget(server->vRoot());
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

            QFontMetrics fmNum(serverNumFont_);

            int numWidth=fmNum.width(numTxt);
            numRect = nodeText.br_;
            numRect.setLeft(currentRight+fmNum.width('A')/2);
            numRect.setWidth(numWidth);
            currentRight=numRect.x()+numRect.width();
        }
    }

    //Update
#if 0
    bool hasUpdate=false;
    ServerUpdateData updateData;
    if(server)
    {
        hasUpdate=true;
        updateData.br_=QRect(currentRight+3*offset,itemRect.y()-1,0,itemRect.height()+2);
        updateData.prevTime_=server->secsSinceLastRefresh();
        updateData.nextTime_=server->secsTillNextRefresh();
        if(updateData.prevTime_ >=0)
        {
            if(updateData.nextTime_ >=0)
            {
                updateData.prevText_="-" + formatTime(updateData.prevTime_);
                updateData.nextText_="+" +formatTime(updateData.nextTime_);
                updateData.prog_=(static_cast<float>(updateData.prevTime_)/static_cast<float>(updateData.prevTime_+updateData.nextTime_));
                updateData.br_.setWidth(fm.width("ABCDE")+fm.width(updateData.prevText_)+fm.width(updateData.nextText_)+2*offset);
            }
            else
            {
                updateData.prevText_="last update: " + formatTime(updateData.prevTime_);
                updateData.prog_=0;
                updateData.br_.setWidth(fm.width(updateData.prevText_));
            }
            currentRight=updateData.br_.right();
         }
        else
        {
            hasUpdate=false;
        }
    }
#endif

    //Define clipping
    int rightPos=currentRight+1;
    totalWidth=rightPos-option.rect.left();
    const bool setClipRect = rightPos > option.rect.right();
    if(setClipRect)
    {
        painter->save();
        painter->setClipRect(option.rect);
    }

    //Draw state/node rect
    renderServerCell(painter,stateShape,nodeText,selected);

    //Draw timer
    /*int remaining=6*60*1000; //   server->remainingTimeToRefresh();
    int total=10*60*1000; //server->refreshPeriod() ;
    renderTimer(painter,timeRect,remaining,total);*/

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
        painter->setPen(childCountColour);
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
#if 0
    if(hasUpdate)
    {
        renderServerUpdate(painter,updateData);
    }
#endif
    if(setClipRect)
    {
        painter->restore();
    }

    return totalWidth;
}

int TreeNodeViewDelegate::renderNode(QPainter *painter,const QModelIndex& index,
                                    const QStyleOptionViewItem& option,QString text) const
{
    int totalWidth=0;
    bool selected=option.state & QStyle::State_Selected;
    QFontMetrics fm(font_);

    //The initial filled rect (we will adjust its  width)
    QRect itemRect=option.rect.adjusted(nodeBox_->leftMargin,nodeBox_->topMargin,0,-nodeBox_->bottomMargin);

    NodeShape stateShape;
    NodeShape realShape;
    NodeText  nodeText;
    NodeText  typeText;

    //get the colours
    QVariant bgVa=index.data(Qt::BackgroundRole);
    bool hasRealBg=false;
    if(bgVa.type() == QVariant::List)
    {
        QVariantList lst=bgVa.toList();
        if(lst.count() ==  2)
        {
            hasRealBg=true;
            stateShape.col_=lst[0].value<QColor>();
            realShape.col_=lst[1].value<QColor>();
        }
    }
    else
    {
        stateShape.col_=bgVa.value<QColor>();
    }

    int currentRight=itemRect.x();

    //Node type
    QFontMetrics fmType(typeFont_);
    int typeWidth=fmType.width(" S");

    if(drawNodeType_)
    {
        int nodeType=index.data(AbstractNodeModel::NodeTypeRole).toInt();
        switch(nodeType)
        {
        case 0: typeText.text_="S"; break;
        case 1: typeText.text_="F"; break;
        case 2: typeText.text_="T"; break;
        case 3: typeText.text_="A"; break;
        default: break;
        }
    }

    //The text rectangle
    nodeText.text_=text;
    int textWidth=fm.width(text);

    if(nodeStyle_ == BoxAndTextNodeStyle)
    {
        int realW=itemRect.height()/4;

        QRect r1(currentRight,itemRect.y(),itemRect.height(),itemRect.height());
        if(hasRealBg)
            r1.adjust(0,0,-realW,0);

        stateShape.shape_=QPolygon(r1);

        if(hasRealBg)
        {
            QRect r2(r1.x()+r1.width(),r1.top(),realW+1,r1.height());
            realShape.shape_=QPolygon(r2);
            currentRight=r2.x()+r2.width()+2;
        }
        else
            currentRight=r1.x()+r1.width()+2;

        if(drawNodeType_)
        {
            typeText.br_=r1;
            typeText.fgCol_=index.data(AbstractNodeModel::NodeTypeForegroundRole).value<QColor>();
        }

        nodeText.br_=QRect(currentRight,r1.top(),textWidth+nodeBox_->leftPadding,r1.height());
        nodeText.fgCol_=QColor(Qt::black);
        currentRight=nodeText.br_.x() + nodeText.br_.width();

#if 0
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
            currentRight+=textWidth+nodeBox_->leftPadding;
        }
#endif

    }
    //Classic style
    else
    {
        if(drawNodeType_)
        {
            typeText.br_=QRect(currentRight,itemRect.y(),typeWidth,itemRect.height());
            typeText.fgCol_=typeFgColourClassic;
            typeText.bgCol_=typeBgColourClassic;
            currentRight=typeText.br_.x()+typeText.br_.width()+2;
        }

        QRect r1(currentRight,itemRect.y(),textWidth+nodeBox_->leftPadding+nodeBox_->rightPadding,itemRect.height());
        stateShape.shape_=QPolygon(r1);
        currentRight=r1.x()+r1.width();

        nodeText.br_=QRect(r1.left()+nodeBox_->leftPadding,r1.top(),textWidth,r1.height());
        nodeText.fgCol_=index.data(Qt::ForegroundRole).value<QColor>();

        if(hasRealBg)
        {
            int realW=6;
            QRect r2(r1.x()+r1.width(),r1.top(),realW+1,r1.height());
            realShape.shape_=QPolygon(r2);
            currentRight=r2.x()+r2.width();
        }
    }

    //Icons area
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
            numTxt="(" + va.toString() + ")";
            QFontMetrics fmNum(suiteNumFont_);

            int numWidth=fmNum.width(numTxt);
            numRect = nodeText.br_;
            numRect.setLeft(currentRight+fmNum.width('A')/2);
            numRect.setWidth(numWidth);
            currentRight=numRect.x()+numRect.width();
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
        reasonRect = nodeText.br_;
        reasonRect.setLeft(currentRight+fmReason.width('A')/2);
        reasonTxt=fmReason.elidedText(reasonTxt,Qt::ElideRight,220);
        reasonRect.setWidth(fmReason.width(reasonTxt));
        currentRight=reasonRect.x()+reasonRect.width();
    }

    //Define clipping
    int rightPos=currentRight+1;
    totalWidth=rightPos-option.rect.left();

#if 0
    const bool setClipRect = rightPos > option.rect.right();
    if(setClipRect)
    {
        painter->save();
        painter->setClipRect(option.rect);
    }
#endif


    renderNodeCell(painter,stateShape,realShape,nodeText,typeText,selected);

    //Draw icons
    for(int i=0; i < pixLst.count(); i++)
    {
        //painter->fillRect(pixRectLst[i],Qt::black);
        painter->drawPixmap(pixRectLst[i],pixLst[i]);
    }

    //Draw number
    if(hasNum)
    {
        painter->setPen(childCountColour);
        painter->setFont(suiteNumFont_);
        painter->drawText(numRect,Qt::AlignLeft | Qt::AlignVCenter,numTxt);
    }

    //Draw aborted reason
    if(hasReason)
    {
        painter->setPen(stateShape.col_.darker(120));
        painter->setFont(abortedReasonFont_);
        painter->drawText(reasonRect,Qt::AlignLeft | Qt::AlignVCenter,reasonTxt);
    }
#if 0
    if(setClipRect)
    {
        painter->restore();
    }
#endif
    return totalWidth;
}

void TreeNodeViewDelegate::renderServerCell(QPainter *painter,const NodeShape& stateShape,
                                        const NodeText& text,bool selected) const
{
    renderNodeShape(painter,stateShape);

    //Draw text
    painter->setFont(font_);
    painter->setPen(QPen(text.fgCol_,0));
    painter->drawText(text.br_,Qt::AlignHCenter | Qt::AlignVCenter,text.text_);

    //selection
    painter->setPen(nodeSelectPen_);
    painter->setBrush(Qt::NoBrush);
    QPolygon sel=stateShape.shape_;

    if(selected)
    {
        if(nodeStyle_ == BoxAndTextNodeStyle)
        {
            sel=sel.united(QPolygon(text.br_.adjusted(0,0,2,0)));
        }
        painter->drawRect(sel.boundingRect());
    }
}

void TreeNodeViewDelegate::renderNodeCell(QPainter *painter,const NodeShape& stateShape,const NodeShape &realShape,
                             const NodeText& nodeText,const NodeText& typeText,bool selected) const
{
    renderNodeShape(painter,stateShape);
    renderNodeShape(painter,realShape);

    //Draw type
    if(drawNodeType_)
    {
        if(typeText.bgCol_.isValid())
        {
            //painter->fillRect(typeText.br_,typeText.bgCol_);
            painter->setBrush(typeText.bgCol_);
            painter->setPen(typeText.bgCol_);
            painter->drawRect(typeText.br_);
        }
        painter->setFont(typeFont_);
        painter->setPen(typeText.fgCol_);
        painter->setBrush(Qt::NoBrush);
        painter->drawText(typeText.br_,Qt::AlignCenter,typeText.text_);
   }

    //Draw text
    painter->setFont(font_);
    painter->setPen(QPen(nodeText.fgCol_,0));
    painter->drawText(nodeBox_->adjustTextRect(nodeText.br_),Qt::AlignHCenter | Qt::AlignVCenter,nodeText.text_);

    //selection
    painter->setPen(nodeSelectPen_);
    painter->setBrush(Qt::NoBrush);
    QPolygon sel=stateShape.shape_;

    if(selected)
    {
        if(nodeStyle_ == BoxAndTextNodeStyle)
        {
            if(!realShape.shape_.isEmpty())
                sel=sel.united(realShape.shape_);

            sel=sel.united(QPolygon(nodeText.br_.adjusted(0,0,0,0)));
        }
        else
        {
            if(!realShape.shape_.isEmpty())
                sel=sel.united(realShape.shape_);
        }
        painter->drawRect(sel.boundingRect());
    }

   /*

        if(nodeStyle_ == BoxAndTextNodeStyle)
        {
            painter->setFont(typeFont_);
            painter->setPen();
            painter->drawText(typeText.br_,Qt::AlignCenter,typeText.text_);
        }
        else
        {
            painter->setPen(Qt::NoPen);

            if(typeTxt == "S")
                painter->setBrush(QColor(150,150,150)); //bgBrush);
            else if(typeTxt == "F")
                painter->setBrush(QColor(150,150,150)); //bgBrush);
            else
                painter->setBrush(QColor(190,190,190)); //bgBrush);

            painter->drawRect(typeRect);

            painter->setPen(Qt::white);
            painter->setFont(typeFont_);
            painter->drawText(typeRect,Qt::AlignCenter,typeTxt);
         }
    }

    //Draw text
    painter->setFont(font_);
    painter->setPen(QPen(fg,0));
    painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);

    //selection
    painter->setPen(nodeSelectPen_);
    painter->setBrush(Qt::NoBrush);
    QPolygon sel=stateShape.shape_;

    if(selected)
    {
        if(nodeStyle_ == BoxAndTextNodeStyle)
        {
            if(!realShape.shape_.isEmpty())
                sel=sel.united(realShape.shape_);

            sel=sel.united(QPolygon(textRect.adjusted(0,0,2,0)));
        }
        else
        {
            if(!realShape.shape_.isEmpty())
                sel=sel.united(realShape.shape_);
        }
        painter->drawRect(sel.boundingRect());
        //painter->drawPolygon(sel);
    }
*/
    //painter->setRenderHints(QPainter::Antialiasing,false);
}


void TreeNodeViewDelegate::renderNodeShape(QPainter* painter,const NodeShape& shape) const
{
    if(shape.shape_.isEmpty())
        return;

    QColor bg=shape.col_;
    QColor bgLight, borderCol;
    Palette::statusColours(bg,bgLight,borderCol);

    QBrush bgBrush;
    if(useStateGrad_)
    {
        grad_.setColorAt(0,bgLight);
        grad_.setColorAt(1,bg);
        bgBrush=QBrush(grad_);
    }
    else
        bgBrush=QBrush(bg);

    //Fill  shape
    painter->setPen(borderCol);
    painter->setBrush(bgBrush);
    painter->drawPolygon(shape.shape_);
}

void TreeNodeViewDelegate::renderTimer(QPainter *painter,QRect target,int remaining, int total) const
{
    QImage img(target.width(),target.height(),QImage::Format_ARGB32_Premultiplied);
    QRect r=img.rect().adjusted(2,2,-2,-2);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing,true);

    int angle=static_cast<int>(360.*static_cast<float>(total-remaining)/static_cast<float>(total));
    /*angle-=90.;
    if(angle >=0 && angle <= 90) angle=90-angle;
    else
       angle=450-angle;*/

    QColor c(43,97,158);

    QBrush b(c);
    p.setBrush(b);
    p.setPen(c);
    p.drawPie(r,90*16,-angle*16);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(r);

    painter->drawImage(target,img,img.rect());
}


void TreeNodeViewDelegate::renderServerUpdate(QPainter* painter,const ServerUpdateData& data) const
{
    QFont font(font_);
    font.setPointSize(font_.pointSize()-1);
    QFontMetrics fm(font);
    painter->setFont(font);
    painter->setPen(Qt::black);

    QColor minCol=QColor(198,215,253);
    QColor maxCol=QColor(43,97,158);

    QRect r1=data.br_;
    r1.setWidth(fm.width(data.prevText_));
    painter->setPen(minCol);
    //painter->setPen(Qt::red);
    painter->drawText(r1,Qt::AlignLeft | Qt::AlignVCenter,data.prevText_);

    if(!data.prevText_.isEmpty())
    {
        QRect r2=data.br_;
        r2.setX(data.br_.right()-fm.width(data.nextText_));
        //painter->setPen(QColor(1,128,73));
        painter->setPen(maxCol);
        painter->drawText(r2,Qt::AlignRight | Qt::AlignVCenter,data.nextText_);

        int dh=(data.br_.height()-fm.height()+1)/2;
        QRect r=data.br_.adjusted(r1.width()+4,2*dh,-r2.width()-4,-2*dh);

        int pos=static_cast<int>(data.prog_* r.width());
        QRect rPrev=r.adjusted(0,0,-(r.width()-pos),0);

        QLinearGradient grad;
        grad.setCoordinateMode(QGradient::ObjectBoundingMode);
        grad.setStart(0,0);
        grad.setFinalStop(1,0);
        QColor posCol=interpolate(minCol,maxCol,data.prog_);

        grad.setColorAt(0,minCol);
        grad.setColorAt(1,posCol);
        painter->setPen(Qt::NoPen);
        painter->setBrush(grad);
        painter->drawRect(rPrev);

        painter->setBrush(Qt::NoBrush);
        painter->setPen(QColor(190,190,190));
        painter->drawRect(r);
    }
}

void TreeNodeViewDelegate::widthHintServer(const QModelIndex& index,int& itemWidth, QString text) const
{
    ServerHandler* server=static_cast<ServerHandler*>(index.data(AbstractNodeModel::ServerPointerRole).value<void*>());
    Q_ASSERT(server);

    QFontMetrics fm(font_);

    //The initial filled rect. We only care of the width
    QRect itemRect(nodeBox_->leftMargin,0,10,10);
    int currentRight=itemRect.left();

    NodeShape stateShape;

    NodeText nodeText;
    nodeText.text_=text;
    int textWidth=fm.width(text);

    if(nodeStyle_ == BoxAndTextNodeStyle)
    {
        currentRight+=2+textWidth+nodeBox_->leftPadding;
    }
    else
    {
        currentRight+=textWidth+nodeBox_->leftPadding+nodeBox_->rightPadding;
    }

    //Icons area
    Q_ASSERT(model_);
    int pixNum=model_->iconNum(server->vRoot());
    if(pixNum > 0)
    {
        currentRight+=nodeBox_->iconPreGap+pixNum*nodeBox_->iconSize + (pixNum-1)*nodeBox_->iconGap;
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
    bool hasLoad=index.data(AbstractNodeModel::LoadRole).toBool();
    Animation* an=0;

    //Update load animation
    if(hasLoad)
    {
        an=animation_->find(Animation::ServerLoadType,true);
        currentRight+=fm.width('A')+an->scaledSize().width();
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

int TreeNodeViewDelegate::nodeWidth(const QModelIndex& index,QString text) const
{
    VNode* node=static_cast<VNode*>(index.data(AbstractNodeModel::NodePointerRole).value<void*>());
    Q_ASSERT(node);

    int offset=nodeBox_->leftMargin;
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
        //state box
        currentRight+=itemRect.height();
        if(hasRealBg)
        {
            currentRight+=1;
        }
        currentRight+=2;

        //node name
        currentRight+=textWidth+nodeBox_->leftPadding;
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
        currentRight+=textWidth+nodeBox_->leftPadding+nodeBox_->rightPadding;

        if(hasRealBg)
        {
            int realW=6;
            currentRight+=realW+1;
        }
    }

    //Icons area
    Q_ASSERT(model_);
    int pixNum=model_->iconNum(node);
    if(pixNum > 0)
    {
        currentRight+=nodeBox_->iconPreGap+pixNum*nodeBox_->iconSize + (pixNum-1)*nodeBox_->iconGap;
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


QString TreeNodeViewDelegate::formatTime(int timeInSec) const
{
    int h=timeInSec/3600;
    int r=timeInSec%3600;
    int m=r/60;
    int s=r%60;

    QTime t(h,m,s);
    if(h > 0)
        return t.toString("h:mm:ss");
    else
        return t.toString("m:ss");

    return QString();
}

QColor TreeNodeViewDelegate::interpolate(QColor c1,QColor c2,float r) const
{
    return QColor::fromRgbF(c1.redF()+r*(c2.redF()-c1.redF()),
                  c1.greenF()+r*(c2.greenF()-c1.greenF()),
                  c1.blueF()+r*(c2.blueF()-c1.blueF()));
}
