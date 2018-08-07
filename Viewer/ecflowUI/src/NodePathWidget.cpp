/***************************** LICENSE START ***********************************

 Copyright 2009-2017 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#include "NodePathWidget.hpp"

#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPolygon>
#include <QSizePolicy>
#include <QStyleOption>
#include <QToolButton>
#include <QVector>

#include "VNode.hpp"
#include "VProperty.hpp"
#include "Palette.hpp"
#include "PropertyMapper.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"
#include "VNState.hpp"
#include "VSState.hpp"
#include "VSettings.hpp"

static std::vector<std::string> propVec;

QColor NodePathItem::disabledBgCol_;
QColor NodePathItem::disabledBorderCol_;
QColor NodePathItem::disabledFontCol_;
int NodePathItem::triLen_=10;
int NodePathItem::height_=0;
int NodePathItem::hPadding_=2;
int NodePathItem::vPadding_=0;

//#define _UI_NODEPATHWIDGET_DEBUG

BcWidget::BcWidget(QWidget* parent) : 
    QWidget(parent),
	hMargin_(1),
	vMargin_(1),    
	gap_(5),
    width_(0),
    maxWidth_(0),
    itemHeight_(0),
    text_("No selection"),
    textCol_(Qt::white),
    textDisabledCol_(QColor(220,220,220)),
    useGrad_(true),
    gradLighter_(150),
    hovered_(-1),
    elided_(false)
{
    font_=QFont();
    font_.setPointSize(font_.pointSize()-1);
    QFontMetrics fm(font_);

    itemHeight_=NodePathItem::height(font_);
    height_=itemHeight_+2*vMargin_;
    
    setMouseTracking(true);
    
    //Property
    if(propVec.empty())
    {
        propVec.push_back("view.common.node_style");
        propVec.push_back("view.common.node_gradient");
    }

    prop_=new PropertyMapper(propVec,this);
   
    updateSettings();

    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    setMinimumSize(width_,height_);

    ellipsisItem_ = new NodePathEllipsisItem(this);
    ellipsisItem_->visible_=false;

    reset(items_,100);

    //setAutoFillBackground(true);
    //QPalette pal=palette();
    //pal.setColor(QPalette::Window,Qt::transparent);
    //setPalette(pal);
}

BcWidget::~BcWidget()
{
    delete prop_;
    delete ellipsisItem_;
}    

void BcWidget::notifyChange(VProperty *p)
{
    updateSettings();
}    
    
void BcWidget::updateSettings()
{
    if(VProperty *p=prop_->find("view.common.node_gradient"))
        useGrad_=p->value().toBool();
}    

bool BcWidget::isFull() const
{
    return !elided_ && !ellipsisItem_->visible_;
}

void BcWidget::clear()
{
    items_.clear();
    reset(items_,100);
}

void BcWidget::resetBorder(int idx)
{
    if(idx >=0 && idx < items_.count())
    {
        items_.at(idx)->resetBorder(idx == hovered_);
        updatePixmap(idx);
        update();
    }
}

void BcWidget::reset(QString txt, int maxWidth)
{
    maxWidth_=maxWidth;
    hovered_=-1;
    ellipsisItem_->visible_=false;
    elided_=false;

    QFontMetrics fm(font_);
    int xp=hMargin_;
    int yp=vMargin_;

    if(!txt.isEmpty())
    {
        text_=txt;
    }
    else
    {
        text_="No selection";
    }

    int len=fm.width(text_);
    textRect_=QRect(xp,yp,len,itemHeight_);
    width_=xp+len+4;

    crePixmap();
    resize(width_,height_);
    update();
}

void BcWidget::reset(int idx,QString text,QColor bgCol,QColor fontCol)
{
    if(idx >=0 && idx < items_.count())
    {                
        bool newText=(text != items_.at(idx)->text_);
        items_[idx]->reset(text,bgCol,fontCol,idx == hovered_);

        if(newText)
           reset(items_,maxWidth_);
        else
        {
            updatePixmap(idx); 
            update();
        }    
    }
}    
    
void BcWidget::reset(QList<NodePathItem*> items, int maxWidth)
{
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UiLog().dbg() << "BcWidget::reset -->";
    UiLog().dbg() << "   maxWidth=" << maxWidth;
#endif

    maxWidth_=maxWidth;
    items_=items;
    hovered_=-1;
    ellipsisItem_->visible_=false;
    elided_=false;

    QFontMetrics fm(font_);
    int xp=hMargin_;
    int yp=vMargin_;

    if(items_.count() ==0)
    {
        int len=fm.width(text_);
        textRect_=QRect(xp,yp,len,itemHeight_);
        width_=xp+len+4;
    }
    else
    {
        //
        // xp is the top right corner of the shape (so it is not the rightmost edge)
        //
        // server shape:
        //
        //  ********
        //  *        *
        //  ********
        //
        // other shape:
        //
        //  ********
        //    *      *
        //  ********
        //

        NodePathItem *lastItem=items_[items_.count()-1];
        Q_ASSERT(lastItem);
        int maxRedTextLen=0;

        //Defines the shapes and positions for all the items
        for(int i=0; i < items_.count(); i++)
        {
            xp=items_[i]->adjust(xp,yp);
           
            if(i != items_.count()-1)
            {
                xp+=gap_;
                int tl=items_[i]->textLen();
                if(tl > maxRedTextLen)
                    maxRedTextLen=tl;
            }
        }

        //The total width
        width_=xp+NodePathItem::triLen_+hMargin_;

#ifdef _UI_NODEPATHWIDGET_DEBUG
        UiLog().dbg() << "   full width=" << width_;
#endif

        //maxWidth-=2*hMargin_;

        //If the total width is too big we try to use elidedtext in the items
        //(with the execption of the last item)
        int redTextLen=0;
        if(width_ > maxWidth)
        {
#ifdef _UI_NODEPATHWIDGET_DEBUG
            UiLog().dbg() << "   try elided text";
#endif
            //Try different elided text lenghts
            for(int i=20; i >= 3; i--)
            {
                QString t;
                for(int j=0; j < i; j++)
                {
                    t+="A";
                }
                t+="...";

                //We only check the elided texts that are shorter then the max text len
                redTextLen=fm.width(t);
                if(redTextLen < maxRedTextLen)
                {
                    //Estimate the total size with the elided text items
                    xp=hMargin_;
                    int estWidth=estimateWidth(0,xp,redTextLen);

                    //if the size fits into maxWidth we adjust all the items
                    if(estWidth < maxWidth)
                    {
                        int xp=hMargin_;
                        width_ = adjustItems(0,xp,yp,redTextLen);
                        elided_=true;

                        Q_ASSERT(width_== estWidth);
                        Q_ASSERT(width_ <  maxWidth);
                        break; //This breaks the whole for loop
                    }
                }
            }
        }

        //If the total width is still too big we start hiding items from the left
        //and insert an ellipsis item to the front.
        int xpAfterEllipsis=0;
        if(width_ > maxWidth)
        {
#ifdef _UI_NODEPATHWIDGET_DEBUG
            UiLog().dbg() << "   insert ellipsis to front + remove items";
            UiLog().dbg() << "     redTextLen=" << redTextLen;
#endif
            Q_ASSERT(elided_==false);

            //width_=maxWidth;

            xp=hMargin_;
            ellipsisItem_->visible_=true;
            ellipsisItem_->adjust(xp,yp);
            xp=ellipsisItem_->estimateRightPos(xp);
            xpAfterEllipsis=xp+gap_;
            bool fitOk=false;
            int estWidth=0;
            for(int i=0; i < items_.count()-1; i++)
            {
                xp=xpAfterEllipsis;
                items_[i]->visible_=false;
#ifdef _UI_NODEPATHWIDGET_DEBUG
                UiLog().dbg() << "     omit item " << i;
#endif
                estWidth=estimateWidth(i+1,xp,redTextLen);
#ifdef _UI_NODEPATHWIDGET_DEBUG
                UiLog().dbg() << "     estWidth " << estWidth;
#endif                
                if(estWidth  < maxWidth)
                {
                    fitOk=true;
                    break;
                }
            }

            if(fitOk)
            {                
                xp=xpAfterEllipsis;
                width_=adjustVisibleItems(0,xp,yp,redTextLen);
                Q_ASSERT(width_ == estWidth);
                Q_ASSERT(width_ < maxWidth);
            }
            else
            {
                xp=xpAfterEllipsis;           
                xp=lastItem->estimateRightPos(xp);
                estWidth=xp+NodePathItem::triLen_+hMargin_;
                if(estWidth < maxWidth)
                {
                    xp=xpAfterEllipsis;
                    xp=lastItem->adjust(xp,yp);
                    width_=xp+NodePathItem::triLen_+hMargin_;
                    Q_ASSERT(width_ == estWidth);
                    Q_ASSERT(width_ < maxWidth);
                }

            }
        }

        //If the total width is still too big we try to use elidedtext in the last item
        //(at this point all the other items are hidden)
        if(width_ > maxWidth)
        {
            int len=lastItem->textLen();

            //Try different elided text lenghts
            for(int i=30; i >= 3; i--)
            {
                QString t;
                for(int j=0; j < i; j++)
                {
                    t+="A";
                }
                t+="...";

                //We only check the elided texts that are shorter then the max text len
                redTextLen=fm.width(t);
                if(redTextLen < len)
                {
                    //Estimate the total size with the elided text item
                    xp=xpAfterEllipsis;
                    xp=lastItem->estimateRightPos(xp,redTextLen);
                    int estWidth=xp+NodePathItem::triLen_+hMargin_;
                    if(estWidth < maxWidth)
                    {
                        xp=xpAfterEllipsis;
                        xp=lastItem->adjust(xp,yp,redTextLen);
                        width_=xp+NodePathItem::triLen_+hMargin_;
                        Q_ASSERT(width_ == estWidth);
                        Q_ASSERT(width_ < maxWidth);
                        break;
                    }
                }
            }
        }

        //If the total width is still too big we also hide the last item and
        //only show the ellipsis item
        if(width_ > maxWidth)
        {
            lastItem->visible_=false;
            width_=maxWidth;
        }
    }

    crePixmap();
    
    resize(width_,height_);

    update();
}  

int BcWidget::estimateWidth(int startIndex,int xp,int redTextLen)
{
    for(int i=startIndex; i < items_.count(); i++)
    {
        if(i != items_.count()-1)
        {
            xp=items_[i]->estimateRightPos(xp,redTextLen);
            xp+=gap_;
        }
        else
            xp=items_[i]->estimateRightPos(xp);
    }

    return xp+NodePathItem::triLen_+hMargin_;
}

int BcWidget::adjustItems(int startIndex,int xp,int yp,int redTextLen)
{
    for(int i=startIndex; i < items_.count(); i++)
    {
        if(i != items_.count()-1)
        {
            xp=items_[i]->adjust(xp,yp,redTextLen);
            xp+=gap_;
        }
        else
            xp=items_[i]->adjust(xp,yp);
    }

    return xp+NodePathItem::triLen_+hMargin_;
}

int BcWidget::adjustVisibleItems(int startIndex,int xp,int yp,int redTextLen)
{
    for(int i=startIndex; i < items_.count(); i++)
    {
        if(items_[i]->visible_)
        {
            if(i != items_.count()-1)
            {
                xp=items_[i]->adjust(xp,yp,redTextLen);
                xp+=gap_;
            }
            else
                xp=items_[i]->adjust(xp,yp);
        }
    }
    return xp+NodePathItem::triLen_+hMargin_;
}

void BcWidget::adjustSize(int maxWidth)
{
    if(isFull())
    {
        if(width_ > maxWidth)
           reset(items_,maxWidth);
    }
    else
    {
        reset(items_,maxWidth);
    }
}

void BcWidget::crePixmap()
{        
    pix_=QPixmap(width_,height_);
    pix_.fill(Qt::transparent);

    QPainter painter(&pix_);
    painter.setRenderHints(QPainter::Antialiasing,true);
    
    painter.setFont(font_);

    if(items_.count() == 0)
    {
        if(isEnabled())
            painter.setPen(textCol_);
        else
            painter.setPen(textDisabledCol_);

        painter.drawText(textRect_,Qt::AlignHCenter | Qt::AlignVCenter, text_);
    }
    else
    {    
        for(int i=0; i < items_.count(); i++)
        {
            items_.at(i)->enabled_=isEnabled();
            items_.at(i)->draw(&painter,useGrad_,gradLighter_);
        }
    } 
    
    if(ellipsisItem_->visible_)
    {
        ellipsisItem_->enabled_=isEnabled();
        ellipsisItem_->draw(&painter,false,gradLighter_);
    }    
}  

void BcWidget::updatePixmap(int idx)
{
    if(idx >=0 && idx < items_.count())
    {
         QPainter painter(&pix_);
         painter.setRenderHints(QPainter::Antialiasing,true);
         painter.setFont(font_);
         items_.at(idx)->draw(&painter,useGrad_,gradLighter_);
    }        
}    

void BcWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawPixmap(0,0,pix_);    
}

void BcWidget::mouseMoveEvent(QMouseEvent *event)
{   
    for(int i=0; i < items_.count(); i++)
    {
        if(items_.at(i)->shape_.containsPoint(event->pos(),Qt::OddEvenFill))
        {
            if(hovered_ == -1)
            {
                hovered_=i;
                resetBorder(i);
            }
            else if(hovered_ != i)
            {
                int prev=hovered_;
                hovered_=i;
                resetBorder(prev);
                resetBorder(i);
            }

            return;
        }
    }

    if(hovered_ != -1)
    {
        int prev=hovered_;
        hovered_=-1;
        resetBorder(prev);
    }

    QWidget::mouseMoveEvent(event);
}

void BcWidget::mousePressEvent(QMouseEvent *event)
{   
	if(event->button() != Qt::RightButton && event->button() != Qt::LeftButton)
		return;

	for(int i=0; i < items_.count(); i++)
    {
        if(items_[i]->visible_ &&
           items_[i]->shape_.containsPoint(event->pos(),Qt::OddEvenFill))
		{
			if(event->button() == Qt::RightButton)
			{
				Q_EMIT menuSelected(i,event->pos());
				return;
			}
			else if(event->button() == Qt::LeftButton)
			{
				Q_EMIT itemSelected(i);
				return;
			}
        }    
    }

    QWidget::mousePressEvent(event);
}    

void BcWidget::changeEvent(QEvent* event)
{
    if(event->type() == QEvent::EnabledChange)
    {
        crePixmap();
//TODO: Will update be called automatically?
    }

    QWidget::changeEvent(event);
}

//=====================================================
//
// NodePathItem
//
//=====================================================

NodePathItem::NodePathItem(BcWidget* owner,int index,QString text,QColor bgCol,QColor fontCol,bool hasMenu,bool current) :
    owner_(owner),
    index_(index),
    text_(text),
    bgCol_(bgCol),
    fontCol_(fontCol),
    current_(current),
    hasMenu_(hasMenu),
    visible_(false),
    enabled_(true)
{
    height(owner_->font());

    if(!disabledBgCol_.isValid())
    {
        disabledBgCol_=QColor(200,200,200);
        disabledBorderCol_=QColor(170,170,170);
        disabledFontCol_=QColor(40,40,40);
    }

    grad_.setCoordinateMode(QGradient::ObjectBoundingMode);
    grad_.setStart(0,0);
    grad_.setFinalStop(0,1);
}    

int NodePathItem::height(QFont f)
{
    if(height_==0)
    {       
        QFontMetrics fm(f);
        height_=fm.height()+2*vPadding_;
    }
    return height_;
}

void NodePathItem::setCurrent(bool)
{
}

int NodePathItem::textLen() const
{
    QFontMetrics fm(owner_->font());
    return fm.width(text_);
}

void NodePathItem::makeShape(int xp,int yp,int len)
{
    QVector<QPoint> vec;
    vec << QPoint(0,0);
    vec << QPoint(len+triLen_,0);
    vec << QPoint(len+2*triLen_,height_/2);
    vec << QPoint(len+triLen_,height_);
    vec << QPoint(0,height_);
    vec << QPoint(triLen_,height_/2);

    shape_=QPolygon(vec).translated(xp,yp);

    textRect_=QRect(xp+triLen_+hPadding_,yp,len,height_);
}

int NodePathItem::adjust(int xp,int yp,int elidedLen)
{
    visible_=true;

    QFontMetrics fm(owner_->font());
    int len;
    if(elidedLen == 0)
    {
        elidedText_=QString();
        len=fm.width(text_);
    }
    else
    {
        elidedText_=fm.elidedText(text_,Qt::ElideRight,elidedLen);
        len=fm.width(elidedText_);
    }

    borderCol_=bgCol_.darker(125);

    makeShape(xp,yp,len);

    return rightPos(xp,len);
}


//It returns the x position of the top right corner!
int NodePathItem::rightPos(int xp,int len) const
{
    return xp+len+triLen_;
}


//It returns the x position of the top right corner!
int NodePathItem::estimateRightPos(int xp,int elidedLen)
{
    QFontMetrics fm(owner_->font());
    int len;

    if(elidedLen==0)
        len=fm.width(text_);
    else
        len=fm.width(fm.elidedText(text_,Qt::ElideRight,elidedLen));

    return rightPos(xp,len);
}

void NodePathItem::resetBorder(bool hovered)
{
    if(!hovered)
        borderCol_=bgCol_.darker(125);
    else
        borderCol_=bgCol_.darker(240);
}

void NodePathItem::reset(QString text,QColor bgCol,QColor fontCol,bool hovered)
{
    text_=text;
    bgCol_=bgCol;
    fontCol_=fontCol;

    if(!hovered)
        borderCol_=bgCol_.darker(125);
    else
        borderCol_=bgCol_.darker(240);
}

void NodePathItem::draw(QPainter  *painter,bool useGrad,int lighter)
{    
    if(!visible_)
        return;

    QColor border, bg, fontCol;
    if(enabled_)
    {
        border=borderCol_;
        bg=bgCol_;
        fontCol=fontCol_;        
    }
    else
    {
        border=disabledBorderCol_;
        bg=disabledBgCol_;
        fontCol=disabledFontCol_;
    }

    QBrush bgBrush;       
    if(useGrad)
    {        
        QColor bgLight;
        Palette::statusColours(bg,bgLight,border);

        //QColor bgLight=bg.lighter(lighter);
        grad_.setColorAt(0,bgLight);
        grad_.setColorAt(1,bg);
        bgBrush=QBrush(grad_);
    }
    else
        bgBrush=QBrush(bg);
    
    painter->setPen(QPen(border,0));
    painter->setBrush(bgBrush);
    painter->drawPolygon(shape_);

    /*if(current_)
    {
    	painter->setPen(QPen(borderCol_,0));
    }*/

    painter->setPen(fontCol);
    painter->drawText(textRect_,Qt::AlignVCenter | Qt::AlignHCenter,(elidedText_.isEmpty())?text_:elidedText_);
    
}

