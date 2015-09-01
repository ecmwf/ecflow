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
#include <QStyleOption>
#include <QPainter>
#include <QVector>

#include "VNode.hpp"
#include "ServerHandler.hpp"
#include "VNState.hpp"
#include "VSState.hpp"
#include "VSettings.hpp"

NodePathNodeItem::NodePathNodeItem(int index,QString name,QColor col,bool selected,QWidget * parent) :
  NodePathItem(NodeType,index,parent),
  col_(col),
  current_(selected)
{
	//Stylesheet

	qss_="QToolButton {\
			    border-radius: 0px; \
		     	padding: 0px; \
				border: BORDER;\
			    font-weight: FONT;\
				background: BASE-COLOR;\
			    color: TEXT-COLOR; \
		 }\
		     QToolButton:hover{\
		    	 border: BORDER;\
		    	 border-radius: 0px; \
		    	 padding: 0px; \
		    	 background: HOVER-COLOR;\
			     font-weight: FONT; \
		 }";

	qssSelected_="QToolButton {\
			    border: BORDER;\
				border-radius: 0px; \
		     	padding: 0px; \
			    font-weight: FONT;\
				background: BASE-COLOR;\
			    color: TEXT-COLOR; \
		 }\
		     QToolButton:hover{\
		    	 border: BORDER;\
		    	 border-radius: 0px; \
		    	 padding: 0px; \
		    	 background: HOVER-COLOR;\
		 }";

	setText(name+" ");
	resetStyle();

	QFont f;
	f.setPointSize(8);
	setFont(f);
};

void NodePathNodeItem::reset(QString name,QColor col,bool current)
{
	if(text() != name)
		setText(name);

	if(col_ != col || current_!=current)
	{
		col_=col;
		current_=current;
		resetStyle();
	}
}

void NodePathNodeItem::reset(QString name,QColor col)
{
	if(text() != name)
		setText(name);

	if(col_ != col)
	{
		col_=col;
		resetStyle();
	}
}


void NodePathNodeItem::resetStyle()
{
	QString st;

	//Selected
	if(current_)
	{
		st=qssSelected_;

		st.replace("BASE-COLOR",col_.name());

		//Font
		st.replace("FONT","bold");

		//Text color
		st.replace("TEXT-COLOR","black");

		//Border
		st.replace("BORDER","1px solid black");
	}
	//Non selected
	else
	{
		st=qss_;
		st.replace("BASE-COLOR",col_.name());

		//Font
		st.replace("FONT","normal");

		//Text color
		st.replace("TEXT-COLOR","black");

		//Border
		st.replace("BORDER","1px solid rgb(180,180,180)");    //+ col_.name());

	}

	st.replace("HOVER-COLOR",col_.darker(104).name());


	//Colour
	//st.replace("BASE-COLOR",col_.name()).replace("HOVER-COLOR",col_.darker(125).name());

	/*st.replace("BASE-COLOR",col_.name());
	st.replace("HOVER-COLOR","rgb(230,230,230)");

	//Font
	st.replace("FONT",current_?"bold":"normal");

	//Text color
	//st.replace("TEXT-COLOR",isDark(col_)?"white":"black");
	st.replace("TEXT-COLOR","black");

	//Border
	//st.replace("BORDER",current_?"2px solid rgb(40, 40, 40)":"1px solid rgb(160, 160, 160)");

	st.replace("BORDER",current_?"2px solid "+ col_.name():"2px solid " +  col_.name());*/


	setStyleSheet(st);
}

bool NodePathNodeItem::isDark(QColor col) const
{
	int h=col.hue();
	int lg=col.lightness();
	bool darkBg=false;
	if(h < 10 || h > 220)
	{
		if(lg <180)
				darkBg=true;
	}
	else if(lg < 110)
	{
		darkBg=true;
	}
	return darkBg;
}

void NodePathNodeItem::colour(QColor col)
{
	if(col_!= col)
	{
		col_=col;
		resetStyle();
	}
}

void NodePathNodeItem::current(bool current)
{
	if(current_!=current)
	{
		current_=current;
		resetStyle();
	}
}


