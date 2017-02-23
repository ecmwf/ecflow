//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TreeNodeView.hpp"

#include <QtAlgorithms>
#include <QApplication>
#include <QDebug>
#include <QHeaderView>
#include <QPalette>
#include <QScrollBar>
#include <QTime>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#endif

#include "ActionHandler.hpp"
#include "Animation.hpp"
#include "AttributeEditor.hpp"
#include "ExpandState.hpp"
#include "TableNodeSortModel.hpp"
#include "PropertyMapper.hpp"
#include "TreeNodeModel.hpp"
#include "TreeNodeViewDelegate.hpp"
#include "UiLog.hpp"
#include "VNode.hpp"
#include "VModelData.hpp"
#include "VTree.hpp"

#define _UI_TREENODEVIEW_DEBUG

TreeNodeView::TreeNodeView(TreeNodeModel* model,NodeFilterDef* filterDef,QWidget* parent) :
	QTreeView(parent),
    NodeViewBase(filterDef),
    model_(model),
    needItemsLayout_(false),
	defaultIndentation_(indentation()),
    prop_(NULL),
    setCurrentIsRunning_(false),
    setCurrentFromExpand_(false)
{
    setObjectName("view");
    setProperty("style","nodeView");
	setProperty("view","tree");

    //expandState_=new ExpandState(this,model_);
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

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    header()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    header()->setStretchLastSection(false);
    header()->setMinimumSectionSize(4096);
#endif

	//!!!!We need to do it because:
	//The background colour between the view's left border and the nodes cannot be
	//controlled by delegates or stylesheets. It always takes the QPalette::Highlight
	//colour from the palette. Here we set this to transparent so that Qt could leave
	//this area empty and we will fill it appropriately in our delegate.
	QPalette pal=palette();
	pal.setColor(QPalette::Highlight,QColor(128,128,128,0));//Qt::transparent);
	setPalette(pal);

	//Hide header
    setHeaderHidden(true);
    //header()->hide();

	//Context menu
	setContextMenuPolicy(Qt::CustomContextMenu);

	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
		                this, SLOT(slotContextMenu(const QPoint &)));

	//Selection
	connect(this,SIGNAL(doubleClicked(const QModelIndex&)),
			this,SLOT(slotDoubleClickItem(const QModelIndex)));

	//expandAll();

	//Properties
	std::vector<std::string> propVec;
	propVec.push_back("view.tree.indentation");
    propVec.push_back("view.tree.background");
    propVec.push_back("view.tree.drawBranchLine");
    propVec.push_back("view.tree.serverToolTip");
    propVec.push_back("view.tree.nodeToolTip");
    propVec.push_back("view.tree.attributeToolTip");
	prop_=new PropertyMapper(propVec,this);

	//Initialise indentation
    Q_ASSERT(prop_->find("view.tree.indentation"));
    adjustIndentation(prop_->find("view.tree.indentation")->value().toInt());

    //Init stylesheet related properties
    Q_ASSERT(prop_->find("view.tree.background"));
    adjustBackground(prop_->find("view.tree.background")->value().value<QColor>(),false);
    Q_ASSERT(prop_->find("view.tree.drawBranchLine"));
    adjustBranchLines(prop_->find("view.tree.drawBranchLine")->value().toBool(),false);
    adjustStyleSheet();

    //Adjust tooltip
    Q_ASSERT(prop_->find("view.tree.serverToolTip"));
    adjustServerToolTip(prop_->find("view.tree.serverToolTip")->value().toBool());

    Q_ASSERT(prop_->find("view.tree.nodeToolTip"));
    adjustNodeToolTip(prop_->find("view.tree.nodeToolTip")->value().toBool());

    Q_ASSERT(prop_->find("view.tree.attributeToolTip"));
    adjustAttributeToolTip(prop_->find("view.tree.attributeToolTip")->value().toBool());
}

TreeNodeView::~TreeNodeView()
{
    qDeleteAll(expandStates_);
	delete actionHandler_;
	delete prop_;
}

#if 0
void TreeNodeView::setModel(NodeFilterModel *model)
{
	model_= model;

	//Set the model.
	QTreeView::setModel(model_);
}
#endif