//=====================================================
//
// NodePathServerItem
//
//=====================================================

//It returns the x position of the top right corner!
int NodePathServerItem::rightPos(int xp,int len) const
{
    return xp+len;
}

void NodePathServerItem::makeShape(int xp,int yp,int len)
{
    QVector<QPoint> vec;
    vec << QPoint(0,0);
    vec << QPoint(len,0);
    vec << QPoint(len+triLen_,height_/2);
    vec << QPoint(len,height_);
    vec << QPoint(0,height_);

    shape_=QPolygon(vec).translated(xp,yp);

    textRect_=QRect(xp+hPadding_,yp,len,height_);
}

//=====================================================
//
// NodePathEllipsisItem
//
//=====================================================

NodePathEllipsisItem::NodePathEllipsisItem(BcWidget* owner) :
    NodePathItem(owner,-1,QString(0x2026),QColor(240,240,240),QColor(Qt::black),false,false)
{
    borderCol_=QColor(190,190,190);
}

//=============================================================
//
//  NodePathWidget
//
//=============================================================

NodePathWidget::NodePathWidget(QWidget *parent) :
  QWidget(parent),
  mode_(GuiMode)
{
	layout_=new QHBoxLayout(this);
	layout_->setSpacing(0);
    layout_->setContentsMargins(2,2,3,2);
	setLayout(layout_);

    bc_=new BcWidget(this);
    layout_->addWidget(bc_);
    
    connect(bc_,SIGNAL(itemSelected(int)),
             this,SLOT(slotNodeSelected(int)));
    connect(bc_,SIGNAL(menuSelected(int,QPoint)),
             this,SLOT(slotMenuSelected(int,QPoint)));

    setAutoFillBackground(true);

    //We make the background transparent
    QPalette pal=palette();
    pal.setColor(QPalette::Window,Qt::transparent);
    setPalette(pal);
}

