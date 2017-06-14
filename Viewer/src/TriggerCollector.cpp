//============================================================================
// Copyright 2009-2017 ECMWF.
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

const std::set<TriggerCollector::Mode>& TriggerTableItem::modes() const
{
    if(modes_.empty())
    {
        for(std::size_t i=0; i < deps_.size(); i++)
        {
            modes_.insert(deps_[i].mode());
        }
    }
    return modes_;
}

//=====================================
// TriggerTableCollector
//=====================================

TriggerTableCollector::~TriggerTableCollector()
{
    clear();
}

bool TriggerTableCollector::add(VItem* trigger, VItem* dep,Mode mode)
{
    TriggerTableItem *item=0;
    for(std::size_t i=0; i < items_.size(); i++)
    {
        if(items_[i]->item() == trigger)
        {
            item=items_[i];
            break;
        }
    }

    if(!item)
    {
        item=new TriggerTableItem(trigger) ;
        items_.push_back(item);
    }

    item->addDependency(dep,mode);
    return true;
}

void TriggerTableCollector::setDependency(bool b)
{
    extended_=b;
    clear();
}

void TriggerTableCollector::clear()
{
    for(size_t i=0; i < items_.size(); i++)
    {
        delete items_[i];
    }
    items_.clear();
}



#if 0
void next_node(node& n,node* p,int mode,node* t) {
    t_.relation(&n,g_,p,mode,t);
    n_++;
}

void graph_layout::relation(node* from, node* to,
                node* through, int mode,node *trigger)
{
  graph_node* from_g    = get_graph_node(from);
  graph_node* to_g      = get_graph_node(to);

  from_g->relation(to_g);

  node_relation* n = (node_relation*)from_g->relation_data(to_g);
  while(n)
    {
      if(n->trigger_ == trigger &&
     n->through_ == through &&
     n->mode_    == mode)
    break;

      n = n->next_;
    }

  if(n == 0) {

    n = new node_relation(trigger,through,mode);
    relations_.add(n);

    void* x = from_g->relation_data(to_g,n);
        if(x) n->next_ = (node_relation*)x;
  }

  switch(mode)
    {
    case trigger_lister::normal:
      break;

    case trigger_lister::child:
      /* from_g->relation_gc(to_g,gui::colorGC(STATUS_SUBMITTED)); */
      from_g->relation_gc(to_g,gui::blueGC());
      break;

    case trigger_lister::parent:
      //from_g->relation_gc(to_g,gui::colorGC(STATUS_COMPLETE));
      from_g->relation_gc(to_g,gui::blueGC());
      break;

    case trigger_lister::hierarchy:
      from_g->relation_gc(to_g,gui::colorGC(STATUS_ABORTED));
      break;
    }
}

#endif
