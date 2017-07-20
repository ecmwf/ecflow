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
#include <QHeaderView>
#include <QPalette>
#include <QScrollBar>
#include <QTime>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#endif

#include "AbstractNodeView.hpp"
#include "ActionHandler.hpp"
#include "Animation.hpp"
#include "AttributeEditor.hpp"
#include "ExpandState.hpp"
#include "TableNodeSortModel.hpp"
#include "PropertyMapper.hpp"
#include "StandardView.hpp"
#include "ServerHandler.hpp"
#include "TreeNodeModel.hpp"
#include "TreeNodeViewDelegate.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "VNode.hpp"
#include "VModelData.hpp"
#include "VTree.hpp"

#define _UI_TREENODEVIEW_DEBUG

TreeNodeView::TreeNodeView(AbstractNodeView* view,TreeNodeModel* model,NodeFilterDef* filterDef,QWidget* parent) :
    QObject(parent),
    view_(view),
    model_(model),
    NodeViewBase(filterDef),
    needItemsLayout_(false),
    prop_(NULL),
    setCurrentIsRunning_(false),
    setCurrentFromExpand_(false),
    inStartUp_(true)
{
    setObjectName("view");
    setProperty("style","nodeView");
    setProperty("view","tree");

    //Context menu
    connect(view_, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(slotContextMenu(const QPoint &)));

    //Selection
    connect(view_,SIGNAL(doubleClicked(const QModelIndex&)),
            this,SLOT(slotDoubleClickItem(const QModelIndex)));

    //Selection
    connect(view_,SIGNAL(selectionChangedInView(const QItemSelection&, const QItemSelection &)),
            this,SLOT(selectionChanged(const QItemSelection&, const QItemSelection &)));

    //expandState_=new ExpandState(this,model_);
    actionHandler_=new ActionHandler(this,view_);

    connect(view_->delegate(),SIGNAL(sizeHintChangedGlobal()),
            this,SLOT(slotSizeHintChangedGlobal()));

    //Properties
    std::vector<std::string> propVec;
    propVec.push_back("view.tree.indentation");
    propVec.push_back("view.tree.background");
    propVec.push_back("view.tree.drawBranchLine");
    propVec.push_back("view.tree.branchLineColour");
    propVec.push_back("view.tree.serverToolTip");
    propVec.push_back("view.tree.nodeToolTip");
    propVec.push_back("view.tree.attributeToolTip");

    prop_=new PropertyMapper(propVec,this);

    VProperty *prop=0;
    std::string propName;

    //Initialise indentation
    prop=prop_->find("view.tree.indentation",true);
    adjustIndentation(prop->value().toInt());

    //Init bg colour
    prop=prop_->find("view.tree.background",true);
    adjustBackground(prop->value().value<QColor>());

    //Init branch line status (on/off)
    prop=prop_->find("view.tree.drawBranchLine",true);
    adjustDrawBranchLine(prop->value().toBool());

    //Init branch line/connector colour
    prop=prop_->find("view.tree.branchLineColour",true);
    adjustBranchLineColour(prop->value().value<QColor>());

    //Adjust tooltip
    prop=prop_->find("view.tree.serverToolTip",true);
    adjustServerToolTip(prop->value().toBool());

    prop=prop_->find("view.tree.nodeToolTip",true);
    adjustNodeToolTip(prop->value().toBool());

    prop=prop_->find("view.tree.attributeToolTip",true);
    adjustAttributeToolTip(prop->value().toBool());

    inStartUp_=false;
}

TreeNodeView::~TreeNodeView()
{
    delete actionHandler_;
    delete prop_;
}

QWidget* TreeNodeView::realWidget()
{
    return view_;
}

QObject* TreeNodeView::realObject()
{
    return this;
}

//Collects the selected list of indexes
QModelIndexList TreeNodeView::selectedList()
{
    QModelIndexList lst;
    Q_FOREACH(QModelIndex idx,view_->selectedIndexes())
        if(idx.column() == 0)
            lst << idx;
    return lst;
}

// reimplement virtual function from CompactView - called when the selection is changed
void TreeNodeView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QModelIndexList lst=view_->selectedIndexes();
    //When the selection was triggered from restoring (expanding) the nodes
    //we do not want to broadcast it
    if(lst.count() > 0 && !setCurrentFromExpand_)
    {
        VInfo_ptr info=model_->nodeInfo(lst.front());
        if(info && !info->isEmpty())
        {
#ifdef _UI_COMPACTNODEVIEW_DEBUG
            UiLog().dbg() << "TreeNodeView::selectionChanged --> emit=" << info->path();
#endif
            Q_EMIT selectionChanged(info);
        }
        lastSelection_=info;
    }

    view_->selectionChanged(selected, deselected);

    //The model has to know about the selection in order to manage the
    //nodes that are forced to be shown
    model_->selectionChanged(lst);
}

