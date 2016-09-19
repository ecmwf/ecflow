//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerCollector.hpp"

#include "UserMessage.hpp"
#include "VItem.hpp"
#include "VItemPathParser.hpp"
#include "VNode.hpp"

#define _UI_TRIGGERCOLLECTOR_DEBUG

TriggerListItem::~TriggerListItem()
{
    //if(t_ && t_->isAttribute()) delete t_;
    //if(dep_ && dep_->isAttribute()) delete dep_;
}

TriggerListCollector::~TriggerListCollector()
{
    for(size_t i=0; i < items_.size(); i++)
    {
        delete items_[i];
    }
}

bool TriggerListCollector::add(VItemTmp_ptr t, VItemTmp_ptr dep,Mode mode)
{
    TriggerListItem *item=new TriggerListItem(t,dep,mode) ;
    items_.push_back(item);
    return true;

#if 0
    if(dep)
    {
        UserMessage::debug(" dep=" + dep->typeName() + " " +  dep->strName());
        UserMessage::debug("    =" + item->dep_->typeName() + " " +  item->dep_->strName());
    }
#endif


#if 0
    // Title
    if(title_)
    {
            int n = fprintf(f_,"\n%s:\n",t_) - 2;
            while(n--) fputc('-',f_);
            fputc('\n',f_);
            t_ = 0;
    }

    p_.observe(&n);
    fprintf(f_,"%s {%s}",n.type_name(), n.full_name().c_str());
    if(p) {
        fprintf(f_," through ");
        p_.observe(p);

        switch(mode)
        {
            case trigger_lister::parent:  fprintf(f_,"parent "); break;
            case trigger_lister::child:   fprintf(f_,"child ");  break;
        }

        fprintf(f_,"%s {%s}",p->type_name(),p->full_name().c_str());
    }
    fputc('\n',f_);
#endif
}

TriggerChildCollector::~TriggerChildCollector()
{
    //if(node_ && node_->isAttribute()) delete node_;
    //if(child_ && child_->isAttribute()) delete child_;
}

bool TriggerChildCollector::add(VItemTmp_ptr t, VItemTmp_ptr,Mode)
{
    if(!t->item()->isAncestor(node_->item()))
    {
        // child is a kid of n whose trigger_panel is outside its subtree
        return collector_->add(t,child_,TriggerCollector::Child);
    }
    return false;
}

TriggerParentCollector::~TriggerParentCollector()
{
    //if(parent_ && parent_->isAttribute()) delete parent_;
}

bool TriggerParentCollector::add(VItemTmp_ptr t, VItemTmp_ptr,Mode)
{
    return collector_->add(t,parent_,TriggerCollector::Parent);
}

bool TriggeredCollector::add(VItemTmp_ptr trigger, VItemTmp_ptr,Mode)
{
    if(VNode *n=trigger->item()->isNode())
    {
        n->addTriggeredData(node_->item());
    }
    return false;

    //else if(trigger->isAttribute())
    //    trigger->parent()->addTriggeredData(node_,trigger);
}

