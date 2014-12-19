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

#include "Node.hpp"
#include "ServerHandler.hpp"
#include "VNState.hpp"
#include "VSState.hpp"

NodePathNodeItem::NodePathNodeItem(int index,QString name,QColor col,bool selected,QWidget * parent) :
  NodePathItem(NodeType,index,parent),
  col_(col),
  current_(selected)
{
	//Stylesheet
	qss_="QToolButton {\
		        border: BORDER;\
			    border-radius: 0px; \
		     	padding: 0px; \
				background: BASE-COLOR;\
			    font-weight: FONT;\
				color: TEXT-COLOR; \
		 }\
		     QToolButton:hover{\
		    	 border: BORDER;\
		    	 border-radius: 0px; \
		    	 padding: 0px; \
		    	 background: HOVER-COLOR;\
			     font-weight: FONT;\
		 }";

	setText(name);
	resetStyle();
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

void NodePathNodeItem::resetStyle()
{
	QString st=qss_;
	//Colour
	st.replace("BASE-COLOR",col_.name()).replace("HOVER-COLOR",col_.darker(125).name());

	//Font
	st.replace("FONT",current_?"bold":"normal");

	//Text color
	st.replace("TEXT-COLOR",isDark(col_)?"white":"black");

	//Border
	st.replace("BORDER",current_?"2px solid rgb(40, 40, 40)":"1px solid rgb(160, 160, 160)");

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
	setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	setIcon(QPixmap(":/viewer/server.svg"));
}



NodePathMenuItem::NodePathMenuItem(int index,QWidget * parent) :
  NodePathItem(MenuType, index,parent)
{
	setObjectName("pathMenuTb");
};




//=============================================================
//
//  NodePathWidget
//
//=============================================================

NodePathWidget::NodePathWidget(QWidget *parent) :
  QWidget(parent),
  stayInParent_(true),
  infoIndex_(-1)
{
	setProperty("breadcrumbs","1");

	layout_=new QHBoxLayout(this);
	layout_->setSpacing(0);
	layout_->setContentsMargins(6,0,6,0);
	setLayout(layout_);

	//QFont f(QApplication::font());
	//QFontMetrics fm(f);
}

NodePathWidget::~NodePathWidget()
{
	clearLayout();
}


void  NodePathWidget::clearContents()
{
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


void NodePathWidget::slotContextMenu(const QPoint& pos)
{

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

QList<Node*> NodePathWidget::toNodeList(VInfo_ptr info)
{
	QList<Node*> lst;
	if(info->isServer() || !info->node())
		return lst;

	Node *node=info->node();
	lst << node;
	Node *f=node;
	while(f->parent())
	{
		f=f->parent();
		lst.prepend(f);
	}

	return lst;
}

//Find p1 in p2
int NodePathWidget::findInPath(VInfo_ptr p1,VInfo_ptr p2,bool sameServer)
{
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
	QList<Node*> lst1=toNodeList(p1);
	QList<Node*> lst2=toNodeList(p2);

	if(lst1.count() > lst2.count())
		return -1;

	for(int i=0; i < lst1.count(); i++)
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

	return idx;
}



void NodePathWidget::setPath(QString)
{

}


void NodePathWidget::setPath(VInfo_ptr info)
{
  	ServerHandler *server=0;
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
  	  		if(info_ && info_->server())
  	  				info_->server()->removeNodeObserver(this);

  	  		info_=info;
  	  		infoIndex_=-1;
  	  		clearContents();
  	  		return;
  	}

  	//Check "stay in parent" mode
  	if(stayInParent_)
  	{
  		//If the new path is part of the current path we just set the
  		//current node index and return.
  		int idx=findInPath(info,info_,sameServer);
  		if(idx != -1)
  		{
  			infoIndex(idx);
  			return;
  		}
  	}

  	//We do not stay in parent mode or the new path is not part of the current one.

  	//Now it is safe to do this assignment
  	info_=info;

	//Get the node list + prepend the server
  	QList<Node*> lst=toNodeList(info_);
  	lst.prepend(0);//indicates the server

  	//--------------------------------------------
  	// Cleaning (as much as needed)
  	//--------------------------------------------

  	//Clear the layout (widgets are not deleted in this step!)
  	clearLayout();

	//Delete
	int cnt=nodeItems_.count();
	for(int i=lst.count(); i < cnt; i++)
	{
			delete nodeItems_.takeLast();

	}
	cnt=menuItems_.count();
	for(int i=lst.count(); i < cnt; i++)
	{
		delete menuItems_.takeLast();

	}

	//--------------------------------------------
	// Reset/rebuild the contents
	//--------------------------------------------

	for(int i=0; i < lst.count(); i++)
	{
			//---------------------------
			// Create node/server item
			//---------------------------

			QColor col;
			QString name;
			NodePathNodeItem* nodeItem=0;

			//Server
			if(i==0)
			{
				col=QColor(255,255,255);
				col=VSState::toColour(server).name();
				name=QString::fromStdString(server->name());
			}
			//Node
			else
			{
				Node *n=lst.at(i);
				col=VNState::toColour(n).name();
				name=QString::fromStdString(n->name());
			}

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

	//Set the current node index (used only in "stay in parent" mode). If we are here it must be the last node!
	infoIndex(lst.count()-1);


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

VInfo_ptr NodePathWidget::nodeAt(int idx)
{
	if(info_ && info_->server())
	{
		ServerHandler* server=info_->server();
		if(Node *node=info_->node())
		{
			for(int i=nodeItems_.count()-1; i > idx && i >=1; i--)
			{
				node=node->parent();
			}

			if(node)
			{
				VInfo_ptr res(VInfo::make(node,server));
				qDebug() << "selected" << node->absNodePath().c_str();
				return res;
			}
		}

		VInfo_ptr res(VInfo::make(server));
		qDebug() << "selected" << server->name().c_str();
		return res;
	}

	return VInfo_ptr();
}

void NodePathWidget::loadMenu(const QPoint& pos,VInfo_ptr p)
{
	QList<QAction*> acLst;

	if(p->isServer())
	{
		ServerHandler* server=p->server();

		int n=server->numSuites();
		for(unsigned int i=0; i < n; i++)
		{
			node_ptr node=server->suiteAt(i);
			if(node)
			{
				QAction *ac=new QAction(QString::fromStdString(node->name()),this);
				ac->setData(i);
				acLst << ac;
			}
		}
		if(acLst.count() > 0)
		{
			if(QAction *ac=QMenu::exec(acLst,pos,acLst.front(),this))
			{
				int idx=ac->data().toInt();
				VInfo_ptr res(VInfo::make(server->suiteAt(idx).get(),server));
				Q_EMIT selected(res);
			}
		}

	}
	else if(Node *node=p->node())
	{
		std::vector<node_ptr> nodes;
		node->immediateChildren(nodes);
		for(unsigned int i=0; i < nodes.size(); i++)
		{
			QAction *ac=new QAction(QString::fromStdString(nodes.at(i)->name()),this);
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
				VInfo_ptr res(VInfo::make(nodes.at(idx).get(),server));
				Q_EMIT selected(res);
			}
		}
	}

	Q_FOREACH(QAction* ac,acLst)
	{
		delete ac;
	}
}

void NodePathWidget::paintEvent(QPaintEvent *)
 {
     QStyleOption opt;
     opt.init(this);
     QPainter p(this);
     style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
 }
