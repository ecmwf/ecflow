//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerCollector.hpp"

#include "UiLog.hpp"
#include "VItem.hpp"
#include "VItemPathParser.hpp"
#include "VNode.hpp"

#define _UI_TRIGGERCOLLECTOR_DEBUG

TriggerListCollector::~TriggerListCollector()
{
    clear();
}

bool TriggerListCollector::add(VItem* t, VItem* dep,Mode mode)
{
    TriggerListItem *item=new TriggerListItem(t,dep,mode) ;
    items_.push_back(item);
    return true;

#if 0
    if(dep)
    {
        UiLog().dbg() << " dep=" << dep->typeName() << " " +  dep->strName();
        UiLog().dbg() << "    =" << item->dep_->typeName() << " " <<  item->dep_->strName());
    }
#endif
}

void TriggerListCollector::setDependency(bool b)
{
    extended_=b;
    clear();
}

void TriggerListCollector::clear()
{
    /*for(size_t i=0; i < items_.size(); i++)
    {
        delete items_[i];
    }*/
    items_.clear();
}


bool TriggerChildCollector::add(VItem* t, VItem*,Mode)
{
    if(!t->isAncestor(node_))
    {
        // child is a kid of n whose trigger_panel is outside its subtree
        return collector_->add(t,child_,TriggerCollector::Child);
    }
    return false;
}

bool TriggerParentCollector::add(VItem* t, VItem*,Mode)
{
    return collector_->add(t,parent_,TriggerCollector::Parent);
}

bool TriggeredCollector::add(VItem* trigger, VItem*,Mode)
{
    if(VNode *n=trigger->isNode())
    {
        n->addTriggeredData(node_);
    }
    return false;

    //else if(trigger->isAttribute())
    //    trigger->parent()->addTriggeredData(node_,trigger);
}