NodePathServerItem::NodePathServerItem(int index,QString name,QColor col,bool selected,QWidget * parent) :
  NodePathNodeItem(index,name,col,selected,parent)
{
	//setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	//setIcon(QPixmap(":/viewer/server.svg"));
}



NodePathMenuItem::NodePathMenuItem(int index,QWidget * parent) :
  NodePathItem(MenuType, index,parent)
{
	/*if(index_==0)
		setText("/");
	else
		setText("/");*/
	setObjectName("pathMenuTb");
};




//=============================================================
//
//  NodePathWidget
//
//=============================================================

NodePathWidget::NodePathWidget(QWidget *parent) :
  QWidget(parent),
  reloadTb_(0),
  stayInParent_(false),
  infoIndex_(-1),
  active_(true)
{
	setProperty("breadcrumbs","1");

	layout_=new QHBoxLayout(this);
	layout_->setSpacing(0);
    layout_->setContentsMargins(6,3,4,2);
	setLayout(layout_);

	//QFont f(QApplication::font());
	//QFontMetrics fm(f);
}

NodePathWidget::~NodePathWidget()
{
	clear(true);
}

void NodePathWidget::clear(bool detachObservers)
{
	if(detachObservers && info_ && info_->server())
	{
		info_->server()->removeNodeObserver(this);
		info_->server()->removeServerObserver(this);
	}

	info_.reset();
	infoIndex_=-1;

	clearLayout();

	int cnt=nodeItems_.count();
	for(int i=0; i < cnt; i++)
	{
			delete nodeItems_.takeLast();

	}
	cnt=menuItems_.count();
	for(int i=0; i < cnt; i++)
	{
		delete menuItems_.takeLast();

	}
}

