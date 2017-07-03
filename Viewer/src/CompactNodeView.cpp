//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "CompactNodeView.hpp"

#include <QDebug>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTime>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#endif

#include "ActionHandler.hpp"
#include "Animation.hpp"
#include "AttributeEditor.hpp"
#include "CompactNodeViewDelegate.hpp"
#include "ExpandState.hpp"
#include "PropertyMapper.hpp"
#include "TreeNodeModel.hpp"
#include "StandardNodeViewDelegate.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "VFilter.hpp"
#include "VModelData.hpp"
#include "VTree.hpp"

#define _UI_COMPACTNODEVIEW_DEBUG

CompactNodeView::CompactNodeView(TreeNodeModel* model,NodeFilterDef* filterDef,QWidget* parent) :
    view_(new CompactView(model,parent)),
    NodeViewBase(filterDef),    
    needItemsLayout_(false),
    //defaultIndentation_(indentation()),
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

    //expandState_=new ExpandState(this,model_);
    actionHandler_=new ActionHandler(this);

    connect(view_->delegate(),SIGNAL(sizeHintChangedGlobal()),
            this,SLOT(slotSizeHintChangedGlobal()));

    //Properties
    std::vector<std::string> propVec;
    propVec.push_back("view.tree.background");
    propVec.push_back("view.tree.branchLineColour");
    propVec.push_back("view.tree.serverToolTip");
    propVec.push_back("view.tree.nodeToolTip");
    propVec.push_back("view.tree.attributeToolTip");
    prop_=new PropertyMapper(propVec,this);

    VProperty *prop=0;
    std::string propName;

    //Init stylesheet related properties
    propName="view.tree.background";
    prop=prop_->find(propName);
    UI_ASSERT(prop,"Could not find property=" + propName);
    adjustBackground(prop->value().value<QColor>());

    //Init stylesheet related properties
    propName="view.tree.branchLineColour";
    prop=prop_->find(propName);
    UI_ASSERT(prop,"Could not find property=" + propName);
    adjustBranchLineColour(prop->value().value<QColor>());

    //Adjust tooltip
    propName="view.tree.serverToolTip";
    prop=prop_->find(propName);
    UI_ASSERT(prop,"Could not find property=" + propName);
    adjustServerToolTip(prop->value().toBool());

    propName="view.tree.nodeToolTip";
    prop=prop_->find(propName);
    UI_ASSERT(prop,"Could not find property=" + propName);
    adjustNodeToolTip(prop->value().toBool());

    propName="view.tree.attributeToolTip";
    prop=prop_->find(propName);
    UI_ASSERT(prop,"Could not find property=" + propName);
    adjustAttributeToolTip(prop->value().toBool());

    inStartUp_=false;
}

CompactNodeView::~CompactNodeView()
{
    //qDeleteAll(expandStates_);
    delete actionHandler_;
    delete prop_;
}

QWidget* CompactNodeView::realWidget()
{
    return view_;
}

//Collects the selected list of indexes
QModelIndexList CompactNodeView::selectedList()
{
    QModelIndexList lst;
    Q_FOREACH(QModelIndex idx,view_->selectedIndexes())
        if(idx.column() == 0)
            lst << idx;
    return lst;
}

// reimplement virtual function from CompactView - called when the selection is changed
void CompactNodeView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
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
            UiLog().dbg() << "CompactNodeView::selectionChanged --> emit=" << info->path();
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

VInfo_ptr CompactNodeView::currentSelection()
{
    QModelIndexList lst=view_->selectedIndexes();
    if(lst.count() > 0)
    {
        return model_->nodeInfo(lst.front());
    }
    return VInfo_ptr();
}

void CompactNodeView::setCurrentSelection(VInfo_ptr info)
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
        UiLog().dbg() << "CompactNodeView::setCurrentSelection --> " << info->path();
#endif
        view_->setCurrentIndex(idx);
    }
    setCurrentIsRunning_=false;
}


void CompactNodeView::setCurrentSelectionFromExpand(VInfo_ptr info)
{
    if(!info || setCurrentFromExpand_)
        return;

#ifdef _UI_COMPACTNODEVIEW_DEBUG
        UiLog().dbg() << "CompactNodeView::setCurrentSelectionFromExpand --> " << info->path();
#endif

    setCurrentFromExpand_=true;
    setCurrentSelection(info);
    setCurrentFromExpand_=false;
}

void CompactNodeView::selectFirstServer()
{
    QModelIndex idx=model_->index(0,0);
    if(idx.isValid())
    {
        view_->setCurrentIndex(idx);
        VInfo_ptr info=model_->nodeInfo(idx);
        Q_EMIT selectionChanged(info);
    }
}

void CompactNodeView::slotContextMenu(const QPoint &position)
{
    QModelIndexList lst=view_->selectedList();
    //QModelIndex index=indexAt(position);
    QPoint scrollOffset(view_->horizontalScrollBar()->value(),view_->verticalScrollBar()->value());

    handleContextMenu(indexAt(position),lst,mapToGlobal(position),position+scrollOffset,this);
}


void CompactNodeView::handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget)
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

void CompactNodeView::slotDoubleClickItem(const QModelIndex& idx)
{
    VInfo_ptr info=model_->nodeInfo(idx);
    if(info && info->isAttribute())
    {
        slotViewCommand(info,"edit");
    }
}

