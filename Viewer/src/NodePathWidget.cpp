/***************************** LICENSE START ***********************************

 Copyright 2013 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#include "NodePathWidget.hpp"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QHBoxLayout>
#include <QMenu>
#include <QSignalMapper>
#include <QStyleOption>
#include <QPainter>
#include <QPalette>
#include <QToolButton>


#include "Node.hpp"
#include "ServerHandler.hpp"
#include "VNState.hpp"

//QList<MvQContextItem*> NodePathWidget::cmTbItems_;
//QList<MvQContextItem*> NodePathWidget::cmMenuItems_;


NodePathWidgetItem::~NodePathWidgetItem()
{
	    if(menuTb_) menuTb_->deleteLater();
	    if(nameTb_) nameTb_->deleteLater();
	    if(menu_) menu_->deleteLater();
}

//=============================================================
//
//  NodePathWidget
//
//=============================================================

NodePathWidget::NodePathWidget(QWidget *parent) :
  QWidget(parent),
  actionReload_(0),
  reloadTb_(0),
  editable_(true),
  paddingX_(2),
  paddingY_(2),
  textPaddingX_(2),
  textPaddingY_(2),
  gapX_(8)
{
	layout_=new QHBoxLayout(this);
	layout_->setSpacing(0);
	layout_->setContentsMargins(6,0,6,0);
	setLayout(layout_);

	setProperty("path","1");

	smp_=new QSignalMapper(this);

	connect(smp_,SIGNAL(mapped(int)),
		this,SIGNAL(nodeClicked(int)));

	connect(this,SIGNAL(nodeClicked(int)),
		this,SLOT(slotChangeNode(int)));

	QFont f(QApplication::font());
	QFontMetrics fm(f);

	height_=fm.height()+2*paddingY_+2*textPaddingY_;
}

void NodePathWidget::slotContextMenu(const QPoint& pos)
{
	/*static MvQContextItemSet cmItems("Breadcrumbs");

	QString path;
	QMenu *menu=0;

	QToolButton *tb=static_cast<QToolButton*>(QObject::sender());
	if(tb)
	{
	  	path=getPath(tb);
	}
	else
	{	menu=static_cast<QMenu*>(QObject::sender());
		if(menu)
		{
		  	QAction *ac=menu->actionAt(pos);
			path=getPath(ac);
		}
	}

	if((tb || menu) && !path.isEmpty())
	{
		QString selection;
		if(tb)
			selection=MvQContextMenu::instance()->exec(cmItems.icon(),tb->mapToGlobal(pos),tb);
		//else
		//  	selection=MvQContextMenu::instance()->exec(cmMenuItems_,menu->mapToGlobal(pos),menu);

		if(!selection.isEmpty())
		{
		  	emit commandRequested(selection,path);
		}
	}*/
}

void NodePathWidget::clearLayout()
{
  	QLayoutItem *item;
 	while( (item = layout_->takeAt(0)) != 0)
	{
     		QWidget *w=item->widget();
		if(w)
		{
		  	layout_->removeWidget(w);
		  	//delete w;
		}
		delete item;
 	}
}

void NodePathWidget::setReloadAction(QAction* ac)
{
  	actionReload_=ac;
}

void NodePathWidget::setPath(QString path)
{
	//Need to find out the node from the path
	//setPath();
}


void NodePathWidget::setPath(ViewNodeInfo_ptr info)
{
	//Clear the layout (widgets are not deleted in this step!)
	clearLayout();
  	path_.clear();
  	items_.clear();

  	ServerHandler *server=0;
  	bool sameServer=false;

  	if(info)
  	{
  		server=info->server();

  		sameServer=(info_)?(info_->server() != server):false;

  		if(!sameServer)
  		{
  			if(info_ && info_->server())
  				info_->server()->removeNodeObserver(this);

  			info->server()->addNodeObserver(this);
  		}
  	}

  	//Now it is safe to do
  	info_=info;

	//Get the node list
  	QList<Node*> lst;
  	if(info_->isNode())
	{
		Node *node=info->node();
		lst << node;
		Node *f=node;
		while(f->parent())
		{
			f=f->parent();
			lst.prepend(f);

			//qDebug() << "node" << f->absNodePath().c_str();
		}
	}

	lst.prepend(0);//indicates the server

	for(int i=0; i < lst.count(); i++)
	{
			QString stateCol;

			NodePathWidgetItem* item=0;

			//----------------------
			// Create item
			//----------------------

			//Server
			if(i==0)
			{
				item = new NodePathWidgetItem(QString::fromStdString(server->name()),
														QString::fromStdString("/"));

				item->colour_=QColor(255,255,255);
			}
			//Node
			else
			{
				Node *n=lst.at(i);
				item = new NodePathWidgetItem(QString::fromStdString(n->name()),
							QString::fromStdString(n->absNodePath()));

				item->colour_=VNState::toColour(n).name();
			}



			items_ << item;

	}

	updateGr();

}


