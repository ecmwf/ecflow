//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "GraphNodeView.hpp"

#include "ActionHandler.hpp"
#include "ExpandState.hpp"
#include "PropertyMapper.hpp"
#include "TreeNodeModel.hpp"
#include "UiLog.hpp"
#include "VFilter.hpp"

GraphNodeView::GraphNodeView(TreeNodeModel* model,NodeFilterDef* filterDef,QWidget* parent) :
    QAbstractItemView(parent),
    NodeViewBase(filterDef),
    model_(model),
    needItemsLayout_(false),
    //defaultIndentation_(indentation()),
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
}

GraphNodeView::~GraphNodeView()
{
    //delete expandState_;
    delete actionHandler_;
    //delete prop_;
}


QWidget* GraphNodeView::realWidget()
{
    return this;
}


VInfo_ptr GraphNodeView::currentSelection()
{
    QModelIndexList lst=selectedIndexes();
    if(lst.count() > 0)
    {
        return model_->nodeInfo(lst.front());
    }
    return VInfo_ptr();
}

void GraphNodeView::setCurrentSelection(VInfo_ptr info)
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
        //setCurrentIndex(idx);
    }
    setCurrentIsRunning_=false;
}

void GraphNodeView::reload()
{
    //model_->reload();
    //expandAll();
}

void GraphNodeView::rerender()
{
    if(needItemsLayout_)
    {
        //doItemsLayout();
        needItemsLayout_=false;
    }
    else
    {
        //viewport()->update();
    }
}
