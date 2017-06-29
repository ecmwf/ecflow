//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeQueryResultView.hpp"

#include <QApplication>
#include <QDebug>
#include <QHeaderView>
#include <QPalette>
#include <QScrollBar>
#include <QSortFilterProxyModel>

#include "ActionHandler.hpp"
#include "NodeQueryResultModel.hpp"
#include "NodeQueryViewDelegate.hpp"
#include "UserMessage.hpp"
#include "VNode.hpp"

NodeQueryResultView::NodeQueryResultView(QWidget* parent) :
	QTreeView(parent),
	model_(NULL),
	sortModel_(NULL),
    needItemsLayout_(false)
{
	//setProperty("style","nodeView");
	setProperty("view","query");

    actionHandler_=new ActionHandler(this,this);

	sortModel_=new QSortFilterProxyModel(this);
	//sortModel_->setDynamicSortFilter(true);
	setModel(sortModel_);

	//Create delegate to the view
	delegate_=new NodeQueryViewDelegate(this);
	setItemDelegate(delegate_);

	connect(delegate_,SIGNAL(sizeHintChangedGlobal()),
			this,SLOT(slotSizeHintChangedGlobal()));

	//setRootIsDecorated(false);
	setAllColumnsShowFocus(true);
	setUniformRowHeights(true);
	setMouseTracking(true);
	setRootIsDecorated(false);
    setSortingEnabled(true);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	//!!!!We need to do it because:
	//The background colour between the view's left border and the nodes cannot be
	//controlled by delegates or stylesheets. It always takes the QPalette::Highlight
	//colour from the palette. Here we set this to transparent so that Qt could leave
	//this area empty and we will fill it appropriately in our delegate.
	QPalette pal=palette();
	pal.setColor(QPalette::Highlight,QColor(128,128,128,0));//Qt::transparent);
	setPalette(pal);

	//Context menu
	enableContextMenu(true);

	//Selection
	connect(this,SIGNAL(clicked(const QModelIndex&)),
            this,SLOT(slotSelectItem(const QModelIndex&)));

	connect(this,SIGNAL(doubleClicked(const QModelIndex&)),
            this,SLOT(slotDoubleClickItem(const QModelIndex&)));

}

NodeQueryResultView::~NodeQueryResultView()
{
	delete actionHandler_;
}


void NodeQueryResultView::enableContextMenu(bool enable)
{
	if (enable)
	{
		setContextMenuPolicy(Qt::CustomContextMenu);

		connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
		                	this, SLOT(slotContextMenu(const QPoint &)));
	}
	else
	{
		setContextMenuPolicy(Qt::NoContextMenu);

		disconnect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
		                	this, SLOT(slotContextMenu(const QPoint &)));
	}
}


void NodeQueryResultView::setSourceModel(NodeQueryResultModel *model)
{
	model_= model;
	sortModel_->setSourceModel(model_);
}



//Collects the selected list of indexes
QModelIndexList NodeQueryResultView::selectedList()
{
  	QModelIndexList lst;
  	Q_FOREACH(QModelIndex idx,selectedIndexes())
  	{
  		if(idx.column() == 0)
		  	lst << idx;
  	}
	return lst;
}

// this is called even if the user clicks outside of the node list to deselect all
void NodeQueryResultView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
	QTreeView::selectionChanged(selected, deselected);
	Q_EMIT selectionChanged();
}

void NodeQueryResultView::slotSelectItem(const QModelIndex&)
{
	QModelIndexList lst=selectedIndexes();
	if(lst.count() > 0)
	{
		VInfo_ptr info=model_->nodeInfo(sortModel_->mapToSource(lst.front()));
		if(info)
		{
			Q_EMIT selectionChanged(info);            
		}
	}
}

VInfo_ptr NodeQueryResultView::currentSelection()
{
	QModelIndexList lst=selectedIndexes();
	if(lst.count() > 0)
	{
		return model_->nodeInfo(sortModel_->mapToSource(lst.front()));
	}
	return VInfo_ptr();
}

void NodeQueryResultView::currentSelection(VInfo_ptr info)
{
	/*QModelIndex idx=model_->infoToIndex(info);
	if(idx.isValid())
	{
		setCurrentIndex(idx);
		Q_EMIT selectionChanged(info);
	}*/
}

void NodeQueryResultView::slotSetCurrent(VInfo_ptr info)
{
	/*QModelIndex idx=model_->infoToIndex(info);
	if(idx.isValid())
	{
		setCurrentIndex(idx);
		Q_EMIT selectionChanged(info);
	}*/
}

void NodeQueryResultView::selectFirstServer()
{
	QModelIndex idx=sortModel_->index(0,0);
	if(idx.isValid())
	{
		setCurrentIndex(idx);
		VInfo_ptr info=model_->nodeInfo(sortModel_->mapToSource(idx));
		Q_EMIT selectionChanged(info);
	}
}


void NodeQueryResultView::getListOfSelectedNodes(std::vector<VInfo_ptr> &nodeList)
{
	QModelIndexList indexList=selectedList();

	nodeList.clear();
	for(int i=0; i < indexList.count(); i++)
	{
		VInfo_ptr info=model_->nodeInfo(sortModel_->mapToSource(indexList[i]));
		if(info && !info->isEmpty())
			nodeList.push_back(info);
	}
}


void NodeQueryResultView::slotDoubleClickItem(const QModelIndex&)
{
}

void NodeQueryResultView::slotContextMenu(const QPoint &position)
{
	QModelIndexList lst=selectedList();
	//QModelIndex index=indexAt(position);
	QPoint scrollOffset(horizontalScrollBar()->value(),verticalScrollBar()->value());

	handleContextMenu(indexAt(position),lst,mapToGlobal(position),position+scrollOffset,this);
}


void NodeQueryResultView::handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget)
{
  	//Node actions
  	if(indexClicked.isValid())   //indexLst[0].isValid() && indexLst[0].column() == 0)
	{
  		std::vector<VInfo_ptr> nodeLst;
		for(int i=0; i < indexLst.count(); i++)
		{
			VInfo_ptr info=model_->nodeInfo(sortModel_->mapToSource(indexLst[i]));
			if(info && !info->isEmpty())
				nodeLst.push_back(info);
		}

		actionHandler_->contextMenu(nodeLst,globalPos);
	}

	//Desktop actions
	else
	{
	}
}

void NodeQueryResultView::slotViewCommand(std::vector<VInfo_ptr> nodeLst,QString cmd)
{

	if(nodeLst.size() == 0)
		return;

	/*if(cmd == "set_as_root")
	{	
		model_->setRootNode(nodeLst.at(0)->node());
		expandAll();
	}*/
}

void NodeQueryResultView::reload()
{
	//model_->reload();
	//expandAll();
}

void NodeQueryResultView::rerender()
{
	if(needItemsLayout_)
	{
		doItemsLayout();
		needItemsLayout_=false;
	}
	else
	{
		viewport()->update();
	}
}

void NodeQueryResultView::slotRerender()
{
	rerender();
}


void NodeQueryResultView::slotSizeHintChangedGlobal()
{
	needItemsLayout_=true;
}
