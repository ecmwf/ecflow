//============================================================================
// Copyright 2009-2019 ECMWF.
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
#include "AttributeEditor.hpp"
#include "NodeQueryResultModel.hpp"
#include "NodeQueryViewDelegate.hpp"
#include "TriggerTableModel.hpp"
#include "TriggerViewDelegate.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"
#include "VNode.hpp"

TriggerTableView::TriggerTableView(QWidget* parent) :
    QTreeView(parent)
{
    setProperty("view","trigger");

    actionHandler_=new ActionHandler(this,this);

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
    Q_ASSERT(model_==nullptr);
    model_=model;
    QTreeView::setModel(model);  
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

    if(lst.count() > 0)
    {
        TriggerTableItem* item=model_->indexToItem(lst.front());
        if(item && item->item())
        {
            Q_EMIT clicked(item);
        }
    }
}

void TriggerTableView::setCurrentItem(TriggerTableItem* item)
{
    QModelIndex idx=model_->itemToIndex(item);
    if(idx.isValid())
    {
        setCurrentIndex(idx);
    }
}

void TriggerTableView::slotDoubleClickItem(const QModelIndex& index)
{    
    VInfo_ptr info=model_->nodeInfo(index);
    if(info)
    {
        Q_EMIT linkSelected(info);
    }
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

void TriggerTableView::slotViewCommand(VInfo_ptr info,QString cmd)
{
    if(cmd == "lookup")
    {
        Q_EMIT linkSelected(info);
    }

    else if(cmd ==  "edit")
    {
        if(info && info->isAttribute())
        {
            AttributeEditor::edit(info,this);
        }
    }
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

