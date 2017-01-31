//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #36 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

// .AUTHOR   Baudouin Raoult
// .DATE     28-JUN-2001 / 28-JUN-2001

#include "arch.h"
#include "repeat_node.h"
#include "show.h"
#include "host.h"
const int  REPEAT_SHOWN = 3;
#include "ecflowview.h"

#ifndef ecf_node_
#include "ecf_node.h"
#endif

const int kPixSize  = 8;
const int kHMargins = 4;
const int kVMargins = 2;

repeat_node::repeat_node(host& h,ecf_node* n) 
  : node(h,n)
  , name_("none")
{
  if (get()) 
    name_ = get()->name();
  else if (n)  
    name_ = n->name();
  if (n) {
    if (n->parent() && n->parent()->get_node())
      full_name_ = n->parent()->get_node()->absNodePath();
    full_name_ += ":";
    full_name_ += n->name(); 
  }
}

#ifdef BRIDGE
repeat_node::repeat_node(host& h,sms_node* n,char b) 
  : node(h,n,b) 
  , name_(n ? n->name : "unknown")
{
  full_name_ = n ? sms_node_full_name(n) : "unknown";
}
#endif

/**********************************************/

RepeatBase* repeat_node::get() const
{
  if (parent()) 
    if (parent()->__node__())
      return parent()->__node__()->get_repeat().repeatBase();
  Node * nnn = 0x0;

  if (nnn) 
    return nnn->findRepeat(full_name_).repeatBase();      
  return 0x0;
}

 void repeat_node::drawNode(Widget w,XRectangle* r,bool tree) {
   node::update(-1,-1,-1);
   node::drawNode(w,r,true);
   sizeNode(w,r,tree);
}

void repeat_node::sizeNode(Widget w,XRectangle* r,bool tree) {
  int extra = 0;
  XmString   s = tree?labelTree():labelTrigger();
  r->width  = XmStringWidth(smallfont(),s)  + 2*kHMargins +  extra * kPixSize;
  r->height = XmStringHeight(smallfont(),s) + 2*kVMargins;
  if(r->height < kPixSize + 2*kVMargins) r->height = kPixSize + 2*kVMargins;
}

int repeat_node::start() const
{
#ifdef BRIDGE
  if (tree_) 
    return ((sms_repeat*) tree_)->start;
#endif
  if (get()) 
    return get()->start();
  return 0;
}

int repeat_node::last() const
{
#ifdef BRIDGE
  if (tree_) { 
    sms_repeat* r = (sms_repeat*)tree_;
    switch (r->mode) {
    case REPEAT_INTEGER: 
      return (r->end - r->start)/r->step + 1;
    case REPEAT_ENUMERATED:    
    case REPEAT_STRING: { int n = 0; sms_list *l = r->str;
        while(l) { n++; l = l->next; }
        return n; }
    case REPEAT_DATE: {
      return (ecf_repeat_date_to_julian(r->end) - 
              ecf_repeat_date_to_julian(r->start))/
        r->step + 1; }
    default: 
      return r->end;
    }
  }
#endif
  if (get()) 
    return get()->end(); 
  return 0;
}

int repeat_node::step() const
{
#ifdef BRIDGE
  if (tree_) 
    return ((sms_repeat*) tree_)->step;
#endif
  if (get()) 
    return get()->step() > 0 ? get()->step() : 1;
  return 1;
}

int repeat_node::current() const
{
#ifdef BRIDGE
  if (tree_) { 
    sms_repeat* r = (sms_repeat*)tree_;
    switch (r->mode) {
    case REPEAT_INTEGER: 
      return (r->status - r->start)/r->step;
    case REPEAT_ENUMERATED: { int rc = 0;
       sms_list *item = r->str;
       char current[255];
       while (item) {
         sprintf(current,"%d",r->status);
         if (!strcmp(item->name, current)) 
	   return rc;
         rc++; item = item->next;
       }
       return rc; }
    case REPEAT_STRING: 
      return r->status;
    case REPEAT_DATE: 
      return (ecf_repeat_date_to_julian(r->status) - 
              ecf_repeat_date_to_julian(r->start))/r->step; 
    default: 
      return r->status;
    }
  }
#endif
  if (get()) {
    return get()->index_or_value();    
  }
  return 0;
}

