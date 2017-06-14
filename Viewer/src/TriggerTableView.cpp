//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TriggerTableView.hpp"

#include <QApplication>
#include <QDebug>
#include <QHeaderView>
#include <QPalette>
#include <QScrollBar>


#include "ActionHandler.hpp"
#include "NodeQueryResultModel.hpp"
#include "NodeQueryViewDelegate.hpp"
#include "TriggerTableModel.hpp"
#include "TriggerViewDelegate.hpp"
#include "UserMessage.hpp"
#include "VNode.hpp"

TriggerTableView::TriggerTableView(QWidget* parent) :
    QTreeView(parent),
    model_(NULL),
    needItemsLayout_(false)
{
    setProperty("view","query");

    actionHandler_=new ActionHandler(this);

    //Set the model
    setModel(model_);

    //Create delegate to the view
    delegate_=new TriggerViewDelegate(this);
    setItemDelegate(delegate_);

    connect(delegate_,SIGNAL(sizeHintChangedGlobal()),
            this,SLOT(slotSizeHintChangedGlobal()));

    setHeaderHidden(true);
    setRootIsDecorated(false);
    setAllColumnsShowFocus(true);
    setUniformRowHeights(true);
    setMouseTracking(true);
    setSortingEnabled(false);
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

TriggerTableView::~TriggerTableView()
{
    delete actionHandler_;
}

//We should only call it once!!!
void TriggerTableView::setModel(TriggerTableModel* model)
{
    Q_ASSERT(model_==0);
    model_=model;
    QTreeView::setModel(model);

    //Set the width of the first column
    QFont f;
    QFontMetrics fm(f);
    setColumnWidth(0,fm.width("TTT"));
}

void TriggerTableView::enableContextMenu(bool enable)
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


//Collects the selected list of indexes
QModelIndexList TriggerTableView::selectedList()
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
void TriggerTableView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QModelIndexList lst=selectedIndexes();
    if(lst.count() > 0)
    {
        TriggerTableItem* item=model_->indexToItem(lst.front());
        if(item && item->item())
        {
            Q_EMIT selectionChanged(item);
        }
    }

    QTreeView::selectionChanged(selected, deselected);

    //Q_EMIT selectionChanged();
}

void TriggerTableView::slotSelectItem(const QModelIndex&)
{
    QModelIndexList lst=selectedIndexes();
#if 0
    if(lst.count() > 0)
    {
        VInfo_ptr info=model_->nodeInfo(sortModel_->mapToSource(lst.front()));
        if(info)
        {
            Q_EMIT selectionChanged(info);
        }
    }
#endif
}

VInfo_ptr TriggerTableView::currentSelection()
{
    QModelIndexList lst=selectedIndexes();
    if(lst.count() > 0)
    {
#if 0
        return model_->nodeInfo(sortModel_->mapToSource(lst.front()));
#endif
    }
    return VInfo_ptr();
}

void TriggerTableView::currentSelection(VInfo_ptr info)
{
    /*QModelIndex idx=model_->infoToIndex(info);
    if(idx.isValid())
    {
        setCurrentIndex(idx);
        Q_EMIT selectionChanged(info);
    }*/
}

void TriggerTableView::slotSetCurrent(VInfo_ptr info)
{
    /*QModelIndex idx=model_->infoToIndex(info);
    if(idx.isValid())
    {
        setCurrentIndex(idx);
        Q_EMIT selectionChanged(info);
    }*/
}

#if 0
void TriggerTableView::getListOfSelectedNodes(std::vector<VInfo_ptr> &nodeList)
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
#endif

void TriggerTableView::slotDoubleClickItem(const QModelIndex&)
{
}

void TriggerTableView::slotContextMenu(const QPoint &position)
{
    QModelIndexList lst=selectedList();
    //QModelIndex index=indexAt(position);
    QPoint scrollOffset(horizontalScrollBar()->value(),verticalScrollBar()->value());

    handleContextMenu(indexAt(position),lst,mapToGlobal(position),position+scrollOffset,this);
}


void TriggerTableView::handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget)
{
    //Node actions
    if(indexClicked.isValid())   //indexLst[0].isValid() && indexLst[0].column() == 0)
    {
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

void TriggerTableView::slotViewCommand(std::vector<VInfo_ptr> nodeLst,QString cmd)
{

    if(nodeLst.size() == 0)
        return;

    /*if(cmd == "set_as_root")
    {
        model_->setRootNode(nodeLst.at(0)->node());
        expandAll();
    }*/
}

void TriggerTableView::reload()
{
    //model_->reload();
    //expandAll();
}

void TriggerTableView::rerender()
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

void TriggerTableView::slotRerender()
{
    rerender();
}


void TriggerTableView::slotSizeHintChangedGlobal()
{
    needItemsLayout_=true;
}

