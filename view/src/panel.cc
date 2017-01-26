//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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

#include <stdio.h>
#include <Xm/Xm.h>
#include "Hyper.h"


#include "panel.h"
#include "node.h"
#include "host.h"
#include "panel_window.h"
#include "tip.h"
extern "C" {
#include "xec.h"
}

panel::panel(panel_window& owner):
	next_(0),
	node_(0),
	owner_(owner)
{
}

panel::~panel()
{
	delete next_;
}

panel_factory* panel_factory::factories_[PANEL_MAX_FACTORIES];

panel_factory::panel_factory(int n)
{
  if(n < 0 || n >= PANEL_MAX_FACTORIES || factories_[n])
    fprintf(stderr, "panel_factory::panel_factory: internal error %d\n",n);
  factories_[n] = this;
}

panel* panel_factory::create_all(panel_window& w,Widget parent)
{
	panel *first = 0;
	for(int i = 0; i < PANEL_MAX_FACTORIES ; i++)
		if(factories_[i])
		{
			panel* x = factories_[i]->create(w,parent);
			XtManageChild(x->widget());

			if(x->tools())
				tip::makeTips(x->tools());

			x->next_ = first;
			first = x;
		}
	return first;
}

void panel::update()
{
	if(owner_.frozen()) 
		return;

	if(node_)
		show(*node_);
	else
		clear();
}

void panel::post_update()
{
	if(!owner_.frozen())
		enable();
}

void panel::run()
{
	update();
	disable();
}

void panel::detach()
{
	owner_.detach();
}

void panel::freeze()
{
	owner_.freeze();
}

void panel::hyper(Widget,XtPointer data,node *n)
{
	hyperCallbackStruct* cb = (hyperCallbackStruct*)data;
	if(n == 0) n = get_node();
	if(n) n = n->find(cb->text);

	if(n == 0)
	{
		host* h = host::find(cb->text);
		if(h) n = h->top();
	}

	if(n) {
		if(cb->event->xbutton.button == 2)
			owner_.new_window(n);
		else
			selection::notify_new_selection(n);
	}
}

void panel::submit()
{
  owner_.submit();
}
