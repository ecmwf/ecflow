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
#include "VNode.hpp"

class TriggerListItem
{
public:
    TriggerListItem(VItem* trigger,VItem* dep,TriggerCollector::Mode mode) :
        trigger_(trigger), dep_(dep), mode_(mode) {}

    VItem* trigger_;
    VItem* dep_;
    TriggerCollector::Mode mode_;
};


void TriggerListCollector::add(VItem* trigger, VItem* dep,Mode mode,VItem*)
{
    TriggerListItem *item=new TriggerListItem(trigger,dep,mode) ;
    items_.push_back(item);

    if(dep)
    {
        UserMessage::debug(" dep=" + dep->typeName() + " " +  dep->strName());
        UserMessage::debug("    =" + item->dep_->typeName() + " " +  item->dep_->strName());
    }


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

QString TriggerListCollector::text()
{
    QString s="<table>";

    s+="<tr><td colspan=\'2\'>Nodes triggering this node</td></tr>";
    for(unsigned int i=0; i < items_.size(); i++)
    {
         s+="<tr>";

         VItem *t=items_[i]->trigger_;
         s+="<td>" + QString::fromStdString(t->typeName()) + "</td>";
         s+="<td>" + t->name() +"</td>";
         s+="<td><a href=\'aa\'>" + QString::fromStdString(t->fullPath()) +"</a></td>";

         VItem *d=items_[i]->dep_;
         if(d)
         {

             UserMessage::debug(" dep=" + d->typeName() + " " +  d->strName());

             s+="<td>" + QString::fromStdString(d->typeName());
             s+=" " + d->name();

             if(items_[i]->mode_== Parent)
                s+=" through parent";
             else
                s+=" through child";
             s+=" <a href=\'aa\'>" + QString::fromStdString(d->fullPath()) +"</a></td>";
         }

         s+="</tr>";
    }

    s+="</table>";
    return s;

}


void TriggerChildCollector::add(VItem* n, VItem*,Mode mode,VItem *t)
{
    //if(!n->is_my_parent(node_))
    {
        // child is a kid of n whose trigger_panel is outside its subtree
        collector_->add(n,child_,TriggerCollector::Child,t);
    }
}

void TriggerParentCollector::add(VItem* n, VItem*,Mode mode,VItem *t)
{
    collector_->add(n,parent_,TriggerCollector::Parent,t);
}

void TriggeredCollector::add(VItem* n, VItem* parent,Mode mode,VItem*)
{
    //if(n->isAttribute())
    //   if(n->parent() == node_)
    //        items_.push_back(n);
}