NodePathWidget::~NodePathWidget()
{
	clear(true);
}

void NodePathWidget::useTransparentBg(bool b)
{
    QPalette pal=palette();

    if(b)
    {
        pal.setColor(QPalette::Window,Qt::transparent);
        bc_->setTextColour(Qt::white);
        bc_->setTextDisabledColour(QColor(220,220,220));
    }
    else
    {
        pal.setColor(QPalette::Window,Qt::white);
        bc_->setTextColour(Qt::black);
        bc_->setTextDisabledColour(QColor(60,60,60));
    }
    setPalette(pal);
}

void NodePathWidget::clear(bool detachObservers)
{
    setEnabled(true);

    if(detachObservers && info_ && info_->server())
	{
		info_->server()->removeNodeObserver(this);
		info_->server()->removeServerObserver(this);
	}

    if(detachObservers && info_)
    {
        info_->removeObserver(this);
    }

    if(info_)
        info_->removeObserver(this);

    info_.reset();
    
    clearItems();

    setEnabled(true);
}

void NodePathWidget::clearItems()
{
    bc_->clear();

    int cnt=nodeItems_.count();
    for(int i=0; i < cnt; i++)
    {
        delete nodeItems_.takeLast();
    }
    nodeItems_.clear();
}

void NodePathWidget::setMode(Mode mode)
{
    if(mode_ != mode)
    {
        mode_=mode;
        VInfo_ptr info=info_;
        clear(true);
        setPath(info);
    }
}