void NodePathWidget::updateGr()
{
    QFont f(QApplication::font());
	QFontMetrics fm(f);

	int w;
	int h=height_-2*paddingY_;

	int xp=paddingX_;
	int yp=0;

	for(int i=0; i < items_.count(); i++)
	{
		NodePathWidgetItem *item=items_.at(i);
		w=fm.width(item->name_)+2*textPaddingX_;
		item->rect_=QRect(xp,yp,w,h);
		xp+=w+gapX_;
	}

	if(items_.count()>0)
	{
		xp-=gapX_;
	}

	xp+=paddingX_;

	resize(xp,height_);

	update();

}




void NodePathWidget::resizeEvent(QResizeEvent */*event*/)
{
		qDebug() << "resize";
}


/*
void NodePathWidget::setPath(ViewNodeInfo_ptr info)
{
	//Clear the layout (widgets are not deleted in this step!)
	clearLayout();
  	path_.clear();

  	ServerHandler *server=0;
  	bool sameServer=false;

  	if(info)
  	{
  		server=info->server();

  		sameServer=(info_)?(info_->server() != server):false;

  		if(!sameServer)
  		{
  			if(info_ && info_->server())
  				info_->server()->removeNodeObserver(this);

  			info->server()->addNodeObserver(this);
  		}
  	}

  	//Now it is safe to do
  	info_=info;

	//Get the node list
  	QList<Node*> lst;
  	if(info_->isNode())
	{
		Node *node=info->node();
		lst << node;
		Node *f=node;
		while(f->parent())
		{
			f=f->parent();
			lst.prepend(f);

			//qDebug() << "node" << f->absNodePath().c_str();
		}
	}

	lst.prepend(0);//indicates the server

	//If same server we try to keep some of the existing widgets/items
	int lastIndex=0;
	if(sameServer)
	{
		//-----------------------------------------------------------
		//Try to reuse the current items and delete the unneeded ones
		//-----------------------------------------------------------

		//qDebug() << "count" << lst.count() << items_.count();

		//Find out how many items can be reused.
		//firstIndex: the first original item that differs from the current one.
		for(int i=1; i <  lst.count() && i < items_.count(); i++)
		{
			//qDebug() << i << lst.at(i)->absNodePath().c_str() << items_[i]->path_;

			if(QString::fromStdString(lst.at(i)->absNodePath()) == items_[i]->path_)
			{
				lastIndex=i;
			}
			else
				break;
		}
	}


	//Delete unused items (if there are any)
	int num=items_.count();
	for(int i=lastIndex; i <  num; i++)
	{
		NodePathWidgetItem *item=items_.back();
	  	items_.removeLast();
		delete item;
	}

	for(int i=lastIndex; i < lst.count(); i++)
	{
		NodePathWidgetItem *item;
		QString stateCol;
		QToolButton *tb;

		//----------------------
		// Create item
		//----------------------

		//Server
		if(i==0)
	  	{
			item = new NodePathWidgetItem(QString::fromStdString(server->name()),
											QString::fromStdString("/"));
	  	}
		//Node
		else
		{
			Node *n=lst.at(i);
			item = new NodePathWidgetItem(QString::fromStdString(n->name()),
								QString::fromStdString(n->absNodePath()));

			stateCol=VState::toColour(f).name();
		}

		items_ << item;

		//---------------------------------------
		// Name
		//---------------------------------------

		tb=new QToolButton(this);
		item->nameTb_=tb;

		tb->setText(item->name_);
		tb->setObjectName("pathNameTb");

		//Server
		if(i==0)
		{
		  	tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		  	tb->setIconSize(QSize(16,16));
			tb->setIcon(QPixmap(":/viewer/server.svg"));
		}
		//Node
		else
		{
			tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
		}

		//qDebug() << tb->styleSheet();

		QString st=" QToolButton#pathNameTb { \
		     	border-radius: 0px;\
		     	padding: 0px; \
				color: black; \
			    background: " + stateCol + ";	\
		 	 }\
		QToolButton#pathNameTb:hover{\
			background:  qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #deebf6, stop: 1 #bed8ee);\
		        border: 1px solid rgb(160, 160, 160);\
		     	border-radius: 0px;\
		        padding: 0px; \
		 }";

		tb->setStyleSheet(st);

		//tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		tb->setAutoRaise(true);

		connect(tb,SIGNAL(clicked(bool)),
			    smp_,SLOT(map()));

		//Context menu
    	tb->setContextMenuPolicy(Qt::CustomContextMenu);

		connect(tb,SIGNAL(customContextMenuRequested(const QPoint &)),
                	this, SLOT(slotContextMenu(const QPoint &)));

		smp_->setMapping(tb,i);

		//connect(tb,SIGNAL(iconDropped(QDropEvent*)),
		//	this,SLOT(slotIconDropped(QDropEvent*)));

	  	//---------------------------------------
		//Menu with the folders in the given dir
		//---------------------------------------

		tb=new QToolButton(this);
		item->menuTb_=tb;

		//tb->setArrowType(Qt::RightArrow);
		tb->setAutoRaise(true);
		//tb->setIconSize(QSize(10,10));
		tb->setPopupMode(QToolButton::InstantPopup);
		tb->setObjectName("pathMenuTb");
		tb->setProperty("index",i);

		connect(tb,SIGNAL(clicked()),
				 this,SLOT(slotShowNodeChildrenMenu()));

		//Server
		if(i==0)
		{
			//item->menu_=createNodeChildrenMenu(i,f,tb);
		}
		else
		{
			Node *node=lst.at(i);
			item->menu_=createNodeChildrenMenu(i,node,tb);
		}
	}

	//Add items to layout
	for(int i=0; i <  items_.count(); i++)
	{
		layout_->addWidget(items_.at(i)->nameTb_);
		layout_->addWidget(items_.at(i)->menuTb_);
	}

	//Set the text in the last item bold
	for(int i=0; i <  items_.count(); i++)
	{
		QString st=items_.at(i)->nameTb_->styleSheet();
		if(i== items_.count()-1)
		 	st+="QToolButton {font-weight: bold;}";
		else
		  	st+="QToolButton {font-weight: normal;}";

		items_.at(i)->nameTb_->setStyleSheet(st);
	}

	layout_->addStretch(1);

	//Reload
	if(actionReload_)
	{
	  	if(!reloadTb_)
		{
		  	reloadTb_=new QToolButton(this);
			reloadTb_->setDefaultAction(actionReload_);
			reloadTb_->setAutoRaise(true);
			//reloadTb_->setIconSize(QSize(20,20));
			reloadTb_->setObjectName("pathIconTb");
		}
		layout_->addWidget(reloadTb_);
	}


}

*/