//We delete the layout items but not the widget they contain
void NodePathWidget::clearLayout()
{
  	QLayoutItem *item;
 	while( (item = layout_->takeAt(0)) != 0)
	{
     	if(QWidget *w=item->widget())
     	{
     		layout_->removeWidget(w);
     		//delete w;
     	}
     	delete item;
 	}
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

void NodePathWidget::infoIndex(int idx)
{
	if(infoIndex_ >=0 && infoIndex_  < nodeItems_.count())
	{
		nodeItems_.at(infoIndex_)->current(false);
	}

	infoIndex_=idx;

	if(infoIndex_ >=0 && infoIndex_  < nodeItems_.count())
	{
			nodeItems_.at(infoIndex_)->current(true);
	}
}

//Find p1 in p2
int NodePathWidget::findInPath(VInfo_ptr p1,VInfo_ptr p2,bool sameServer)
{
	return -1;

	//TODO: reimplement if needed

	/*if(!p1 || !p1.get())
		return -1;

	//The servers are the same
	if(sameServer)
	{
		if(p1->isServer())
			return 0;
	}
	else
	{
		return -1;
	}

	int idx=-1;
	std::vector<VNode*> lst1=p1->ancestors(VInfo::ParentToChildOrder);
	std::vector<VNode*> lst2=p2->ancestors(VInfo::ParentToChildOrder);

	if(lst1.size() > lst2.size())
		return -1;

	for(unsigned int i=0; i < lst1.size(); i++)
	{
			qDebug() << lst1.at(i) << lst2.at(i);

			if(lst1.at(i) != lst2.at(i))
			{
				idx=-1;
				break;
			}
			else
				idx=i+1;
	}

	return idx;*/
}

void NodePathWidget::adjust(VInfo_ptr info,ServerHandler** serverOut,bool &sameServer)
{
	ServerHandler* server=0;

  	//Check if there is data in info
  	if(info.get())
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
  	}

  	//Set the info
  	info_=info;

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
  	if(!active_)
  		return;

  	ServerHandler *server=0;
  	bool sameServer=false;

  	VInfo_ptr info_ori=info_;

  	adjust(info,&server,sameServer);

  	if(!info_ || !info_.get() ||
  	  (!info_->isServer() && !info_->isNode()))
  	{
  		clear();
  		return;
  	}

	/*ServerHandler *server=0;
  	bool sameServer=false;

  	//Check if there is data in info
  	if(info.get())
  	{
  		server=info->server();

  		sameServer=(info_)?(info_->server() == server):false;

  		//Handle observers
  		if(!sameServer)
  		{
  			if(info_ && info_->server())
  				info_->server()->removeNodeObserver(this);

  			info->server()->addNodeObserver(this);
  		}
  	}
  	//If the there is no data we clean everything and return
  	else
  	{
  	  		clear();
  			//if(info_ && info_->server())
  	  		//		info_->server()->removeNodeObserver(this);

  	  		//info_=info;
  	  		//infoIndex_=-1;
  	  		//clearContents();
  	  		return;
  	}*/

  	//Check "stay in parent" mode
  	if(stayInParent_)
  	{
  		//If the new path is part of the current path we just set the
  		//current node index and return.
  		int idx=findInPath(info_,info_ori,sameServer);
  		if(idx != -1)
  		{
  			infoIndex(idx);
  			return;
  		}
  	}

  	//We do not stay in parent mode or the new path is not part of the current one.

  	//Now it is safe to do this assignment //????
  	//info_=info;

	//Get the node list including the server
  	std::vector<VNode*> lst;
  	if(info_->node())
  	{
  		lst=info_->node()->ancestors(VNode::ParentToChildSort);
  	}

  	//--------------------------------------------
  	// Cleaning (as much as needed)
  	//--------------------------------------------

  	//Clear the layout (widgets are not deleted in this step!)
  	clearLayout();

	//Delete
	int cnt=nodeItems_.count();
	for(unsigned int i=lst.size(); i < cnt; i++)
	{
			delete nodeItems_.takeLast();

	}
	cnt=menuItems_.count();
	for(unsigned int i=lst.size(); i < cnt; i++)
	{
		delete menuItems_.takeLast();

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
		NodePathNodeItem* nodeItem=0;

		VNode *n=lst.at(i);
		col=n->stateColour();
		name=n->name();
		bool hasChildren=hasChildren=(n->numOfChildren() >0);

		//Server
		/*if(i==0)
		{
			col=QColor(255,255,255);
			col=VSState::toColour(server);
			name=QString::fromStdString(server->name());
			hasChildren=true;
		}
			//Node
			else
			{
				VNode *n=lst.at(i);
				col=n->stateColour();
				name=n->name();
				hasChildren=(n->numOfChildren() >0);
			}
	*/
		if(i < nodeItems_.count())
		{
			nodeItem=nodeItems_.at(i);
			nodeItem->reset(name,col,false);
		}
		else
		{
			//Server
			if(i==0)
			{
				nodeItem=new NodePathServerItem(i,name,col,false,this);
			}
			//Node
			else
			{
				nodeItem=new NodePathNodeItem(i,name,col,false,this);
			}
			nodeItems_ << nodeItem;
			connect(nodeItem,SIGNAL(clicked()),
			  		     this,SLOT(nodeItemSelected()));
		}

		layout_->addWidget(nodeItem);

		//-----------------------------------------
		// Create sub item (connector or menu)
		//-----------------------------------------

		if(hasChildren)
		{
			NodePathMenuItem* menuItem=0;

			if(i >= menuItems_.count())
			{
				menuItem= new NodePathMenuItem(i,this);

				menuItems_ << menuItem;
				connect(menuItem,SIGNAL(clicked()),
					   this,SLOT(menuItemSelected()));
			}
			else
			{
				menuItem=menuItems_.at(i);
			}

			layout_->addWidget(menuItem);
		}
	}

	layout_->addStretch(1);

	//Reload
	//if(actionReload_)
	//{
		if(!reloadTb_)
		{
			reloadTb_=new QToolButton(this);
			//reloadTb_->setDefaultAction(actionReload_);
			reloadTb_->setIcon(QPixmap(":/viewer/reload.svg"));
			reloadTb_->setToolTip(tr("Refresh server"));
			reloadTb_->setAutoRaise(true);
				//reloadTb_->setIconSize(QSize(20,20));
			reloadTb_->setObjectName("pathIconTb");

			connect(reloadTb_,SIGNAL(clicked()),
				this,SLOT(slotRefreshServer()));

		}
		layout_->addWidget(reloadTb_);
	//}

	//Set the current node index (used only in "stay in parent" mode). If we are here it must be the last node!
	infoIndex(lst.size()-1);
}

