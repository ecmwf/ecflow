//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TreeNodeView.hpp"

#include <QApplication>
#include <QDebug>
#include <QScrollBar>

#include "ActionHandler.hpp"
#include "NodeFilterModel.hpp"
#include "TreeNodeModel.hpp"
#include "TreeNodeViewDelegate.hpp"

TreeNodeView::TreeNodeView(QString ,VConfig* config,QWidget* parent) : QTreeView(parent)
{
		model_=new TreeNodeModel(config,this);

		filterModel_=new NodeFilterModel(this);
		filterModel_->setSourceModel(model_);
		filterModel_->setDynamicSortFilter(true);

		setModel(filterModel_);

		TreeNodeViewDelegate *delegate=new TreeNodeViewDelegate(this);
		setItemDelegate(delegate);

		//setRootIsDecorated(false);
		setAllColumnsShowFocus(true);
	    setUniformRowHeights(true);
	    setMouseTracking(true);
		setSelectionMode(QAbstractItemView::ExtendedSelection);

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

QWidget* TreeNodeView::realWidget()
{
	return this;
}

//Collects the selected list of indexes
QModelIndexList TreeNodeView::selectedList()
{
  	QModelIndexList lst;
  	foreach(QModelIndex idx,selectedIndexes())
	  	if(idx.column() == 0)
		  	lst << idx;
	return lst;
}

void TreeNodeView::slotSelectItem(const QModelIndex&)
{
	QModelIndexList lst=selectedIndexes();
	if(lst.count() > 0)
	{
		ViewNodeInfo_ptr info=model_->nodeInfo(filterModel_->mapToSource(lst.front()));
		if(!info->isEmpty())
		{
			emit selectionChanged(info);
		}
	}
}
ViewNodeInfo_ptr TreeNodeView::currentSelection()
{
	QModelIndexList lst=selectedIndexes();
	if(lst.count() > 0)
	{
		return model_->nodeInfo(filterModel_->mapToSource(lst.front()));
	}
	return ViewNodeInfo_ptr();
}

void TreeNodeView::slotDoubleClickItem(const QModelIndex&)
{
}

void TreeNodeView::slotContextMenu(const QPoint &position)
{
	QModelIndexList lst=selectedList();
	//QModelIndex index=indexAt(position);
	QPoint scrollOffset(horizontalScrollBar()->value(),verticalScrollBar()->value());

	handleContextMenu(indexAt(position),lst,mapToGlobal(position),position+scrollOffset,this);
}


void TreeNodeView::handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget)
{
  	//Node actions
  	if(indexClicked.isValid() && indexClicked.column() == 0)   //indexLst[0].isValid() && indexLst[0].column() == 0)
	{
	  	qDebug() << "context menu" << indexClicked;

  		std::vector<ViewNodeInfo_ptr> nodeLst;
		for(int i=0; i < indexLst.count(); i++)
		{
			ViewNodeInfo_ptr info=model_->nodeInfo(filterModel_->mapToSource(indexLst[i]));
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

void TreeNodeView::slotViewCommand(std::vector<ViewNodeInfo_ptr> nodeLst,QString cmd)
{

	if(nodeLst.size() == 0)
		return;

	if(cmd == "set_as_root")
	{
		qDebug() << "set as root";
		model_->setRootNode(nodeLst.at(0)->node());
		expandAll();
	}
}


void TreeNodeView::reload()
{
	model_->reload();
	//expandAll();
}