QMenu* NodePathWidget::createNodeChildrenMenu(int index,Node* node,QWidget *parent)
{
	if(index >=0 && index < items_.count())
	{
	  	QMenu *childrenMenu=new QMenu(parent);

	  	int num=ServerHandler::numOfImmediateChildren(node);
	  	for(int i=0; i < num; i++)
	  	{
	  		if(Node *n=ServerHandler::immediateChildAt(node,i))
	  		{
	  			QAction *ac=childrenMenu->addAction(QString::fromStdString(n->name()));
	  			ac->setData(index);
	  		}
	  	}

		//connect(childrenMenu,SIGNAL(triggered(QAction*)),
		//	this,SLOT(slotChangeNode(QAction*)));

		childrenMenu->setContextMenuPolicy(Qt::CustomContextMenu);

		connect(childrenMenu,SIGNAL(customContextMenuRequested(const QPoint &)),
                        this, SLOT(slotContextMenu(const QPoint &)));

		return childrenMenu;
	}

	return 0;
}

void NodePathWidget::slotShowNodeChildrenMenu()
{
	if(editable_)
		return;

	if(QToolButton *tb=static_cast<QToolButton*>(sender()))
	{
			int index=tb->property("index").toInt();
			if(index>= 0 && index < items_.count())
				items_.at(index)->menu_->popup(mapToGlobal(tb->pos()));
	}
}

