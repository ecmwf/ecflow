//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #8 $ 
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

#include "node_list.h"
#include "opener.h"
#include "node.h"
#include "host.h"
#include "str.h"
#include "gui.h"
#include "relation.h"
#include "counted.h"
#include "opener.h"
#include <Xm/List.h>

extern "C" {
#include "xec.h"
}

class node_list_data : public counted {
  str name_;
public:
  node_list_data(const char* n) : name_(n) {}
  const char* name() { return name_.c_str(); }
};

node_list::node_list()
{
}

node_list::~node_list()
{
}


void node_list::remove(node* n)
{
  if (forget(n))
    xec_RemoveListItem(list(),(char*)name(n));
}


void node_list::add(node* n,bool sel)
{
  if(n) {
    const char *p = name(n);
    if(xec_AddListItemUnique(list(),(char*)p,sel)) {
      observe(n);
      relation::set_data(this,n,new node_list_data(p));
    }
  }

  static opener o;
  o.show(form());
  gui::raise();
}


void node_list::reset()
{
  forget_all();
  XmListDeleteAllItems(list());
}


node* node_list::find(XmString s)
{
       char *p = xec_GetString(s);
       char *q = p;

       while(*q != ' ') q++;
       *q = 0; q++;
       while(*q == ' ') *q++ = 0;

       node* n =   host::find(p,q);

       if(!n) {
         printf("node_list::find cannot find <%s> <%s>\n",p,q);
       }

       XtFree(p);
       return n;
}

node* node_list::find(const char *p)
{
  xmstring s(p);
       return find(s);
}


const char* node_list::name(node* n)
{
       static char buf[1024];
       sprintf(buf,"%-8s %s",n->serv().name(),n->full_name().c_str());
       return buf;
}


void node_list::notification(observable* o)
{
  node* n = (node*)o;
  if(!keep(n))
    remove(n);
}

void node_list::adoption(observable*,observable* o)
{
  node* n = (node*)o;
  if(!keep(n))
    remove(n);
}

void node_list::gone(observable* o)
{
  node_list_data* p = (node_list_data*)relation::get_data(this,o);
  if(p) xec_RemoveListItem(list(),(char*)p->name());
}