QWidget* TreeNodeView::realWidget()
{
	return this;
}

void TreeNodeView::resizeEvent(QResizeEvent* e)
{
    QTreeView::resizeEvent(e);
    //resizeColumnToContents(0);
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

// reimplement virtual function from QTreeView - called when the selection is changed
void TreeNodeView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QModelIndexList lst=selectedIndexes();
    //When the selection was triggered from restoring (expanding) the nodes
    //we do not want to broadcast it
    if(lst.count() > 0 && !setCurrentFromExpand_)
    {            
        VInfo_ptr info=model_->nodeInfo(lst.front());
        if(info && !info->isEmpty())
        {
#ifdef _UI_TREENODEVIEW_DEBUG
            UiLog().dbg() << "TreeNodeView::selectionChanged --> emit=" << info->path();
#endif
            Q_EMIT selectionChanged(info);
        }
        lastSelection_=info;
    }

    QTreeView::selectionChanged(selected, deselected);

    //The model has to know about the selection in order to manage the
    //nodes that are forced to be shown
    model_->selectionChanged(lst);
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

void TreeNodeView::setCurrentSelection(VInfo_ptr info)
{
    //While the current is being selected we do not allow
    //another setCurrent call go through
    if(!info || setCurrentIsRunning_)
        return;

    setCurrentIsRunning_=true;
    QModelIndex idx=model_->infoToIndex(info);
	if(idx.isValid())
	{          
#ifdef _UI_TREENODEVIEW_DEBUG
        UiLog().dbg() << "TreeNodeView::setCurrentSelection --> " << info->path();
#endif
        setCurrentIndex(idx);
	}
    else
    {
        lastSelection_.reset();
    }
    setCurrentIsRunning_=false;
}

void TreeNodeView::setCurrentSelectionFromExpand(VInfo_ptr info)
{
    if(!info || setCurrentFromExpand_)
        return;

#ifdef _UI_TREENODEVIEW_DEBUG
        UiLog().dbg() << "TreeNodeView::setCurrentSelectionFromExpand --> " << info->path();
#endif

    setCurrentFromExpand_=true;
    setCurrentSelection(info);
    setCurrentFromExpand_=false;
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


void TreeNodeView::slotDoubleClickItem(const QModelIndex& idx)
{
    VInfo_ptr info=model_->nodeInfo(idx);
    if(info && info->isAttribute())
    {
        slotViewCommand(info,"edit");
    }
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
        //qDebug() << "context menu" << indexClicked;

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
#ifdef _UI_TREENODEVIEW_DEBUG
            QTime t;
            t.start();
#endif
            expandAll(idx);
#ifdef _UI_TREENODEVIEW_DEBUG
            UiLog().dbg() << "expandAll time=" << t.elapsed()/1000. << "s";
#endif
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

    else if(cmd ==  "edit")
    {
        if(info && info->isAttribute())
        {
            AttributeEditor::edit(info,this);
        }
    }

	/*if(cmd == "set_as_root")
	{
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

    Q_FOREACH(VNode* n,an->targets())
	{
        update(model_->nodeToIndex(n));
	}
}

void TreeNodeView::slotSizeHintChangedGlobal()
{
	needItemsLayout_=true;
}

void TreeNodeView::adjustStyleSheet()
{
    QString sh;
    if(styleSheet_.contains("bg"))
       sh+=styleSheet_["bg"];
    if(styleSheet_.contains("branch"))
       sh+=styleSheet_["branch"];

    qDebug() << "stylesheet" << sh;

    setStyleSheet(sh);
}

void TreeNodeView::adjustIndentation(int offset)
{
	if(offset >=0)
	{
		setIndentation(defaultIndentation_+offset);
		delegate_->setIndentation(indentation());
	}
}

void TreeNodeView::adjustBackground(QColor col,bool adjust)
{
	if(col.isValid())
	{       
        styleSheet_["bg"]="QTreeView { background : " + col.name() + ";}";

        if(adjust)
            adjustStyleSheet();
	}
}

void TreeNodeView::adjustBranchLines(bool st,bool adjust)
{
    if(styleSheet_.contains("branch"))
    {
        bool oriSt=styleSheet_["branch"].contains("url(:");
        if(oriSt == st)
            return;
    }

    QString vline((st)?"url(:/viewer/tree_vline.png) 0":"none");
    QString bmore((st)?"url(:/viewer/tree_branch_more.png) 0":"none");
    QString bend((st)?"url(:/viewer/tree_branch_end.png) 0":"none");

    styleSheet_["branch"]="QTreeView::branch:has-siblings:!adjoins-item { border-image: " + vline + ";}" \
     "QTreeView::branch:!has-children:has-siblings:adjoins-item {border-image: " +  bmore + ";}" \
     "QTreeView::branch:!has-children:!has-siblings:adjoins-item {border-image: " + bend + ";}";

    if(adjust)
        adjustStyleSheet();
}

void TreeNodeView::adjustServerToolTip(bool st)
{
    model_->setEnableServerToolTip(st);
}

void TreeNodeView::adjustNodeToolTip(bool st)
{
    model_->setEnableNodeToolTip(st);
}

void TreeNodeView::adjustAttributeToolTip(bool st)
{
    model_->setEnableAttributeToolTip(st);
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
    else if(p->path() == "view.tree.drawBranchLine")
    {
        adjustBranchLines(p->value().toBool());
    }
    else if(p->path() == "view.tree.serverToolTip")
    {
        adjustServerToolTip(p->value().toBool());
    }
    else if(p->path() == "view.tree.nodeToolTip")
    {
        adjustNodeToolTip(p->value().toBool());
    }
    else if(p->path() == "view.tree.attributeToolTip")
    {
        adjustAttributeToolTip(p->value().toBool());
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

void TreeNodeView::expandTo(const QModelIndex& idxTo)
{
    QModelIndex idx=model_->parent(idxTo);
    QModelIndexList lst;

    qDebug() << idxTo << idx;

    while(idx.isValid())
    {
        lst.push_front(idx);
        idx=idx.parent();
    }

    qDebug() << lst;

    Q_FOREACH(QModelIndex d,lst)
    {
        expand(d);
        qDebug() << "expand" << d << isExpanded(d);
    }
}

//Save all
void TreeNodeView::slotSaveExpand()
{
    for(int i=0; i < model_->rowCount(); i++)
    {
        QModelIndex serverIdx=model_->index(i, 0);
        VTreeServer* ts=model_->indexToServer(serverIdx);
        Q_ASSERT(ts);

        TreeViewExpandState* es=new TreeViewExpandState(this,model_);
        expandStates_ << es;
        es->save(ts->tree());

        //ExpandStateTree* es=expandState_->add();
        //es->save(ts->tree());
    }
}

void TreeNodeView::slotRestoreExpand()
{
    Q_FOREACH(TreeViewExpandState* es,expandStates_)
    {
        if(es->root())
        {
            VTreeServer* ts=model_->nameToServer(es->root()->name_);
            if(ts)
            {
                es->restore(ts->tree());
            }
        }
    }

    qDeleteAll(expandStates_);
    regainSelectionFromExpand();
}

//Save the expand state for the given node (it can be a server as well)
void TreeNodeView::slotSaveExpand(const VTreeNode* node)
{
    TreeViewExpandState* es=new TreeViewExpandState(this,model_);
    expandStates_ << es;
    es->save(node);

    //TreeViewExpandState* es=expandState_->add();
    //es->save(node);
}

//Restore the expand state for the given node (it can be a server as well)
void TreeNodeView::slotRestoreExpand(const VTreeNode* node)
{
    Q_FOREACH(TreeViewExpandState* es,expandStates_)
    {
        if(es->rootSameAs(node->vnode()->strName()))
        {
            es->restore(node);
            expandStates_.removeOne(es);
            delete es;
        }
    }

    regainSelectionFromExpand();
}

void TreeNodeView::regainSelectionFromExpand()
{
    VInfo_ptr s=currentSelection();
    if(!s)
    {
        if(lastSelection_)
        {
            lastSelection_->regainData();
            if(!lastSelection_->hasData())
            {
                lastSelection_.reset();
            }
            else
            {
                setCurrentSelectionFromExpand(lastSelection_);
            }
        }
    }
}
