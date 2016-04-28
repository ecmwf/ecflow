/***************************** LICENSE START ***********************************

 Copyright 2014 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#include "NodePathWidget.hpp"

#include <QDebug>
#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygon>
#include <QSizePolicy>
#include <QStyleOption>
#include <QToolButton>
#include <QVector>

#include "VNode.hpp"
#include "VProperty.hpp"
#include "PropertyMapper.hpp"
#include "ServerHandler.hpp"
#include "UserMessage.hpp"
#include "VNState.hpp"
#include "VSState.hpp"
#include "VSettings.hpp"

static std::vector<std::string> propVec;

QColor NodePathItem::disabledBgCol_;
QColor NodePathItem::disabledBorderCol_;
QColor NodePathItem::disabledFontCol_;

//#define _UI_NODEPATHWIDGET_DEBUG

BcWidget::BcWidget(QWidget* parent) : 
    QWidget(parent),
    hPadding_(2),
    vPadding_(1),
	hMargin_(1),
	vMargin_(1),
    triLen_(10),
	gap_(5),
    width_(0),
    itemHeight_(0),
    emptyText_("No selection"),
    useGrad_(true),
    gradLighter_(150)
{
    font_=QFont();
    QFontMetrics fm(font_);

    itemHeight_=fm.height()+2*vPadding_;
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

    reset(items_);
}

BcWidget::~BcWidget()
{
    delete prop_;
}    


void BcWidget::notifyChange(VProperty *p)
{
    updateSettings();
}    
    
void BcWidget::updateSettings()
{
    //if(VProperty *p=prop_->find("view.common.node_style"))
    //    node_style=p->value().toString()';
    if(VProperty *p=prop_->find("view.common.node_gradient"))
        useGrad_=p->value().toBool();
}    

void BcWidget::clear()
{
    items_.clear();
    reset(items_);
}
    
void BcWidget::reset(int idx,QString text,QColor bgCol,QColor fontCol)
{
    if(idx >=0 && idx < items_.count())
    {
        bool newText=(text != items_.at(idx)->text_);
        
        if(newText)
            items_.at(idx)->text_=text;
        
        items_.at(idx)->bgCol_=bgCol;
        items_.at(idx)->fontCol_=fontCol;
        items_.at(idx)->borderCol_=bgCol.darker(125);
        
        if(newText)
           reset(items_);
        else
        {
            updatePixmap(idx); 
            update();
        }    
    }
}    
    
void BcWidget::reset(QList<NodePathItem*> items)
{
    items_=items;
    
    QFontMetrics fm(font_);
    int xp=hMargin_;
    int yp=vMargin_;

    for(int i=0; i < items_.count(); i++)
    {
        int len=fm.width(items_.at(i)->text_);
        
        QRect textRect;
        items_.at(i)->borderCol_=items_.at(i)->bgCol_.darker(125);
                
        QVector<QPoint> vec;
        QVector<QPoint> menuVec;
        if(i ==0)
        {
            textRect=QRect(xp+hPadding_,yp,len,itemHeight_);
            
            vec << QPoint(xp,yp);            
            vec << QPoint(xp+len,yp);
            vec << QPoint(xp+len+triLen_,yp+itemHeight_/2);
            vec << QPoint(xp+len,yp+itemHeight_);
            vec << QPoint(xp,yp+itemHeight_);
            
            xp+=len+triLen_+gap_;
        }
        else
        {
            textRect=QRect(xp+hPadding_,yp,len,itemHeight_);
            
            vec << QPoint(xp-triLen_,yp);            
            vec << QPoint(xp+len,yp);
            vec << QPoint(xp+len+triLen_,yp+itemHeight_/2);
            vec << QPoint(xp+len,yp+itemHeight_);
            vec << QPoint(xp-triLen_,yp+itemHeight_);
            vec << QPoint(xp,yp+itemHeight_/2);
            
            xp+=len+triLen_+gap_;
        } 
        

       // if(i < items_.count()-1)
       // 	xp+=1;

        items_.at(i)->shape_=QPolygon(vec);
        items_.at(i)->textRect_=textRect;
    }

    width_=xp+hMargin_;
    
    if(items_.count() ==0)
    {
        int len=fm.width(emptyText_);
        emptyRect_=QRect(xp+hPadding_,yp,len,itemHeight_);
        width_=xp+len+4;
    }    
    
    crePixmap();
    
    resize(width_,height_);

    update();
}  


void BcWidget::crePixmap()
{        
    pix_=QPixmap(width_,height_);
    pix_.fill(Qt::transparent);
    
    QPainter painter(&pix_);
    painter.setRenderHints(QPainter::Antialiasing,true);
    
    if(items_.count() == 0)
    {
        painter.setPen(Qt::black);
        painter.drawText(emptyRect_,Qt::AlignHCenter | Qt::AlignVCenter, emptyText_);
    }
    else
    {    
        for(int i=0; i < items_.count(); i++)
        {
            items_.at(i)->enabled_=isEnabled();
            items_.at(i)->draw(&painter,useGrad_,gradLighter_);
        }
    }    
}  

void BcWidget::updatePixmap(int idx)
{
    if(idx >=0 && idx < items_.count())
    {
         QPainter painter(&pix_);
         painter.setRenderHints(QPainter::Antialiasing,true);
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

}

void BcWidget::mousePressEvent(QMouseEvent *event)
{   
	if(event->button() != Qt::RightButton && event->button() != Qt::LeftButton)
		return;

	for(int i=0; i < items_.count(); i++)
    {
		if(items_.at(i)->shape_.containsPoint(event->pos(),Qt::OddEvenFill))
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

NodePathItem::NodePathItem(int index,QString text,QColor bgCol,QColor fontCol,bool hasMenu,bool current) :
    index_(index),
    text_(text),
    bgCol_(bgCol),
    fontCol_(fontCol),
    current_(current),
    hasMenu_(hasMenu),
    enabled_(true)
{
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

void NodePathItem::setCurrent(bool)
{
}

void NodePathItem::draw(QPainter  *painter,bool useGrad,int lighter)
{    
   /* if(current_)
    {
    	painter->setPen(QPen(Qt::black,2));
    }
    else
    {
    	painter->setPen(QPen(borderCol_,0));
    }*/

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

    painter->setPen(QPen(border,0));
    
    QBrush bgBrush;
       
    if(useGrad)
    {
        QColor bgLight=bg.lighter(lighter);
        grad_.setColorAt(0,bgLight);
        grad_.setColorAt(1,bg);
        bgBrush=QBrush(grad_);
    }
    else
        bgBrush=QBrush(bg);
    
    painter->setBrush(bgBrush);
    painter->drawPolygon(shape_);

    /*if(current_)
    {
    	painter->setPen(QPen(borderCol_,0));
    }*/

    painter->setPen(fontCol);
    painter->drawText(textRect_,Qt::AlignVCenter | Qt::AlignHCenter,text_);  
    
}


