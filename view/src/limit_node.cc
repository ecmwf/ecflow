//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #15 $ 
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

#include "node.h"
#include "show.h"
#include "pixmap.h"
#include <math.h>
// #include "text_lister.h"
#include "node_lister.h"
#include "limit_node.h"
#include <Xm/ManagerP.h>
#include "ecf_node.h"

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

static Pixmap images[3] = {XmUNSPECIFIED_PIXMAP, };

Limit* limit_node::get() const {
#ifdef BRIDGE
  if (tree_) return 0x0;
#endif
  if (!owner_.get()) return 0x0;
  return dynamic_cast<ecf_concrete_node<Limit>*>(owner_.get())->get();
}

xmstring limit_node::make_label_tree()
{
  if (get()) {
    xmstring s(owner_->name().c_str(),"bold"); 
    s += xmstring(": ","bold");
    char buf[30];
    sprintf(buf,"%d/%d",get()->value(), get()->theLimit());
    s += xmstring(buf);
    return s; 
  } 
#ifdef BRIDGE
  else if (!tree_) return xmstring ("nolimit");
  sms_limit* n = (sms_limit*) tree_;
  xmstring s(n->name,"bold");
  s += xmstring(": ","bold");

  char buf[80];

  if(n->base)
    sprintf(buf,"%d - %d - %d",n->base,n->status,n->limit);
  else
    sprintf(buf,"%d/%d",n->status,n->limit);

  s += xmstring(buf);
  
  if(n->unit)
    {
      sprintf(buf," (%s)",n->unit);
      s += xmstring(buf);
    }
  
  return s;
#endif
  return xmstring(": ","bold");
}

const int kPixSize = 8;
const int kHMargins = 4;
const int kVMargins = 2;

inline int max(int a,int b) { return a>b?a:b; }

void limit_node::drawNode(Widget w,XRectangle* r,bool tree)
{
  int m = 0, v = 0; m = maximum(); v = value();
  // { Limit *lim = owner_ ? owner_->get_limit("") : 0x0; 
  //   if (lim)  { m = lim->theLimit(); v = lim->value(); }
  // }

  XmString s = labelTree();
  
  XRectangle x = *r;
  x.width      = XmStringWidth(smallfont(),s)  + 2*kHMargins;

  XmStringDraw(XtDisplay(w),XtWindow(w),
	    smallfont(),
	    s,
	    blackGC(),    
	    r->x, 
	    r->y,
	    x.width,
	    XmALIGNMENT_CENTER, XmSTRING_DIRECTION_L_TO_R, NULL);

  for(int i = 0; i < max(m, v); i++)
    XCopyArea(XtDisplay(w),
	      images[(i<v)?(i>=m?2:1):0],XtWindow(w),
	      blackGC(),
	      0,0,kPixSize,kPixSize,
	      r->x + x.width + (i*kPixSize),
	      r->y + (r->height - kPixSize) / 2);

  sizeNode( w, r, tree);
}


void limit_node::sizeNode(Widget w,XRectangle* r,bool tree)
{
  int m = maximum(), v = value();
  if(images[0] == XmUNSPECIFIED_PIXMAP) {
    images[0] = pixmap::find("limit0").pixels();
    images[1] = pixmap::find("limit1").pixels();
    images[2] = pixmap::find("limit2").pixels();
  }
  
  XmString s = labelTree();
  r->height = XmStringHeight(smallfont(),s);
  r->width  = XmStringWidth(smallfont(),s) + 2*kHMargins + max(m,v) * kPixSize;
  if(r->height < kPixSize) r->height = kPixSize;
}

void limit_node::perlify(FILE* f)
{
  perl_member(f,"value",value());
  perl_member(f,"maximum",maximum());
  // FILL perl_member(f,"tasks",n->tasks);  
}