void repeat_node::value(char* n,int i) const
{
#ifdef BRIDGE
  if (tree_) {
    sms_repeat* r = (sms_repeat*)tree_;
    switch (r->mode) {
    case REPEAT_INTEGER: { 
      sprintf(n,"%d",r->start + i*r->step); 
      return; }
    case REPEAT_ENUMERATED:    
    case REPEAT_STRING: { int j = 0; sms_list *l = r->str;
        while(l && j != i) { j++; l = l->next; } if(l) strcpy(n,l->name); 
	return; }
    case REPEAT_DATE: {
      sprintf(n,"%ld",ecf_repeat_julian_to_date
              (ecf_repeat_date_to_julian(r->start) + i*r->step)); 
      return; }
    default: 
      return;
    }
  }
#endif
  if (get() && n) {
    sprintf(n,"%s",get()->value_as_string(i).c_str());
  }
}

//===============================================================

xmstring repeat_node::make_label_tree()
{
        str80 vals[REPEAT_SHOWN];
        str80 buf;
        int end    = last();
        int curr   = current();
        int first  = curr - REPEAT_SHOWN/2;
        static xmstring space(" ");
        {
        if(first<0)                    first = 0;
        if(end -first<REPEAT_SHOWN)    first = end  - REPEAT_SHOWN;
        if(end <=REPEAT_SHOWN)         first = 0;

        int m = MIN(REPEAT_SHOWN,end);
        int i;

        for(i = 0 ; i < REPEAT_SHOWN ; i++)
                vals[i][0] = 0;

        for(i=0 ; i < m ; i++)
                value(vals[i],i+first);

        if(first>0)          
                strcpy(vals[0],      "...");

        if( first+REPEAT_SHOWN < end) 
                strcpy(vals[REPEAT_SHOWN-1],"...");

        curr = curr - first;

        strcpy(buf,name_.c_str());
        strcat(buf,"=");

        xmstring s(buf);

        m = 0;
        for(i = 0; i < REPEAT_SHOWN ; i ++)
        {               
                if(m != 0)
                        s += space;

                s += xmstring(vals[i], (i==curr)?"bold":"normal" );

                m = strlen(vals[i]);
        }

        return s;
        }
}

void repeat_node::info(std::ostream& f)
{
  if (get()) {
    f << get()->toString() << "\n";
  }
  f << "Values are:\n";
  f << "-----------\n";

  str80 buf;

  int end = last();
  int cur = current();
  int i;

  if(end > 50) {
    for(i=0 ; i < 22 ; i++) {
      value(buf,i);
      f << char( (i == cur) ? '>' : ' ') << buf << "\n";
    }
    
    f << "...\n";

    for(i= end-22 ; i < end ; i++) {
      value(buf,i);
      f << char( (i == cur) ? '>' : ' ') << buf << "\n";
    }
    return;
  }

  for(i=0 ; i < end ; i++) {
    value(buf,i);
    f << char( (i == cur) ? '>' : ' ') << buf << "\n";
  }
  f << "-----------" << "\n";
}

const char* repeat_node::status_name() const
{
  static char buf[80];
  int end    = last();
  int cur    = current();

  if(cur < 0)    
    return "not started";
  if(cur >= end) 
    return "finished";

  value(buf,cur);
  return buf;
}

void repeat_node::perlify(FILE* f)
{
  perl_member(f,"start",  start());
  perl_member(f,"end",    last());
  perl_member(f,"step",   step());
  perl_member(f,"current",current());  
}

Boolean repeat_node::visible() const { 
  return show::want(show::repeat); 
}
