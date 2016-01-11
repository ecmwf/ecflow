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
#include <QHeaderView>
#include <QPalette>
#include <QScrollBar>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#endif

#include "ActionHandler.hpp"
#include "Animation.hpp"
#include "ExpandState.hpp"
#include "NodeFilterModel.hpp"
#include "PropertyMapper.hpp"
#include "TreeNodeViewDelegate.hpp"
#include "VNode.hpp"

TreeNodeView::TreeNodeView(NodeFilterModel* model,NodeFilterDef* filterDef,QWidget* parent) :
	QTreeView(parent),
	NodeViewBase(model,filterDef),
    needItemsLayout_(false),
	defaultIndentation_(indentation()),
	prop_(NULL)
{
	setProperty("style","nodeView");
	setProperty("view","tree");

	expandState_=new ExpandState();
	actionHandler_=new ActionHandler(this);

	//Set the model.
	setModel(model_);

	//Create delegate to the view
	delegate_=new TreeNodeViewDelegate(this);
	setItemDelegate(delegate_);

	connect(delegate_,SIGNAL(sizeHintChangedGlobal()),
			this,SLOT(slotSizeHintChangedGlobal()));

	//setRootIsDecorated(false);
	setAllColumnsShowFocus(true);
	//setUniformRowHeights(true);
	setMouseTracking(true);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	//!!!!We need to do it because:
	//The background colour between the view's left border and the nodes cannot be
	//controlled by delegates or stylesheets. It always takes the QPalette::Highlight
	//colour from the palette. Here we set this to transparent so that Qt could leave
	//this area empty and we will fill it appropriately in our delegate.
	QPalette pal=palette();
	pal.setColor(QPalette::Highlight,QColor(128,128,128,0));//Qt::transparent);
	setPalette(pal);

	//Hide header
	header()->hide();

	//Context menu
	setContextMenuPolicy(Qt::CustomContextMenu);

	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
		                this, SLOT(slotContextMenu(const QPoint &)));

	//Selection
	connect(this,SIGNAL(clicked(const QModelIndex&)),
			this,SLOT(slotSelectItem(const QModelIndex)));

	connect(this,SIGNAL(doubleClicked(const QModelIndex&)),
			this,SLOT(slotDoubleClickItem(const QModelIndex)));

	//expandAll();

	//Properties
	std::vector<std::string> propVec;
	propVec.push_back("view.tree.indentation");
	propVec.push_back("view.tree.background");
	prop_=new PropertyMapper(propVec,this);

	//Initialise indentation
	adjustIndentation(prop_->find("view.tree.indentation")->value().toInt());
	adjustBackground(prop_->find("view.tree.background")->value().value<QColor>());
}

TreeNodeView::~TreeNodeView()
{
	delete expandState_;
	delete actionHandler_;
	delete prop_;
}

void TreeNodeView::setModel(NodeFilterModel *model)
{
	model_= model;

	//Set the model.
	QTreeView::setModel(model_);
}


QWidget* TreeNodeView::realWidget()
{
	return this;
}

//Collects the selected list of indexes
QModelIndexList TreeNodeView::selectedList()
{
  	QModelIndexList lst;
  	Q_FOREACH(QModelIndex idx,selectedIndexes())
	  	if(idx.column() == 0)
		  	lst << idx;
	return lst;
}

void TreeNodeView::slotSelectItem(const QModelIndex&)
{
	QModelIndexList lst=selectedIndexes();
	if(lst.count() > 0)
	{
		VInfo_ptr info=model_->nodeInfo(lst.front());
		if(info)
		{
			Q_EMIT selectionChanged(info);
		}
	}
}
VInfo_ptr TreeNodeView::currentSelection()
{
	QModelIndexList lst=selectedIndexes();
	if(lst.count() > 0)
	{
		return model_->nodeInfo(lst.front());
	}
	return VInfo_ptr();
}

void TreeNodeView::currentSelection(VInfo_ptr info)
{
	QModelIndex idx=model_->infoToIndex(info);
	if(idx.isValid())
	{
		setCurrentIndex(idx);
		Q_EMIT selectionChanged(info);
	}
}

void TreeNodeView::slotSetCurrent(VInfo_ptr info)
{
	QModelIndex idx=model_->infoToIndex(info);
	if(idx.isValid())
	{
			setCurrentIndex(idx);
			Q_EMIT selectionChanged(info);
	}
}