void NodePathWidget::slotContextMenu(const QPoint& pos)
{
}

void NodePathWidget::adjust(VInfo_ptr info,ServerHandler** serverOut,bool &sameServer)
{
    ServerHandler* server=0;

  	//Check if there is data in info
    if(info)
  	{
        server=info->server();

  		sameServer=(info_)?(info_->server() == server):false;

  		//Handle observers
  		if(!sameServer)
  		{
  			if(info_ && info_->server())
  			{
  				info_->server()->removeServerObserver(this);
  				info_->server()->removeNodeObserver(this);
  			}

  			info->server()->addServerObserver(this);
  			info->server()->addNodeObserver(this);

#if 0
  			if(server)
  			{
  				if(reloadTb_)
  				{
  					reloadTb_->setToolTip("Refresh server <b>" + QString::fromStdString(server->name()) + "</b>");
                    reloadTb_->setEnabled(true);
  				}
  			}
  			else
  			{
                reloadTb_->setToolTip("");
                reloadTb_->setEnabled(false);
  			}
#endif
  		}
  	}
  	//If the there is no data we clean everything and return
  	else
  	{
  	  	if(info_ && info_->server())
  	  	{
  	  		info_->server()->removeServerObserver(this);
  	  		info_->server()->removeNodeObserver(this);
  	  	}
#if 0
        reloadTb_->setToolTip("");
        reloadTb_->setEnabled(false);
#endif
  	}

    //Set the info
    if(info_)
    {
        info_->removeObserver(this);
    }

    info_=info;

    if(info_)
    {
        info_->addObserver(this);
    }

  	*serverOut=server;
}