void CompactNodeView::slotViewCommand(VInfo_ptr info,QString cmd)
{
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
            expandAll(idx);
#ifdef _UI_COMPACTNODEVIEW_DEBUG
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
            view_->collapseAll(idx);
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

void CompactNodeView::reload()
{
    //model_->reload();
    //expandAll();
}

void CompactNodeView::rerender()
{
    if(needItemsLayout_)
    {
        doItemsLayout();
        needItemsLayout_=false;
    }
    else
    {
        view_->viewport()->update();
    }
}

void CompactNodeView::slotRerender()
{
    rerender();
}

void CompactNodeView::slotRepaint(Animation* an)
{
    if(!an)
        return;

    Q_FOREACH(VNode* n,an->targets())
    {
        view_->update(model_->nodeToIndex(n));
    }
}

void CompactNodeView::slotSizeHintChangedGlobal()
{
    needItemsLayout_=true;
}


//====================================================
// Expand state management
//====================================================

bool CompactNodeView::isNodeExpanded(const QModelIndex& idx) const
{
    return view_->isExpanded(idx);
}

#if 0
void CompactNodeView::expandAll(const QModelIndex& idx)
{
    expand(idx);

    for(int i=0; i < model_->rowCount(idx); i++)
    {
        QModelIndex chIdx=model_->index(i, 0, idx);
        expandAll(chIdx);
    }
}
#endif

#if 0
void CompactNodeView::collapseAll(const QModelIndex& idx)
{
    collapse(idx);

    for(int i=0; i < model_->rowCount(idx); i++)
    {
        QModelIndex chIdx=model_->index(i, 0, idx);
        collapseAll(chIdx);
    }
}
#endif

void CompactNodeView::expandTo(const QModelIndex& idxTo)
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
void CompactNodeView::slotSaveExpand()
{
    for(int i=0; i < model_->rowCount(); i++)
    {
        QModelIndex serverIdx=model_->index(i, 0);
        VTreeServer* ts=model_->indexToServer(serverIdx);
        Q_ASSERT(ts);

        ExpandState* es=ts->expandState();
        if(!es)
        {
            es=new ExpandState(this,model_);
            ts->setExpandState(es); //thre treeserver takes ownership of the expandstate
        }

        es->save(ts->tree());
    }

#if 0
    for(int i=0; i < model_->rowCount(); i++)
    {
        QModelIndex serverIdx=model_->index(i, 0);
        VTreeServer* ts=model_->indexToServer(serverIdx);
        Q_ASSERT(ts);

        CompactViewExpandState* es=expandStates_->find(ts);
        if(!es)
        {
            es=expandStates_->add(this,model_);
        }
        //CompactViewExpandState* es=new CompactViewExpandState(this,model_);
        //expandStates_ << es;
        es->save(ts->tree());
    }

    expandStates_->removeUnused();
#endif
}

void CompactNodeView::slotRestoreExpand()
{
#if 0
    Q_FOREACH(CompactViewExpandState* es,expandStates_)
    {
        if(es->root())
        {
            VTreeServer* ts=model_->nameToServer(es->root()->name_);
            if(ts)
            {
                QModelIndex idx=model_->nodeToIndex(ts->tree());
                if(idx.isValid())
                {
                    collapse(idx);
                    es->collectExpanded(ts->tree(),expandedIndexes);
                    expand(idx);
                }
            }
        }
    }

    qDeleteAll(expandStates_);
    expandStates_.clear();
    regainSelectionFromExpand();
#endif
}

//Save the expand state for the given node (it can be a server as well)
void CompactNodeView::slotSaveExpand(const VTreeNode* node)
{
#if 0
    CompactViewExpandState* es=new CompactViewExpandState(this,model_);
    expandStates_ << es;
    es->save(node);
#endif
}

//Restore the expand state for the given node (it can be a server as well)
void CompactNodeView::slotRestoreExpand(const VTreeNode* node)
{
#if 0
    for(int i=0; i < expandStates_.count(); i++)
    {
        CompactViewExpandState* es=expandStates_[i];
        {
            if(es->rootSameAs(node->vnode()->strName()))
            {
                //es->restore(node);
                QModelIndex idx=model_->nodeToIndex(node);
                if(idx.isValid())
                {
                    collapse(idx);
                    es->collectExpanded(node,expandedIndexes);
                    expand(idx);
                }
                expandStates_.remove(i);
                delete es;
                break;
            }
        }
    }

    regainSelectionFromExpand();
#endif
}

void CompactNodeView::regainSelectionFromExpand()
{
    VInfo_ptr s=view_->currentSelection();
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

void CompactNodeView::adjustBackground(QColor col)
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
            setExpectedBg(col);
    }
}

void CompactNodeView::adjustBranchLineColour(QColor col)
{
    setConnectorColour(col);
}

void CompactNodeView::adjustServerToolTip(bool st)
{
    model_->setEnableServerToolTip(st);
}

void CompactNodeView::adjustNodeToolTip(bool st)
{
    model_->setEnableNodeToolTip(st);
}

void CompactNodeView::adjustAttributeToolTip(bool st)
{
    model_->setEnableAttributeToolTip(st);
}

void CompactNodeView::notifyChange(VProperty* p)
{   
    if(p->path() == "view.tree.background")
    {
        adjustBackground(p->value().value<QColor>());
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