void limit_node::info(std::ostream& f)
{
  Limit* n= get(); if (n) {
    const std::set<std::string>& list = n->paths();
    std::set<std::string>::const_iterator it;
    node::info(f);
  
    f << "value    : " << value() << "\n";
    f << "maximum  : " << maximum()  << "\n";
    //    1234567890
  
    if(!list.empty())
      f << "\nNodes in this limit_node:" << "\n-------------------------\n";
    
    for (it = list.begin(); it != list.end(); ++it) {    
      f << *it;
      node* p = find(*it); 
      if(p) 
	f << "   " << p->type_name() << ' ' 
	  << " (" << p->status_name() << ")\n";
    }
  } 
#ifdef BRIDGE
else if (tree_) {
    sms_limit *n = (sms_limit*)tree_; if (!n) return;
	sms_list*  l = n->tasks;

	node::info(f);

	f << "value    : " << n->status << "\n";
	f << "maximum  : " << n->limit  << "\n";
	//    1234567890

	if(l)
		f << "\nNodes in this limit_node:" 
		  << "\n--------------------\n";
	while(l)
	{
	  node* p = find(l->name);
		if(p) 
		  f << p->type_name() << ' ' << p << " (" 
		    << p->status_name() << ')';
		else
		  f << l->name;
		f << "\n";
		l = l->next;
	}
  }
#endif
}

const char* limit_node::status_name() const
{
  static char buf[20];
  if(value() >= maximum()) return "full";
  if(value() <= 0 )        return "empty";
  sprintf(buf,"%d%%",int((value()*100.0/maximum())+0.5));
  return buf; 
}

bool limit_node::evaluate() const
{
  return (value() >= maximum());  
}

int limit_node::maximum() const
{
  Limit* n= get();
  if (n) return n->theLimit();
#ifdef BRIDGE
  if (tree_) return ((sms_limit*) tree_)->limit;
#endif
  return 0;
}

int limit_node::value() const
{
  if (get()) return get()->value();
#ifdef BRIDGE
  if (tree_) return ((sms_limit*) tree_)->status;
#endif
  return 0;
}

void limit_node::nodes(node_lister& node_list)
{
  Limit* n= get(); 
  if (n) {
    const std::set<std::string>& list = n->paths();
    std::set<std::string>::const_iterator it;
    for (it = list.begin(); it != list.end(); ++it) {
      node* p = find(*it);
      if(p) node_list.next(*p);
      else  node_list.next(*it);
    }    
    return;
  } else {
#ifdef BRIDGE
    sms_limit *n = (sms_limit*)tree_; if (!n) return;
    sms_list*  l = n->tasks;

    while(l) {
      node* p = find(l->name);
      if(p) node_list.next(*p);
      l = l->next;
    }
#endif
  }
}

bool limit_node::match(const char* p)
{
  return p == parent()->full_name() + ":" + name();
}

//===============================================================

const double kLength   = 30;
const double kMark     = 5;
const double kVuHeight = 2 + (kLength + kMark); 
const double kVuWidth  = (kLength + kMark)*2.0;



void limit_integer_node::sizeNode(Widget w,XRectangle* r,bool)
{
#if 1
	XmString s = labelTree();
	r->width   = XmStringWidth(smallfont(),s) +  2 * kHMargins ;
	r->height  = XmStringHeight(smallfont(),s) + 2 * kVMargins + kVuHeight;

	if(r->width < kVuWidth) r->width  = kVuWidth;
#else
	r->width   = kVuWidth;
	r->height  = 2 * kVMargins  + kVuHeight;
#endif
}

void limit_integer_node::drawNode(Widget w,XRectangle* r,bool)
{
	XRectangle y = *r;
	y.width  = kVuWidth;
	y.height = kVuHeight;
	y.y += kVMargins;
	drawMeter(w,&y);

	char buffer[1024];
	if (get()) sprintf(buffer,"%s",get()->name().c_str());
#ifdef BRIDGE
	else if (tree_) sprintf(buffer,"%s",((sms_limit*)tree_)->name);
#endif
	xmstring s(buffer);
	XmFontList f = gui::tinyfont();

	XmStringDraw(XtDisplay(w),XtWindow(w),
	    f,
	    s,
	    blackGC(),    
	    r->x, 
	    r->y + (r->height - XmStringHeight(f,s)) / 2 ,
	    r->width,
	    XmALIGNMENT_CENTER, XmSTRING_DIRECTION_L_TO_R, r);	
#if 0
	y   = *r;
	y.y += kVuHeight + kVMargins;

	XmString s = labelTree();
	XmStringDraw(XtDisplay(w),XtWindow(w),
	    smallfont(),
	    s,
	    blackGC(),    
	    y.x, 
	    y.y,
	    y.width,
	    XmALIGNMENT_CENTER, XmSTRING_DIRECTION_L_TO_R, NULL);

	/* shadow(w,r); */
#endif
  node::update(-1,-1,-1);notify_observers();
}

