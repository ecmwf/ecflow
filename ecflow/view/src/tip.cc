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

#include <X11/IntrinsicP.h>
#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <stdio.h>
#include "tip.h"
#include "gui.h"
#include "pixmap.h"
extern "C" {
#include "xec.h"
}

tip::tip(Widget w):
	timeout(0.5),
	in_(false),
	widget_(w)
{
  if(XmIsPushButton(w)) {
    XtAddEventHandler(w,EnterWindowMask,False,enterCB,this);
    XtAddEventHandler(w,LeaveWindowMask|ButtonPressMask,False,leaveCB,this);
  }
  
  create(gui::top(),NULL);
  xec_SetLabel(label_,XtName(w));
  XtRealizeWidget(_xd_rootwidget);

  pixmap::find(XtName(w)).set_label(w);
}

tip::~tip()
{
}

void tip::enter()
{
  if (1) { 
    in_ = true; 
    enable(); 
  }
  else 
    in_ = false;
}

void tip::leave()
{
  in_ = false;
  disable();
  XUnmapWindow(XtDisplay(_xd_rootwidget),XtWindow(_xd_rootwidget));
}

void tip::enterCB(Widget w,XtPointer data,XEvent*,Boolean*)
{
  ((tip*)data)->enter();
}

void tip::leaveCB(Widget w,XtPointer data,XEvent*,Boolean*)
{
  ((tip*)data)->leave();
}

#include <Xm/XmP.h>

void tip::makeTips(Widget w)
{
  CompositeWidget c = (CompositeWidget)w;
  for(unsigned int i = 0 ; i < c->composite.num_children; i++)
    {
      Widget p = c->composite.children[i];
      if(XmIsPushButton(p))	new tip(p);
      // else { printf("# xtname: %s\n", XtName(p)); new tip(p); }
    }
}

void tip::run()
{
  Position x,y;	
  Dimension h,w,lw,lh;
  XmString  ls;

  if (widget_==NULL) return;  
  XtTranslateCoords(widget_,0,0,&x,&y);
  
  if (0) {
    XtVaGetValues(widget_,XmNheight,&h,NULL);
    XtVaSetValues(tip_shell,XmNx,x,XmNy,y + h, NULL);
  } else {
    XmFontList fl = NULL;
    XtVaGetValues(widget_, XmNfontList, &fl, NULL);
    if (fl == NULL) return; /* ??? 20120119 */    
    ls = XmStringCreateSimple(XtName(widget_));
    lw = XmStringWidth(fl,  ls)+6;
    lh = XmStringHeight(fl, ls)+6;
    XmStringFree(ls);
    XtVaGetValues(widget_,XmNheight,&h,XmNwidth,&w,NULL);
    XtVaSetValues(tip_shell, XmNx,x + w,XmNy,y + h, 
		  XmNwidth,lw,XmNheight,lh,NULL);
  }
  XMapRaised(XtDisplay(_xd_rootwidget),XtWindow(_xd_rootwidget));
  disable();
}