void NodePathWidget::reset()
{
	setPath(info_);
}


void NodePathWidget::setPath(VInfo_ptr info)
{
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UiLog().dbg() << "NodePathWidget::setPath -->";
#endif

    setEnabled(true);

  	ServerHandler *server=0;
  	bool sameServer=false;

  	VInfo_ptr info_ori=info_;

  	adjust(info,&server,sameServer);

    if(!info_ || !info_->server())
  	{
  		clear();
  		return;
  	}
  	else
    {    
        clearItems();
    }
      
    //------------------------------------
    // Only text is displayed
    //------------------------------------

    if(mode_ == TextMode)
    {
        info_=info;
        QString pt;
        if(info_)
            pt=QString::fromStdString(info_->path());

        bc_->reset(pt,bcWidth());
        return;
    }

    //------------------------------------
    // Interactive breadcrumsbs
    //------------------------------------

    Q_ASSERT(mode_ = GuiMode);

	//Get the node list including the server
  	std::vector<VNode*> lst;
  	if(info_->node())
  	{
  		lst=info_->node()->ancestors(VNode::ParentToChildSort);
  	}

	//--------------------------------------------
	// Reset/rebuild the contents
	//--------------------------------------------

	for(unsigned int i=0; i < lst.size(); i++)
	{
		//---------------------------
		// Create node/server item
		//---------------------------

		QColor col;
		QString name;
		NodePathItem* nodeItem=0;

		VNode *n=lst.at(i);
		col=n->stateColour(); 
#ifdef _UI_NODEPATHWIDGET_DEBUG
        UiLog().dbg() << "   state=" << n->stateName();
#endif
		QColor fontCol=n->stateFontColour();
		name=n->name();
        bool hasChildren=(n->numOfChildren() >0);

        if(i==0)
        {
            nodeItem=new NodePathServerItem(bc_,i,name,col,fontCol,hasChildren,(i == lst.size()-1)?true:false);
        }
        else
        {
            nodeItem=new NodePathItem(bc_,i,name,col,fontCol,hasChildren,(i == lst.size()-1)?true:false);
        }
        nodeItems_ << nodeItem;
	}

    bc_->reset(nodeItems_,bcWidth());

#ifdef _UI_NODEPATHWIDGET_DEBUG
    UiLog().dbg() << "<-- setPath";
#endif
}