void limit_integer_node::drawMeter(Widget w,XRectangle* r)
{
#if 0
	int xcenter = 80;
	int ycenter = 55;
	int lastxs  = xcenter;
	int lastys  = ycenter;

	double min_ = 0;
	double max_ = 5000;
	double v_   = 15;

	int length = 30;
	int mark  = 5;

	int ticks = 5;
#endif
	const double round  = 20;

	const double angle = 120;
	const double pi2   = M_PI_2;
	const double twopi = M_PI * 2.0;
	const int ticks = 5;

	const double a = angle / 360.0 * twopi;

	double maxval = maximum(); // n->limit;
	double minval = 0.0;       // n->min;
	double curval = value();   // n->status;

	double	xcenter = r->width / 2.0;
	double	ycenter = r->height;

	double d = (maxval - minval);
	double c = (curval - minval)/d*a - a/2.0;

	if(c > pi2) c = pi2;

	int xs = cos(c  - pi2) * kLength + xcenter;
	int ys = sin(c  - pi2) * kLength + ycenter;

	GC gc = (curval > maxval) ? redGC() : blueGC();

	XSetLineAttributes(XtDisplay(w),gc, 2,0,0,0);

	XDrawLine(XtDisplay(w),XtWindow(w),
		gc,
		r->x + xcenter, r->y + ycenter-1, r->x + xs, r->y + ys);

	XSetLineAttributes(XtDisplay(w),gc, 1,0,0,0);
	/*
	// --- base
	if(n->base) {

		double c1 =  - a/2.0;
		double c2 =  (n->base - minval)/d*a - a/2.0;

		c1 = twopi - c1 + pi2;
		c2 = twopi - c2 + pi2;
		// c1 -= pi2; 
		// c2 -= pi2; 

		double size = kLength+kMark + 2;

		XFillArc(XtDisplay(w),XtWindow(w),
			XmParentBottomShadowGC(w),
			r->x + xcenter - size,
			r->y + ycenter - size,
			size * 2,
			size * 2,

			(c1 / twopi * 360) * 64,
			(c2 / twopi * 360) * 64   - (c1 / twopi * 360) * 64);
	}
	*/
#if 0
		g.setColor(Color.gray);
		g.fillArc(xcenter-round/2,ycenter-round/2,round,round,0,180);
#endif

	for(int i = 0 ; i < ticks ; i++)
	{
		double v = i*d/(ticks-1);
		c = v/d*a - a/2;
		int x1 = cos(c  - pi2) * kLength + xcenter;
		int y1 = sin(c  - pi2) * kLength + ycenter;
	
		int x2 = cos(c  - pi2) * (kLength+kMark) + xcenter;
		int y2 = sin(c  - pi2) * (kLength+kMark) + ycenter;

		XDrawLine(XtDisplay(w),XtWindow(w),
			blackGC(),r->x + x1,r->y + y1, r->x + x2, r->y + y2);
	}

	XFillArc(XtDisplay(w),XtWindow(w),
		XmParentBackgroundGC(w),
		r->x + xcenter-round/2,
		r->y + ycenter-round/2,
		round,
		round,0,180*64);
	
	const	int drop_shadow  = 10;
	XDrawArc(XtDisplay(w),XtWindow(w),XmParentBottomShadowGC(w),
		r->x + xcenter-round/2,
		r->y + ycenter-round/2,round,round,64,(90-drop_shadow) * 64);

	XDrawArc(XtDisplay(w),XtWindow(w),XmParentTopShadowGC(w),
		r->x + xcenter-round/2,
		r->y + ycenter-round/2,round,round, (90+drop_shadow)*64,180*64 - (90+drop_shadow)*64);


	char buf[80];
	sprintf(buf,"%d",value());

	xmstring t(buf);
	XmFontList f = gui::tinyfont();

	XmStringDraw(XtDisplay(w),XtWindow(w),
		f,
		t,
		(curval > maxval) ? redGC() : blueGC(),
		r->x + xcenter-round/2 + 2,
		r->y + ycenter-round/2 + 3,
		round,
	    XmALIGNMENT_CENTER, XmSTRING_DIRECTION_L_TO_R, NULL);

	shadow(w,r,false);
}

//===============================================================


//===============================================================

Boolean limit_node::visible() const { return show::want(show::limit); }