VInfo_ptr TreeNodeView::currentSelection()
{
    QModelIndexList lst=view_->selectedIndexes();
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
#ifdef _UI_COMPACTNODEVIEW_DEBUG
        UiLog().dbg() << "TreeNodeView::setCurrentSelection --> " << info->path();
#endif
        view_->setCurrentIndex(idx);
    }
    setCurrentIsRunning_=false;
}


void TreeNodeView::setCurrentSelectionFromExpand(VInfo_ptr info)
{
    if(!info || setCurrentFromExpand_)
        return;

#ifdef _UI_COMPACTNODEVIEW_DEBUG
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
        view_->setCurrentIndex(idx);
        VInfo_ptr info=model_->nodeInfo(idx);
        Q_EMIT selectionChanged(info);
    }
}

void TreeNodeView::slotContextMenu(const QPoint &position)
{
    QModelIndexList lst=selectedList();
    //QModelIndex index=indexAt(position);
    QPoint scrollOffset(view_->horizontalScrollBar()->value(),view_->verticalScrollBar()->value());

    handleContextMenu(view_->indexAt(position),lst,view_->mapToGlobal(position),position+scrollOffset,view_);
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

void TreeNodeView::slotDoubleClickItem(const QModelIndex& idx)
{
    VInfo_ptr info=model_->nodeInfo(idx);
    if(info && info->isAttribute())
    {
        slotViewCommand(info,"edit");
    }
}

void TreeNodeView::slotViewCommand(VInfo_ptr info,QString cmd)
{
    //Expand all the children of the given node
    if(cmd == "expand")
    {
        QModelIndex idx=model_->infoToIndex(info);
        if(idx.isValid())
        {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
            QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
#ifdef _UI_COMPACTNODEVIEW_DEBUG
            QTime t;
            t.start();
#endif
            //apply expand in the view
            view_->expandAll(idx);
#ifdef _UI_COMPACTNODEVIEW_DEBUG
            UiLog().dbg() << "expandAll time=" << t.elapsed()/1000. << "s";
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
            QGuiApplication::restoreOverrideCursor();

            //save/update the expand state object
            saveExpandAll(idx);

#endif
        }
    }
    else if(cmd == "collapse")
    {
        QModelIndex idx=model_->infoToIndex(info);
        if(idx.isValid())
        {
            //apply expand in the view
            view_->collapseAll(idx);

            //save/update the expand state object
            saveCollapseAll(idx);
        }
    }

    else if(cmd ==  "edit")
    {
        if(info && info->isAttribute())
        {
            AttributeEditor::edit(info,view_);
        }
    }

    //Only filter the given suite (of the node stored in info).
    //We achive this by setting the suitefilter to filter only this
    //suite.
    else if(cmd ==  "filterOne")
    {
        if(info)
        {
            if(ServerHandler* server=info->server())
            {
                VNode* n=info->node();
                if(n && info->isNode())
                    server->setSuiteFilterWithOne(n);
            }
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
        view_->doItemsLayout();
        needItemsLayout_=false;
    }
    else
    {
        view_->viewport()->update();
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
        view_->update(model_->nodeToIndex(n));
    }
}

void TreeNodeView::slotSizeHintChangedGlobal()
{
    needItemsLayout_=true;
}

//====================================================
// Expand state management
//====================================================

void TreeNodeView::expandTo(const QModelIndex& idxTo)
{
    QModelIndex idx=model_->parent(idxTo);
    QModelIndexList lst;

    //qDebug() << idxTo << idx;

    while(idx.isValid())
    {
        lst.push_front(idx);
        idx=idx.parent();
    }

    //qDebug() << lst;

    Q_FOREACH(QModelIndex d,lst)
    {
        view_->expand(d);
        //qDebug() << "expand" << d << isExpanded(d);
    }
}

//Save all
void TreeNodeView::slotSaveExpand()
{
    //For each server we save the expand state
    for(int i=0; i < model_->rowCount(); i++)
    {
        QModelIndex serverIdx=model_->index(i, 0);
        VTreeServer* ts=model_->indexToServer(serverIdx);
        Q_ASSERT(ts);

        //The expand state is stored on the VTreeServer and must survive updates and refreshes!
        ExpandState* es=ts->expandState();
        if(!es)
        {
            es=new ExpandState(view_,model_);
            ts->setExpandState(es); //the treeserver takes ownership of the expandstate
        }

        //Save the current state
        Q_ASSERT(ts->tree());
        VNode* vnode=ts->tree()->vnode();
        Q_ASSERT(vnode);
        es->save(vnode);
    }
}

void TreeNodeView::slotRestoreExpand()
{    
    //For each server we restore the expand state
    for(int i=0; i < model_->rowCount(); i++)
    {
        QModelIndex serverIdx=model_->index(i, 0);
        VTreeServer* ts=model_->indexToServer(serverIdx);
        Q_ASSERT(ts);

        //The expand state is stored on the VTreeServer
        ExpandState* es=ts->expandState();
        if(es)
        {
            Q_ASSERT(ts->tree());
            VNode* vnode=ts->tree()->vnode();
            Q_ASSERT(vnode);

            bool expanded=view_->isExpanded(serverIdx);
            view_->collapse(serverIdx);
            es->collectExpanded(vnode,view_->expandedIndexes);
            if(expanded)
                view_->expand(serverIdx);
        }
    }
    regainSelectionFromExpand();
}

//Save the expand state for the given node (it can be a server as well)
void TreeNodeView::slotSaveExpand(const VTreeNode* node)
{
    Q_ASSERT(node);
    ExpandState* es=0;
    VTreeServer* ts=node->server();
    Q_ASSERT(ts);

    //for servers
    if(node->isRoot())
    {
         es=ts->expandState();
         if(!es)
         {
             es=new ExpandState(view_,model_);
             ts->setExpandState(es); //the treeserver takes ownership of the expandstate
         }
    }
    //for other nodes - it is just a tmp expand state
    else
    {
        es=new ExpandState(view_,model_);
        ts->setTmpExpandState(es);
    }

    Q_ASSERT(es);

    //Save the current state
    es->save(node->vnode());

    //es->print();
}

//Restore the expand state for the given node (it can be a server as well)
void TreeNodeView::slotRestoreExpand(const VTreeNode* node)
{    
    Q_ASSERT(node);
    ExpandState* es=0;
    VTreeServer* ts=node->server();
    Q_ASSERT(ts);

    //For servers the expand state persists on the vtreenode, For other
    //nodes we just store a tmop expand state on the vtreenode: it only
    //exists for one save-restore cycle.
    es=(node->isRoot())?ts->expandState():ts->tmpExpandState();

    if(es)
    {
        QModelIndex idx=model_->nodeToIndex(node);
        if(idx.isValid())
        {
            bool expanded=view_->isExpanded(idx);
            view_->collapse(idx);
            es->collectExpanded(node->vnode(),view_->expandedIndexes);
            if(expanded)
                view_->expand(idx);
        }

        //we delete the tmp expand state
        if(es == ts->tmpExpandState())
        {
            ts->clearTmpExpandState();
        }
        //es->print();
    }

    regainSelectionFromExpand();
}

void TreeNodeView::saveExpandAll(const QModelIndex& idx)
{
    if(!idx.isValid())
        return;

    VTreeNode* tnode=model_->indexToServerOrNode(idx);
    Q_ASSERT(tnode);
    VTreeServer* ts=tnode->server();
    Q_ASSERT(ts);
    Q_ASSERT(ts->tree());
    VNode* vnode=ts->tree()->vnode();
    Q_ASSERT(vnode);

    //The expand state is stored on the VTreeServer
    ExpandState* es=ts->expandState();
    if(!es)
    {
        es=new ExpandState(view_,model_);
        ts->setExpandState(es); //the treeserver takes ownership of the expandstate
    }
    if(es->isEmpty())
    {      
        es->save(vnode);
    }
    es->saveExpandAll(vnode);
}

void TreeNodeView::saveCollapseAll(const QModelIndex& idx)
{
    if(!idx.isValid())
        return;

    VTreeNode* tnode=model_->indexToServerOrNode(idx);
    Q_ASSERT(tnode);
    VTreeServer* ts=tnode->server();
    Q_ASSERT(ts);
    Q_ASSERT(ts->tree());
    VNode* vnode=ts->tree()->vnode();
    Q_ASSERT(vnode);

    //The expand state is stored on the VTreeServer
    ExpandState* es=ts->expandState();
    if(!es)
    {
        es=new ExpandState(view_,model_);
        ts->setExpandState(es); //the treeserver takes ownership of the expandstate
    }
    if(es->isEmpty())
    {     
        es->save(vnode);
    }
    es->saveCollapseAll(vnode);
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

//==============================================
// Property handling
//==============================================

void TreeNodeView::adjustIndentation(int indent)
{
    if(indent >=0)
    {
        view_->setIndentation(indent);
        needItemsLayout_=true;
    }
}

void TreeNodeView::adjustBackground(QColor col)
{
    if(col.isValid())
    {
        QPalette p=view_->viewport()->palette();
        p.setColor(QPalette::Window,col);
        view_->viewport()->setPalette(p);

        //When we set the palette on startup something resets the palette
        //before the first paint event happens. So we set the expected bg colour
        //so that the view should know what bg colour it should use.
        if(inStartUp_)
            view_->setExpectedBg(col);
    }
}

void TreeNodeView::adjustDrawBranchLine(bool b)
{
    view_->setDrawConnector(b);
}

void TreeNodeView::adjustBranchLineColour(QColor col)
{
    view_->setConnectorColour(col);
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
    if(p->path() == "view.tree.background")
    {
        adjustBackground(p->value().value<QColor>());
    }
    else if(p->path() == "view.tree.indentation")
    {
        adjustIndentation(p->value().toInt());
    }
    else if(p->path() == "view.tree.drawBranchLine")
    {
        adjustDrawBranchLine(p->value().toBool());
    }
    else if(p->path() == "view.tree.branchLineColour")
    {
        adjustBranchLineColour(p->value().value<QColor>());
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