int NodePathWidget::bcWidth()
{
    //return width()-reloadTb_->width()-5;
    return width()-5;
}

void  NodePathWidget::slotNodeSelected(int idx)
{
    Q_ASSERT(mode_ == GuiMode);
    if(idx != -1)
	{
		Q_EMIT selected(nodeAt(idx));
	}
}

void  NodePathWidget::slotMenuSelected(int idx,QPoint bcPos)
{
    Q_ASSERT(mode_ == GuiMode);
    if(idx != -1)
	{
        loadMenu(bc_->mapToGlobal(bcPos),nodeAt(idx));
	}
}

//-------------------------------------------------------------------------------------------
// Get the object from nodeItems_ at position idx.
// This is the order/position of the items:
//
//   0         1     2     ....    nodeItems_.count()-2          nodeItems_.count()-1
// server                              node's parent 		           node (=info_)
//--------------------------------------------------------------------------------------------

VInfo_ptr NodePathWidget::nodeAt(int idx)
{
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UiLog().dbg() << "NodePathWidget::nodeAt idx=" << idx;
#endif

    Q_ASSERT(mode_ == GuiMode);
    if(mode_ == TextMode)
        return VInfo_ptr();

	ServerHandler* server=info_->server();

	if(info_ && server)
	{
		if(VNode *n=info_->node()->ancestorAt(idx,VNode::ParentToChildSort))
		{
			if(n == info_->node())
				return info_;
			else if(n->isServer())
				return VInfoServer::create(n->server());
			else
				return VInfoNode::create(n);
		}
	}

	return VInfo_ptr();
}