//=============================================================
//
//  NodePathWidget
//
//=============================================================

NodePathWidget::NodePathWidget(QWidget *parent) :
  QWidget(parent),
  reloadTb_(0),
  active_(true)
{
	setProperty("breadcrumbs","1");

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
    
    reloadTb_=new QToolButton(this);
    //reloadTb_->setDefaultAction(actionReload_);
    reloadTb_->setIcon(QPixmap(":/viewer/reload_one.svg"));
    reloadTb_->setToolTip(tr("Refresh server"));
    reloadTb_->setAutoRaise(true);
    //reloadTb_->setIconSize(QSize(20,20));
    reloadTb_->setObjectName("pathIconTb");

    connect(reloadTb_,SIGNAL(clicked()),
            this,SLOT(slotRefreshServer()));

    layout_->addWidget(reloadTb_);
}

NodePathWidget::~NodePathWidget()
{
	clear(true);
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

    reloadTb_->setEnabled(false);
    reloadTb_->setToolTip("");
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

void NodePathWidget::active(bool active)
{
	if(active_ != active)
		active_=active;

	if(active_)
	{
		setVisible(true);
	}
	else
	{
		setVisible(false);
		clear();
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

        reloadTb_->setToolTip("");
        reloadTb_->setEnabled(false);
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

void NodePathWidget::setPath(QString)
{
	if(!active_)
	  		return;
}

void NodePathWidget::setPath(VInfo_ptr info)
{
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UserMessage::debug("NodePathWidget::setPath -->");
#endif

    setEnabled(true);

    if(!active_)
        return;

  	ServerHandler *server=0;
  	bool sameServer=false;

  	VInfo_ptr info_ori=info_;

  	adjust(info,&server,sameServer);

    if(!info_ ||
  	  (!info_->isServer() && !info_->isNode()))
  	{
  		clear();
  		return;
  	}
  	else
    {    
        clearItems();
    }
   
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
        UserMessage::debug("   state=" + n->stateName().toStdString());
#endif
		QColor fontCol=n->stateFontColour();
		name=n->name();
		bool hasChildren=hasChildren=(n->numOfChildren() >0);

	    nodeItem=new NodePathItem(i,name,col,fontCol,hasChildren,(i == lst.size()-1)?true:false);
		nodeItems_ << nodeItem;           
	}

	bc_->reset(nodeItems_);

#ifdef _UI_NODEPATHWIDGET_DEBUG
    UserMessage::debug("<-- NodePathWidget::setPath");
#endif
}

void  NodePathWidget::slotNodeSelected(int idx)
{
	if(idx != -1)
	{
		Q_EMIT selected(nodeAt(idx));
	}
}

void  NodePathWidget::slotMenuSelected(int idx,QPoint bcPos)
{
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
    UserMessage::debug("NodePathWidget::nodeAt idx=" + boost::lexical_cast<std::string>(idx));
#endif
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
	if(p && p->node())
	{
		QList<QAction*> acLst;
		VNode* node=p->node();

		for(unsigned int i=0; i < node->numOfChildren(); i++)
		{
			QAction *ac=new QAction(node->childAt(i)->name(),this);
			ac->setData(i);
			acLst << ac;
		}

		if(acLst.count() > 0)
		{
#ifdef _UI_NODEPATHWIDGET_DEBUG
            UserMessage::message(UserMessage::DBG,false,"NodePathWidget::loadMenu");
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
	if(!active_)
		return;

	//Check if there is data in info
    if(info_ && info_->isNode() && info_->node())
	{
		//TODO: MAKE IT SAFE!!!!

		//State changed
		if(std::find(aspect.begin(),aspect.end(),ecf::Aspect::STATE) != aspect.end() ||
		   std::find(aspect.begin(),aspect.end(),ecf::Aspect::SUSPENDED) != aspect.end())
		{
			std::vector<VNode*> nodes=info_->node()->ancestors(VNode::ParentToChildSort);
			for(unsigned int i=0; i < nodes.size(); i++)
			{
				if(nodes.at(i) == node)
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
			for(unsigned int i=0; i < nodes.size(); i++)
			{
				if(node == nodes.at(i))
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
    UserMessage::debug("NodePathWidget::notifyDefsChanged -->");
#endif

    if(!active_)
		return;

	//Check if there is data in inf0
    if(info_ && info_->server()  && info_->server() == server)
	{
		//State changed
		if(std::find(aspect.begin(),aspect.end(),ecf::Aspect::SERVER_STATE) != aspect.end())
		{
			if(nodeItems_.count() > 0)
			{
				bc_->reset(0,server->vRoot()->name(),
						                server->vRoot()->stateColour(),
										server->vRoot()->stateFontColour());
			}

		}
	}

}

//This must be called at the beginning of a reset
void NodePathWidget::notifyBeginServerClear(ServerHandler* server)
{
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UserMessage::debug("NodePathWidget::notifyBeginServerClear -->");
#endif
    if(info_)
    {
        if(info_->server() && info_->server() == server)
        {
            setEnabled(false);
        }
    }
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UserMessage::debug("<-- NodePathWidget::notifyBeginServerClear");
#endif
}

//This must be called at the end of a reset
void NodePathWidget::notifyEndServerScan(ServerHandler* server)
{
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UserMessage::debug("NodePathWidget::notifyEndServerScan -->");
#endif

    if(info_)
    {
        if(info_->server() && info_->server() == server)
        {
#ifdef _UI_NODEPATHWIDGET_DEBUG
            UserMessage::debug("   setEnabled(true)");
#endif

            setEnabled(true);

#ifdef _UI_NODEPATHWIDGET_DEBUG
            UserMessage::debug("   regainData");
#endif
            //We try to ressurect the info. We have to do it explicitly because it is not guaranteed
            //the notifyEndServerScan() will be first called on the VInfo then on the breadcrumbs. So it
            //is possible that the node still exists but it is still set to NULL in VInfo.
            info_->regainData();

            //If the info is not available dataLost() must have already been called and
            //the breadcrumbs were reset!
            if(!info_)
                return;

            Q_ASSERT(info_->server() && info_->node());

#ifdef _UI_NODEPATHWIDGET_DEBUG
            UserMessage::debug("   reset");
#endif
            reset();
        }
    }

#ifdef _UI_NODEPATHWIDGET_DEBUG
    UserMessage::debug("<-- NodePathWidget::notifyEndServerScan");
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
    UserMessage::debug("NodePathWidget::notifyDataLost -->");
#endif
    if(info_ && info_.get() == info)
    {
#ifdef _UI_NODEPATHWIDGET_DEBUG
        UserMessage::debug("   clear(true)");
#endif
        clear(true);
    }
#ifdef _UI_NODEPATHWIDGET_DEBUG
    UserMessage::debug("<-- NodePathWidget::notifyDataLost");
#endif
}

void  NodePathWidget::slotRefreshServer()
{
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


void NodePathWidget::writeSettings(VSettings *vs)
{
	vs->beginGroup("breadcrumbs");
	vs->put("active",active_);
	vs->endGroup();
}

void NodePathWidget::readSettings(VSettings* vs)
{
	vs->beginGroup("breadcrumbs");
	int ival;

	ival=vs->get<int>("active",1);
	active((ival==1)?true:false);

	vs->endGroup();
}