void TreeNodeView::selectFirstServer()
{
	QModelIndex idx=model_->index(0,0);
	if(idx.isValid())
	{
		setCurrentIndex(idx);
		VInfo_ptr info=model_->nodeInfo(idx);
		Q_EMIT selectionChanged(info);
	}
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

  		std::vector<VInfo_ptr> nodeLst;
		for(int i=0; i < indexLst.count(); i++)
		{
			VInfo_ptr info=model_->nodeInfo(indexLst[i]);
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

void TreeNodeView::slotViewCommand(VInfo_ptr info,QString cmd)
{
	if(cmd == "expand")
	{
		QModelIndex idx=model_->infoToIndex(info);
		if(idx.isValid())
		{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
		expandAll(idx);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		QGuiApplication::restoreOverrideCursor();
#endif

		}
	}
	else if(cmd == "collapse")
	{
		QModelIndex idx=model_->infoToIndex(info);
		if(idx.isValid())
		{
				collapseAll(idx);

		}
	}


	/*if(cmd == "set_as_root")
	{
		qDebug() << "set as root";
		model_->setRootNode(nodeLst.at(0)->node());
		expandAll();
	}*/
}

void TreeNodeView::reload()
{
	//model_->reload();
	//expandAll();
}

void TreeNodeView::rerender()
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

void TreeNodeView::slotRerender()
{
	rerender();
}

void TreeNodeView::slotRepaint(Animation* an)
{
	if(!an)
		return;

	Q_FOREACH(QModelIndex idx,an->targets())
	{
		update(idx);
	}
}


void TreeNodeView::slotSizeHintChangedGlobal()
{
	needItemsLayout_=true;
}

void TreeNodeView::adjustIndentation(int offset)
{
	if(offset >=0)
	{
		setIndentation(defaultIndentation_+offset);
		delegate_->setIndentation(indentation());
	}
}

void TreeNodeView::adjustBackground(QColor col)
{
	if(col.isValid())
	{
		qDebug() << "bg" << col << col.name();
		QString sh="QTreeView { background : " + col.name() + ";}";
		setStyleSheet(sh);
	}
}

void TreeNodeView::notifyChange(VProperty* p)
{
	if(p->path() == "view.tree.indentation")
    {
		adjustIndentation(p->value().toInt());
    }
	else if(p->path() == "view.tree.background")
	{
		adjustBackground(p->value().value<QColor>());
	}
}

//====================================================
// Expand state management
//====================================================

void TreeNodeView::expandAll(const QModelIndex& idx)
{
	expand(idx);

	for(int i=0; i < model_->rowCount(idx); i++)
	{
		QModelIndex chIdx=model_->index(i, 0, idx);
		expandAll(chIdx);
	}
}

void TreeNodeView::collapseAll(const QModelIndex& idx)
{
	collapse(idx);

	for(int i=0; i < model_->rowCount(idx); i++)
	{
		QModelIndex chIdx=model_->index(i, 0, idx);
		collapseAll(chIdx);
	}
}

//Save the expand state for the given node (it can be a server as well)
void TreeNodeView::slotSaveExpand(const VNode* node)
{
	expandState_->clear();

	QModelIndex idx=model_->nodeToIndex(node);
	if(isExpanded(idx))
	{
		expandState_->setRoot(node->strName());
		saveExpand(expandState_->root(),idx);
	}
}

void TreeNodeView::saveExpand(ExpandNode *parentExpand,const QModelIndex& idx)
{
	for(int i=0; i < model_->rowCount(idx); i++)
	{
		QModelIndex chIdx=model_->index(i, 0, idx);

		if(!isExpanded(chIdx))
	        continue;
		else
		{
			ExpandNode* expand=parentExpand->add(chIdx.data(Qt::DisplayRole).toString().toStdString());
			saveExpand(expand,chIdx);
		}
	}
}

//Save the expand state for the given node (it can be a server as well)
void TreeNodeView::slotRestoreExpand(const VNode* node)
{
	if(!expandState_->root())
		return;

	if(node->strName() != expandState_->root()->name_)
	{
		expandState_->clear();
		return;
	}

	restoreExpand(expandState_->root(),node);

	expandState_->clear();
}

void TreeNodeView::restoreExpand(ExpandNode *expand,const VNode* node)
{
	//Lookup the mnode in the model
	QModelIndex nodeIdx=model_->nodeToIndex(node);
	if(nodeIdx != QModelIndex())
	{
		setExpanded(nodeIdx,true);
	}
	else
	{
		return;
	}

	for(int i=0; i < expand->children_.size(); i++)
	{
		ExpandNode *chExpand=expand->children_.at(i);
		std::string name=chExpand->name_;

		if(VNode *chNode=node->findChild(name))
		{
			QModelIndex chIdx=model_->nodeToIndex(chNode);
			if(chIdx != QModelIndex())
			{
				//setExpanded(chIdx,true);
				restoreExpand(chExpand,chNode);
			}
		}
	}
}
