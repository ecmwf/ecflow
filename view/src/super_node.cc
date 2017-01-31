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

#include "simple_node.h"
#include "host.h"
#include "timeout.h"

#include "super_node.h"

void super_node::why(std::ostream &w)
{
  if(isLocked())
    w << "The server " << this << " is locked\n";
  if (owner_)
    owner_->why(w);
}

void super_node::run()
{
  decay_++;
  /* printf("node run .. %s %d\n",node_name(),decay_); */
  redraw();
}

void super_node::active(bool b)
{
  /* printf("node activate %s %d\n",node_name(),int(b)); */
  if(b != active_) {
    active_ = b;
    redraw();
  }
}

void super_node::up_to_date()
{
  frequency(60); // This will  reset the timeout 
  if(decay_) {
    decay_ = 0;
    redraw();
  }
}

void super_node::drawBackground(Widget w,XRectangle* r,bool)
{
  if(true /*active_*/ ) {
    XRectangle a = *r;
    
    // All grey after 1 hours    
    double x = a.width * ( decay_ / 60.0 );
    
    if(x > a.width) x = a.width;
    
    a.width  -= x;
    
    /* printf("%d %d\n",r->width,a.width); */    
    /* a.width  -= x + 0.5;  a.x += x/2.0 + 0.5; */
    /* a.height -= y + 0.5;  a.y += y/2.0 + 0.5; */
    
#if 0
    printf("%d x y = %g %g %d.%d.%d.%d %d.%d.%d.%d\n", decay_, x , y ,
	   r->x,r->y,r->width,r->height,
	   a.x,a.y,a.width,a.height
	   );
#endif
    
    GC gc = colorGC(STATUS_UNKNOWN);
    XFillRectangles(XtDisplay(w),XtWindow(w),gc,r,1);

    gc = colorGC(status());
    XFillRectangles(XtDisplay(w),XtWindow(w),gc,&a,1);
  }
  else {
    GC gc = colorGC(STATUS_UNKNOWN);
    XFillRectangles(XtDisplay(w),XtWindow(w),gc,r,1);
  }
}

void super_node::info(std::ostream& f)
{
  serv().stats(f);
  simple_node::info(f);
}

// static node_maker<super_node> super_maker(NODE_SUPER);