void NodePathWidget::loadMenu(const QPoint& pos,VInfo_ptr p)
{
    Q_ASSERT(mode_ == GuiMode);
    if(mode_ == TextMode)
        return;

    if(p && p->node())
	{
		QList<QAction*> acLst;
		VNode* node=p->node();

        for(int i=0; i < node->numOfChildren(); i++)
		{
			QAction *ac=new QAction(node->childAt(i)->name(),this);
			ac->setData(i);
			acLst << ac;
		}

		if(acLst.count() > 0)
		{
#ifdef _UI_NODEPATHWIDGET_DEBUG
            UiLog().info() << "NodePathWidget::loadMenu";
#endif
            
            if(QAction *ac=QMenu::exec(acLst,pos,acLst.front(),this))
			{
				int idx=ac->data().toInt();
				VInfo_ptr res=VInfoNode::create(node->childAt(idx));
				Q_EMIT selected(res);
			}
	    }

	    Q_FOREACH(QAction* ac,acLst)
		{
			delete ac;
		}
	}
}

void NodePathWidget::notifyBeginNodeChange(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect,const VNodeChange&)
{
    Q_ASSERT(mode_ == GuiMode);
    if(mode_ == TextMode)
        return;

#if 0
    if(!active_)
		return;
#endif

	//Check if there is data in info
    if(info_ && !info_->isServer() && info_->node())
	{
		//TODO: MAKE IT SAFE!!!!

		//State changed
		if(std::find(aspect.begin(),aspect.end(),ecf::Aspect::STATE) != aspect.end() ||
		   std::find(aspect.begin(),aspect.end(),ecf::Aspect::SUSPENDED) != aspect.end())
		{
			std::vector<VNode*> nodes=info_->node()->ancestors(VNode::ParentToChildSort);
            for(int i=0; i < static_cast<int>(nodes.size()); i++)
			{
                if(nodes[i] == node)
				{
                    if(i < nodeItems_.count())
					{						
                        bc_->reset(i,node->name(),node->stateColour(),node->stateFontColour());
					}
					return;
				}
			}
		}

		//A child was removed or added
		else if(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_NODE) != aspect.end())
		{
			std::vector<VNode*> nodes=info_->node()->ancestors(VNode::ParentToChildSort);
			for(auto & i : nodes)
			{
				if(node == i)
				{
					//Reload everything
					setPath(info_);
				}
			}
		}
	}
}

