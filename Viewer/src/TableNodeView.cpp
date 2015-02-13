//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TableNodeView.hpp"

#include <QDebug>
#include <QScrollBar>

//#include "Defs.hpp"
//#include "ClientInvoker.hpp"
//#include "Node.hpp"

#include "ActionHandler.hpp"
#include "NodeFilterModel.hpp"
#include "TableNodeModel.hpp"

TableNodeView::TableNodeView(QWidget* parent) : QTreeView(parent), NodeViewBase(0)
{
	setRootIsDecorated(false);

	setSortingEnabled(true);
	sortByColumn(0,Qt::AscendingOrder);

	setAllColumnsShowFocus(true);
	setUniformRowHeights(true);
	setMouseTracking(true);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	//!!!!We need to do it because:
	//The background colour between the views left border and the nodes cannot be
	//controlled by delegates or stylesheets. It always takes the QPalette::Highlight
	//colour from the palette. Here we set this to transparent so that Qt could leave
	//this are empty and we will fill it appropriately in our delegate.
	QPalette pal=palette();
	pal.setColor(QPalette::Highlight,Qt::transparent);
	setPalette(pal);

	//Context menu
	setContextMenuPolicy(Qt::CustomContextMenu);

	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
		                this, SLOT(slotContextMenu(const QPoint &)));

	//Selection
	connect(this,SIGNAL(clicked(const QModelIndex&)),
			this,SLOT(slotSelectItem(const QModelIndex)));

	connect(this,SIGNAL(doubleClicked(const QModelIndex&)),
			this,SLOT(slotDoubleClickItem(const QModelIndex)));

	actionHandler_=new ActionHandler(this);

	expandAll();
}

void TableNodeView::setModel(NodeFilterModel *model)
{
	model_= model;

	//Set the model.
	QTreeView::setModel(model_);
}

QWidget* TableNodeView::realWidget()
{
	return this;
}

//Collects the selected list of indexes
QModelIndexList TableNodeView::selectedList()
{
  	QModelIndexList lst;
  	Q_FOREACH(QModelIndex idx,selectedIndexes())
	  	if(idx.column() == 0)
		  	lst << idx;
	return lst;
}

void TableNodeView::slotSelectItem(const QModelIndex&)
{
	QModelIndexList lst=selectedIndexes();
	if(lst.count() > 0)
	{
		VInfo_ptr info=model_->nodeInfo(lst.front());
		if(!info->isEmpty())
		{
			Q_EMIT selectionChanged(info);
		}
	}
}

VInfo_ptr TableNodeView::currentSelection()
{
	QModelIndexList lst=selectedIndexes();
	if(lst.count() > 0)
	{
		return model_->nodeInfo(lst.front());
	}
	return VInfo_ptr();
}

void TableNodeView::slotDoubleClickItem(const QModelIndex&)
{
}

void TableNodeView::slotContextMenu(const QPoint &position)
{
	QModelIndexList lst=selectedList();
	//QModelIndex index=indexAt(position);
	QPoint scrollOffset(horizontalScrollBar()->value(),verticalScrollBar()->value());

	handleContextMenu(indexAt(position),lst,mapToGlobal(position),position+scrollOffset,this);
}


void TableNodeView::handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget)
{
	//Node actions
	  	if(indexClicked.isValid() && indexClicked.column() == 0)   //indexLst[0].isValid() && indexLst[0].column() == 0)
		{
		  	qDebug() << "context menu" << indexClicked;

	  		std::vector<VInfo_ptr> nodeLst;
			for(int i=0; i < indexLst.count(); i++)
			{
				VInfo_ptr info=model_->nodeInfo(indexLst[i]);
				if(!info->isEmpty())
					nodeLst.push_back(info);
			}

			actionHandler_->contextMenu(nodeLst,globalPos);
		}

		//Desktop actions
		else
		{
		}
}

void TableNodeView::slotViewCommand(std::vector<VInfo_ptr> nodeLst,QString cmd)
{

	if(nodeLst.size() == 0)
		return;

	if(cmd == "set_as_root")
	{
		qDebug() << "set as root";
		//model_->setRootNode(nodeLst.at(0)->node());
		//expandAll();
	}
}