//Slot for handling the node button click
void NodePathWidget::slotChangeNode(int index)
{
	if(editable_)
			return;

	if(index >=0 && index < items_.count()-1)
	{
	  	QString path=items_.at(index)->path_;

	  	//QString path;
		//for(int i=0; i <= index; i++)
		//	path+="/"+items_[i].name_;

		qDebug() << path;

		//setPath(path);

		//emit nodeSelected(path);

		setPath(path);
	}
}

//Slot for handling the node children menu selection
void NodePathWidget::slotChangeNode(QAction *ac)
{
	if(editable_)
			return;

	QString path=getPath(ac);

	if(!path.isEmpty())
	{
	  	//emit nodeSelected(path);
		setPath(path);
	}

	/*
	if (!ac) return;

	QVariant var=ac->data();
	if(var.isNull())  return;

	int index=var.toInt();

  	if(index >=0 && index < items_.count())
	{
		QString path=items_.at(index)->fullName_;

	  	//QString path;
		//for(int i=0; i < index; i++)
		//	path+="/"+items_[i].name_;

		path+="/" + ac->text();

		qDebug() << path;

		emit dirChanged(path);

		setPath(path);
	}*/
}

void NodePathWidget::slotEditable(bool e)
{
	if(e != editable_)
	{
		editable_=e;

		setPath(info_);
	}
}

QString NodePathWidget::getPath(QToolButton *tb)
{
	foreach(NodePathWidgetItem *item,items_)
	{
	  	if(item->nameTb_ == tb)
		{
	  		return item->path_;
		}
	}

	return QString();
}

QString NodePathWidget::getPath(QAction *ac)
{
	if (!ac) return QString();

	QVariant var=ac->data();
	if(var.isNull())  return QString();

	int index=var.toInt();

  	if(index >=0 && index < items_.count())
	{
		return items_.at(index)->path_ + "/" + ac->text();;
	}

	return QString();
}


ViewNodeInfo_ptr NodePathWidget::nodeAt(int idx)
{
	if(info_ && info_->server())
	{
		ServerHandler* server=info_->server();
		if(Node *node=info_->node())
		{
			for(int i=items_.count()-1; i > idx && i >=1; i--)
			{
				node=node->parent();
			}

			if(node)
			{
				ViewNodeInfo_ptr res(new ViewNodeInfo(node,server));
				qDebug() << "selected" << node->absNodePath().c_str();
				return res;
			}
		}

		ViewNodeInfo_ptr res(new ViewNodeInfo(server));
		qDebug() << "selected" << server->name().c_str();
		return res;
	}

	return ViewNodeInfo_ptr();
}


void NodePathWidget::mousePressEvent(QMouseEvent *event)
{
	if(editable_)
	{
		int idx=nodeIndex(event->pos());
		if(idx != -1)
		{
			emit nodeSelected(nodeAt(idx));
		}
	}
}

void NodePathWidget::mouseMoveEvent(QMouseEvent *event)
{
	/*if(event->buttons() != Qt::NoButton)
		return;

	int idx=index(event->pos());
	if(idx >= 0 && idx < cols_.count())
	{
		setToolTip(QString::fromStdString(names_[idx]));

	}*/
}

int NodePathWidget::nodeIndex(QPoint pos)
{
  	for(int i=0; i < items_.count(); i++)
  		if(items_.at(i)->rect_.contains(pos))
  			return i;

  	return -1;

}

void NodePathWidget::paintEvent(QPaintEvent *)
 {
     /*QStyleOption opt;
     opt.init(this);
     QPainter p(this);
     style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);*/

	QPainter p(this);

	QFont f(QApplication::font());
    QFontMetrics fm(f);

    int w;
    int h=fm.height();

    p.setPen(QPen(Qt::black));

    for(int i=0; i < items_.count(); i++)
    {
     	NodePathWidgetItem *item=items_.at(i);
     	//w=fm.width(item->name_)+2*paddingX_;

     	p.fillRect(item->rect_,item->colour_);
     	p.setPen(QPen(QColor(150,150,150)));
     	p.drawRect(item->rect_);

     	if(i>0)
     	{
     	     NodePathWidgetItem *prevItem=items_.at(i-1);
     	     p.drawLine(item->rect_.x(),item->rect_.center().y(),
     	     			 prevItem->rect_.right(),prevItem->rect_.center().y());
     	}

     	p.setPen(QPen(Qt::black));
     	p.drawText(item->rect_.adjusted(textPaddingX_,textPaddingY_,0,0),Qt::AlignCenter,item->name_);
     }
 }