void NodePathWidget::notifyDefsChanged(ServerHandler* server,const std::vector<ecf::Aspect::Type>& aspect)
{
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UiLog().dbg() << "NodePathWidget::notifyDefsChanged -->";
#endif

    Q_ASSERT(mode_ == GuiMode);
    if(mode_ == TextMode)
        return;
#if 0
    if(!active_)
		return;
#endif

    //Check if there is data in info
    if(info_ && info_->server()  && info_->server() == server)
	{
        UiLog().dbg() << "Server change";

        //State changed
        for(auto it : aspect)
        {
            if(it == ecf::Aspect::STATE || it == ecf::Aspect::SERVER_STATE)
            {
#ifdef _UI_NODEPATHWIDGET_DEBUG
                UiLog().dbg() << "   update server item";
#endif
                if(nodeItems_.count() > 0)
                {
                    bc_->reset(0,server->vRoot()->name(),
						                server->vRoot()->stateColour(),
										server->vRoot()->stateFontColour());
                }

            }
        }
	}
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UiLog().dbg() << "<-- notifyDefsChanged";
#endif
}

//This must be called at the beginning of a reset
void NodePathWidget::notifyBeginServerClear(ServerHandler* server)
{
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UiLog().dbg() << "NodePathWidget::notifyBeginServerClear -->";
#endif
    if(info_)
    {
        if(info_->server() && info_->server() == server)
        {
            setEnabled(false);
        }
    }
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UiLog().dbg() << "<-- notifyBeginServerClear";
#endif
}

//This must be called at the end of a reset
void NodePathWidget::notifyEndServerScan(ServerHandler* server)
{
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UiLog().dbg() << "NodePathWidget::notifyEndServerScan -->";
#endif
    if(info_)
    {
        if(info_->server() && info_->server() == server)
        {
#ifdef _UI_NODEPATHWIDGET_DEBUG
            UiLog().dbg() << "   setEnabled(true)";
#endif
            setEnabled(true);

#ifdef _UI_NODEPATHWIDGET_DEBUG
            UiLog().dbg() << "   regainData";
#endif
            //We try to ressurect the info. We have to do it explicitly because it is not guaranteed that
            //notifyEndServerScan() will be first called on the VInfo then on the breadcrumbs. So it
            //is possible that the node still exists but it is still set to NULL in VInfo.
            info_->regainData();

            //If the info is not available dataLost() must have already been called and
            //the breadcrumbs were reset!
            if(!info_)
                return;

            Q_ASSERT(info_->server() && info_->node());

#ifdef _UI_NODEPATHWIDGET_DEBUG
            UiLog().dbg() << "   reset";
#endif
            reset();
        }
    }

#ifdef _UI_NODEPATHWIDGET_DEBUG
    UiLog().dbg() << "<-- notifyEndServerScan";
#endif
}

void NodePathWidget::notifyServerDelete(ServerHandler* server)
{
    if(info_ && info_->server() ==  server)
	{
		//We do not want to detach ourselves as an observer the from the server. When this function is
		//called the server actually loops through its observers and notify them.
		clear(false);
	}
}

void NodePathWidget::notifyServerConnectState(ServerHandler* server)
{
    //TODO: we need to indicate the state here!
    if(info_ && info_->server() ==  server)
    {
        reset();
    }
}

void NodePathWidget::notifyServerActivityChanged(ServerHandler* /*server*/)
{
    //reset();
}

void NodePathWidget::notifyDataLost(VInfo* info)
{
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UiLog().dbg() << "NodePathWidget::notifyDataLost -->";
#endif

    if(info_ && info_.get() == info)
    {
#ifdef _UI_NODEPATHWIDGET_DEBUG
        UiLog().dbg() << "   clear(true)";
#endif
        clear(true);
    }
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UiLog().dbg() << "<-- notifyDataLost";
#endif
}

void  NodePathWidget::slotRefreshServer()
{
    Q_ASSERT(mode_ == GuiMode);
    if(mode_ == TextMode)
        return;

    if(info_ && info_->server())
	{
		info_->server()->refresh();
	}
}

void NodePathWidget::rerender()
{
	reset();
}

void NodePathWidget::paintEvent(QPaintEvent *)
{
     QStyleOption opt;
     opt.init(this);
     QPainter p(this);
     style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void NodePathWidget::resizeEvent(QResizeEvent *)
{
    bc_->adjustSize(bcWidth());
}

void NodePathWidget::writeSettings(VSettings *vs)
{
	vs->beginGroup("breadcrumbs");
    vs->put("mode",(mode_==TextMode)?"text":"gui");
	vs->endGroup();
}

void NodePathWidget::readSettings(VSettings* vs)
{
	vs->beginGroup("breadcrumbs");
    std::string modeStr=vs->get<std::string>("mode","");

    if(modeStr == "text")
        setMode(TextMode);
    else
        setMode(GuiMode);

	vs->endGroup();
}


