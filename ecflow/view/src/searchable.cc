//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

#include "top.h"
#include "searchable.h"
#include <Xm/Text.h>
#include <Xm/ToggleB.h>

Widget searchable::parent_ = 0;

searchable::searchable():
	toggle_(0),
	active_(False)
{
}

searchable::~searchable()
{
	if(toggle_)
		XtDestroyWidget(toggle_);
}

void searchable::look_for(node_lister& p,bool all)
{
	searchable* s = first();
	while(s)
	{
		if( all || (s->toggle_ && XtIsManaged(s->toggle_) && 
						 XmToggleButtonGetState(s->toggle_)))
			s->search(p);
		s = s->next();
	}
}

void searchable::parent(Widget w)
{
  parent_ = w;
  
  searchable* s = first();
  while(s)
    {
      if(s->toggle_ == 0)
	{
	  s->toggle_ = XmCreateToggleButton(parent_,(char*)s->name(),0,0);
	  if(s->active_) XtManageChild(s->toggle_);
	}
      s = s->next();
    }
}

void searchable::active(Boolean a)
{
	active_ = a;

	if(!toggle_ && parent_)
		parent(parent_);

	if(toggle_) {
		if(active_) 
			XtManageChild(toggle_);
		else 
			XtUnmanageChild(toggle_);
	}
}

IMP(searchable)



#if 0
static char sccsid[] = "%W% %E% B.Raoult";

#include "ecflowview.h"
#include <Xm/List.h>
#include <Xm/Text.h>

static Widget text;
static int    ignorecase = TRUE;
static Widget last_shell = NULL;

void search_set_text_callback(Widget w,Widget t,XtPointer cb)
{
	char *p;
	Arg arg;

	text = t;
	while(!XtIsShell(t))
		t = XtParent(t);

	last_shell = t;

	XtSetArg(arg,XmNtitle,&p);
	XtGetValues(t,&arg,1);

	xec_SetLabel(label_search_text,p);

	XtManageChild(search_form);
	XMapRaised(XtDisplay(search),XtWindow(search));

}

void hide_search_callback(Widget w,XtPointer cd,XtPointer cb)
{

	while(!XtIsShell(w))
		w = XtParent(w);

	if(last_shell == w)
	{
		XtUnmanageChild(search_form);
		last_shell = NULL;
	}
}


void search_case_callback(Widget w,int nocase,
XmToggleButtonCallbackStruct *cb)
{
	if(cb->set) ignorecase = nocase;
}

void search_next_callback(Widget w,XtPointer cd,XtPointer cb)
{
	char *p = (char*)XmTextGetString(search_text);
	Boolean (*f)(Widget,char*,Boolean,Boolean,Boolean);

	if(!*p) return;
	if(XmIsList(text)) 
		f = xec_ListSearch;
	else 
		if(XmIsText(text)) 
			f = xec_TextSearch;
	else f = help_search;

	if(!(*f)(text,p,ignorecase,FALSE,TRUE)) ring(1);

	XtFree((XtPointer)p);
}
#endif
