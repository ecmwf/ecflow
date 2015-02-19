//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #22 $ 
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

#include "show.h"
#include "meter_node.h"
#include "text_lister.h"
#include "ecf_node.h"
#include "ecflowview.h"
#include "NodeAttr.hpp"

#ifdef BRIDGE
meter_node::meter_node(host& h,sms_node* n, char b) 
  : node (h,n,b) 
  , name_ (n ? n->name : "STEP")
{}
#endif

meter_node::meter_node(host& h,ecf_node* n) 
  : node  (h,n)
  , name_ (n ? n->name() : "STEP") 
{
}

const Meter&  meter_node::get() const {
  ecf_concrete_node<const Meter>* base = 
    dynamic_cast<ecf_concrete_node<const Meter>*> (owner_);
  if (base) return *(base->get());
  if (parent() && parent()->__node__())
      return parent()->__node__()->get_meter(name_);
  return Meter::EMPTY();
}

xmstring meter_node::make_label_tree()
{
  char buff[80], name[80];
  snprintf(name,80, " %s: ", name_.c_str());
  snprintf(buff,80, "%d", value());
  return xmstring(name,"bold") + xmstring(buff);
}

void meter_node::drawNode(Widget w,XRectangle* r,bool tree)
{
	drawBackground(w,r,tree);

	XmString s = tree?labelTree():labelTrigger();
	XRectangle x = *r;
	int width,mark;

	x.x += (x.height - 10)/2;
	x.height = 10;
	x.width  = 50;

	width = (float)x.width / (float)(maximum() - minimum())
	  * (float)(value() - minimum());

	mark  = (float)x.width / (float)(maximum() - minimum())
	  * (float)(threshold() - minimum());

	XFillRectangles(XtDisplay(w),XtWindow(w),colorGC(0),&x,1);

	if (value() > threshold())
          XFillRectangle(XtDisplay(w),XtWindow(w),gui::colorGC(STATUS_MAX+1),x.x,x.y,width,x.height);
        else
          XFillRectangle(XtDisplay(w),XtWindow(w),gui::colorGC(STATUS_MAX),x.x,x.y,width,x.height);

	shadow(w,&x);

	if(mark < width)
	{
		x.width = mark;
		shadow(w,&x);
	}

	XmStringDraw(XtDisplay(w),XtWindow(w),
	    smallfont(),
	    s,
	    blackGC(),    
	    r->x + 52,
	    r->y,
	    r->width - 52,
	    XmALIGNMENT_CENTER, XmSTRING_DIRECTION_L_TO_R, NULL);

	node::update(-1,-1,-1);
}

void meter_node::sizeNode(Widget w,XRectangle* r,bool tree)
{
	XmString s = tree?labelTree():labelTrigger();
	r->height = XmStringHeight(smallfont(),s);
	r->width  = XmStringWidth(smallfont(),s) + 54 ;
	if(r->height<10) r->height = 10;
}

const char* meter_node::status_name() const
{
  static char buf[10];
  sprintf(buf,"%d",value());
  return buf;
}

void meter_node::info(std::ostream& f)
{
  node::info(f);
  f << "value    : " << value()     << "\n";
  f << "minimum  : " << minimum()   << "\n";
  f << "maximum  : " << maximum()   << "\n";
  f << "threshold: " << threshold() << "\n";
}

int meter_node::minimum()
{
#ifdef BRIDGE
  if (tree_) return ((sms_meter*) tree_)->min;
#endif
  return get().min();
}

int meter_node::maximum()
{
#ifdef BRIDGE
  if (tree_) return ((sms_meter*) tree_)->max;
#endif
  return get().max();
}

int meter_node::value() const
{
#ifdef BRIDGE
  if (tree_) return ((sms_meter*) tree_)-> status;
#endif
  return get().value();
}

int meter_node::threshold()
{
#ifdef BRIDGE
  if (tree_) return ((sms_meter*) tree_)->color;
#endif
  return get().colorChange();
}

void meter_node::perlify(FILE* f)
{
}

Boolean meter_node::visible() const { return show::want(show::meter); }


