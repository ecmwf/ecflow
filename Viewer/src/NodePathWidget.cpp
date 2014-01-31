/***************************** LICENSE START ***********************************

 Copyright 2013 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#include "NodePathWidget.hpp"

#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QHBoxLayout>
#include <QMenu>
#include <QSignalMapper>
#include <QStyleOption>
#include <QPainter>

#include "Node.hpp"

//QList<MvQContextItem*> NodePathWidget::cmTbItems_;
//QList<MvQContextItem*> NodePathWidget::cmMenuItems_;

//=============================================================
//
//  NodePathWidget
//
//=============================================================

NodePathWidget::NodePathWidget(QWidget *parent) :
  QWidget(parent),
  actionReload_(0),
  reloadTb_(0)
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

void NodePathWidget::setPath(Node *node)
{
	//Clear the layout (widgets are not deleted in this step!)
	clearLayout();
  	path_.clear();

	if(!node)
	  	return;

	path_=QString::fromStdString(node->absNodePath());

	QList<Node*> lst;
	lst << node;
	Node *f=node;
	while(f->parent())
	{
	  	f=f->parent();
		lst.prepend(f);
	}

	//-----------------------------------------------------------
	//Try to reuse the current items and delete the unneeded ones
	//-----------------------------------------------------------

	//Find out how many items can be reused.
	//firstIndex: the first original item that differs from the current one.
	int lastIndex=-1;
	for(int i=0; i <  lst.count() && i < items_.count(); i++)
	{
		if(QString::fromStdString(lst.at(i)->absNodePath()) == items_[i]->path_)
		{
			lastIndex=i;
		}
		else
		  	break;
	}

	//Delete unused items (if there are any)
	int num=items_.count();
	for(int i=lastIndex+1; i <  num; i++)
	{
		NodePathWidgetItem *item=items_.back();
	  	items_.removeLast();
		delete item;
	}

	for(int i=lastIndex+1; i < lst.count(); i++)
	{
	  	Node *f=lst.at(i);

		NodePathWidgetItem *item = new NodePathWidgetItem(QString::fromStdString(f->name()),
								QString::fromStdString(f->absNodePath()));
	  	items_ << item;

	  	QToolButton *tb;

		//---------------------------------------
		// Dir name
		//---------------------------------------

		tb=new QToolButton(this);
		item->nameTb_=tb;

		if(item->path_ == "/")
		{
		  	tb->setToolButtonStyle(Qt::ToolButtonIconOnly);
			tb->setIconSize(QSize(16,16));
			//tb->setIcon(MvQIconProvider::pixmap(f,16));
			tb->setObjectName("pathIconTb");
		}
		else
		{
		  	/*if(f->locked())
			{
		  		tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		  		tb->setIconSize(QSize(12,12));
				tb->setIcon(MvQIconProvider::lockPixmap(12));
			}
			else
			{
		  		tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
			}*/

			tb->setToolButtonStyle(Qt::ToolButtonTextOnly);

			tb->setText(item->name_);
			tb->setObjectName("pathNameTb");

			//if(f->isLink())
			//  	tb->setStyleSheet("QToolButton {font-style: italic;}");
		}

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

		tb->setArrowType(Qt::RightArrow);
		tb->setAutoRaise(true);
		tb->setIconSize(QSize(10,10));
		tb->setPopupMode(QToolButton::InstantPopup);
		tb->setObjectName("pathMenuTb");

		QMenu *mn=createNodeChildrenMenu(i,f,tb);
		tb->setMenu(mn);
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

	/*items_.at(items_.count()-1)->nameTb_->setStyleSheet("QToolButton {font-weight: bold;}");

	for(int i=0; i <  items_.count()-1; i++)
	{
		items_.at(i)->nameTb_->setStyleSheet("QToolButton {font-weight: normal;}");
	}*/

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

QMenu* NodePathWidget::createNodeChildrenMenu(int index,Node* node,QWidget *parent)
{
	if(index >=0 && index < items_.count())
	{
	  	QMenu *childrenMenu=new QMenu(parent);

		/*for(map<string,IconObjectH>::const_iterator it=folder->kids().begin(); it!= folder->kids().end(); it++)
		{
		  	if(it->second->visible() &&  it->second->iconClass().type() == "Folder")
			{
				QAction *ac=dirMenu->addAction(QString::fromStdString(it->first));
				ac->setData(index);
			}
		}*/

		connect(childrenMenu,SIGNAL(triggered(QAction*)),
			this,SLOT(slotChangeNode(QAction*)));

		childrenMenu->setContextMenuPolicy(Qt::CustomContextMenu);

		connect(childrenMenu,SIGNAL(customContextMenuRequested(const QPoint &)),
                        this, SLOT(slotContextMenu(const QPoint &)));

		return childrenMenu;
	}

	return 0;
}

//Slot for handling the node button click
void NodePathWidget::slotChangeNode(int index)
{
	if(index >=0 && index < items_.count()-1)
	{
	  	QString path=items_.at(index)->path_;

	  	//QString path;
		//for(int i=0; i <= index; i++)
		//	path+="/"+items_[i].name_;

		qDebug() << path;

		//setPath(path);

		emit nodeSelected(path);

		setPath(path);
	}
}

//Slot for handling the node children menu selection
void NodePathWidget::slotChangeNode(QAction *ac)
{
	QString path=getPath(ac);

	if(!path.isEmpty())
	{
	  	emit nodeSelected(path);
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

void NodePathWidget::paintEvent(QPaintEvent *)
 {
     QStyleOption opt;
     opt.init(this);
     QPainter p(this);
     style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
 }
