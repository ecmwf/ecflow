//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #7 $ 
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

#include "event_node.h"

#ifndef ecf_node_
#include "ecf_node.h"
#endif

event_node::event_node(host& h,ecf_node* n) : node(h,n) {}

const char* event_node::status_name() const 
{ 
  char *event_name[]  = { (char*) "clear", (char*) "set", NULL };
  return event_name[owner_->status()]; 
}

void event_node::drawNode(Widget w,XRectangle* r,bool tree)
{
	drawBackground(w,r,tree);

	XmString s = tree ? labelTree() : labelTrigger();
	XRectangle x = *r;

	x.x += (x.height - 10)/2;
	x.width = x.height = 10;

	XFillRectangles
	  (XtDisplay(w),XtWindow(w),
	   // status() ? blueGC() : colorGC(0), &x,1);
	   status() ? gui::colorGC(STATUS_MAX+2) : colorGC(0), &x,1);

	shadow(w,&x);

	XmStringDraw(XtDisplay(w),XtWindow(w),
	    smallfont(),
	    s,
	    blackGC(),    
	    r->x + x.width + 4,
	    r->y,
	    r->width - x.width - 4,
	    XmALIGNMENT_CENTER, XmSTRING_DIRECTION_L_TO_R, 
		     NULL);
}

void event_node::sizeNode(Widget w,XRectangle* r,bool tree)
{
	XmString s = tree ? labelTree() : labelTrigger();
	r->height = XmStringHeight(smallfont(),s);
	r->width  = XmStringWidth(smallfont(),s) + 14 ;
	if(r->height<10) r->height = 10;
}

bool event_node::evaluate() const
{
  return status() != 0;
}

void event_node::perlify(FILE* f)
{
  perl_member(f,"status",owner_->status());
}

