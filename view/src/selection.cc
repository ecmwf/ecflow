//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #9 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include "selection.h"
#include "node.h"
#include "observer.h"
#include "host.h"

class selection_observer :public observer {
	node* n_;

	void gone(observable*);
	void adoption(observable*,observable*);
	void notification(observable*);

public:

	selection_observer() : n_(0) {}
	void  set(node*);
	node* get() { return n_; }
};

static selection_observer current;
static selection_observer menu;

void selection_observer::set(node* n)
{
	if(n == n_)
	  return;

	if(n_) 
	  forget(&(n_->serv()));

	forget(n_);
	n_ = n;
	observe(n_);

	if(n_) 
	  observe((&n_->serv()));
}

void selection_observer::adoption(observable* o,observable* n)
{
  if(o == n_)
    n_ = (node*)n;
  else
    fprintf(stderr, "Selection adoption: bad value\n");
}

void selection_observer::gone(observable*)
{
  // printf("Selection gone\n");
  n_ = 0;
}

void selection_observer::notification(observable*)
{
}


selection::selection()
{
}

selection::~selection()
{
}

void selection::notify_new_selection(node* n)
{
  if(n == current.get())
    return;
  
  if(n == 0) {
    notify_selection_cleared();
    return;
  }
  
  // printf("selection is %s %s %02d\n", n->full_name().c_str(), n->type_name(), n->type());
  
  if(!n->selectable()) 
    return;
  
  selection* w = first();
  
  current.set(n);
  
  while(w) {
    w->new_selection(*n);
    w = w->next();
  }
}

void selection::notify_selection_cleared()
{
  if(current.get() == 0)
    return;
  
  // printf("selection is cleared\n");
  
  selection* w = first();
  
  current.set(0);
  
  while(w)
    {
      w->selection_cleared();
      w = w->next();
    }
}

void selection::menu_node(node* n)
{
	menu.set(n);
}

node *selection::menu_node()
{
  return menu.get();
}

node *selection::current_node()
{
  return current.get();
}

IMP(selection)