void  NodePathWidget::nodeItemSelected()
{
	NodePathNodeItem* item=static_cast<NodePathNodeItem*>(sender());
	int idx=nodeItems_.indexOf(item);
	if(idx != -1 && idx != infoIndex_)
	{
		Q_EMIT selected(nodeAt(idx));
	}
}

void  NodePathWidget::menuItemSelected()
{
	NodePathMenuItem* item=static_cast<NodePathMenuItem*>(sender());
	int idx=menuItems_.indexOf(item);
	if(idx != -1)
	{
		loadMenu(mapToGlobal(item->pos()+QPoint(0,item->size().height()/2)),nodeAt(idx));
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
	qDebug() << "nodeAt()" << idx;

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

		/*
		//The last item
		if(idx == nodeItems_.count()-1)
			return info_;

		//Server
		if(idx == 0)
		{
			VInfo_ptr res=VInfoServer::make(server);
			qDebug() << "selected" << server->name().c_str();
			return res;
		}
		//Node
		else if(info_->node())
		{
			std::vector<VNode*> nodes=info_->node()->ancestors(VNode::ParentToChildSort);
			if(idx-1 < nodes.size())
			{
				VInfo_ptr res=VInfoNode::make(nodes.at(idx-1));
				qDebug() << "selected" << nodes.at(idx-1)->node()->absNodePath().c_str();
				return res;
			}
		}*/
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

	/*
	if(p->isServer() && p->server())
	{
		ServerHandler* server=p->server();
		VServer* root=server->vRoot();

		for(unsigned int i=0; i < root->numOfChildren(); i++)
		{
			if(VNode* node=root->childAt(i))
			{
				QAction *ac=new QAction(node->name(),this);
				ac->setData(i);
				acLst << ac;
			}
		}
		if(acLst.count() > 0)
		{
			if(QAction *ac=QMenu::exec(acLst,pos,acLst.front(),this))
			{
				int idx=ac->data().toInt();
				VInfo_ptr res(VInfo::make(root->childAt(idx),server));
				Q_EMIT selected(res);
			}
		}

	}
	else if(VNode *node=p->node())
	{
		for(unsigned int i=0; i < node->numOfChildren(); i++)
		{
			QAction *ac=new QAction(node->childAt(i)->name(),this);
			ac->setData(i);
			acLst << ac;
		}

		if(acLst.count() > 0)
		{
			if(QAction *ac=QMenu::exec(acLst,pos,acLst.front(),this))
			{
				int idx=ac->data().toInt();
				//ServerHandler* server=p->server();
				ServerHandler* server=info_->server();
				VInfo_ptr res(VInfo::make(node->childAt(idx),server));
				Q_EMIT selected(res);
			}
		}
	}

	Q_FOREACH(QAction* ac,acLst)
	{
		delete ac;
	}*/
}

void NodePathWidget::notifyBeginNodeChange(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect,const VNodeChange&)
{
	if(!active_)
		return;

	//Check if there is data in info
	if(info_.get() && info_->isNode() && info_->node())
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
						nodeItems_.at(i)->reset(node->name(),node->stateColour());
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
	if(!active_)
		return;

	//Check if there is data in inf0
	if(info_.get() && info_->server()  && info_->server() == server)
	{
		//State changed
		if(std::find(aspect.begin(),aspect.end(),ecf::Aspect::SERVER_STATE) != aspect.end())
		{
			if(nodeItems_.count() > 0)
			{
				nodeItems_.at(0)->reset(server->vRoot()->name(),
						                server->vRoot()->stateColour());
			}

		}
	}

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
	reset();
}

void NodePathWidget::notifyServerActivityChanged(ServerHandler* server)
{
	reset();
}


void  NodePathWidget::slotRefreshServer()
{
	if(info_ && info_->server())
	{
		info_->server()->refresh();
	}
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
	vs->put("stayInParent",stayInParent_);
	vs->endGroup();
}

void NodePathWidget::readSettings(VSettings* vs)
{
	vs->beginGroup("breadcrumbs");
	int ival;

	ival=vs->get<int>("active",1);
	active((ival==1)?true:false);

	//ival=vs->get<int>("stayInParent",1);
	//stayInParent_=(ival==1)?true:false;

	vs->endGroup();
}






