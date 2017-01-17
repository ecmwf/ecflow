/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #5 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2017 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ConstrainP.h>
#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include <Xm/DrawingAP.h>
#include <Xm/ExtObjectP.h>
#include "SimpleGraph.h"
#include "SimpleGraphP.h"

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define LEVEL 0
#define ARC   1
#define VISIT 2
#define GROUP 3

#define round(X) (((X) >= 0) ? (int)((X)+0.5) : (int)((X)-0.5))
#define RADIANS(x) (M_PI * (x) / 180.0)
#define ABS(a) ((a)>0?(a):(-(a)))
#define C(w) (SIMPLEGRAPH_CONSTRAINT(w)->simplegraph)


static void Initialize();
static void Destroy();
static Boolean SetValues();
static void Redisplay();
static void compute_positions(SimpleGraphWidget,int);
static void set_positions(SimpleGraphWidget,long*,long*);
static void Print();
static void Layout(Widget,long*,long*);
static void bezier_find(SimpleGraphWidget,XEvent*);

static XtResource resources[] = {
	{XtNhorizontalSpace,XtCSpace,XtRDimension,sizeof(Dimension),
	XtOffset(SimpleGraphWidget, simplegraph.h_min_space), XtRString,"30" },
	{XtNverticalSpace,XtCSpace, XtRDimension,sizeof (Dimension),
	XtOffset(SimpleGraphWidget, simplegraph.v_min_space), XtRString,"10" },
	{XtNarrowAngle,XtCArrowAngle, XtRDimension,sizeof (Dimension),
	XtOffset(SimpleGraphWidget, simplegraph.arrow_angle), XtRString,"22" },
	{XtNarrowLength,XtCArrowLength, XtRDimension,sizeof (Dimension),
	XtOffset(SimpleGraphWidget, simplegraph.arrow_length), XtRString,"8" },
	{XtNarrowFilled,XtCArrowFilled, XtRBoolean,sizeof (Boolean),
	XtOffset(SimpleGraphWidget, simplegraph.arrow_filled), XtRString,"false" },
	{"round","Round", XtRBoolean,sizeof (Boolean),
	XtOffset(SimpleGraphWidget, simplegraph.mode), XtRString,"false" },
	{XtNarcOnly,XtCArcOnly, XtRPointer,sizeof(Widget),
	XtOffset(SimpleGraphWidget, simplegraph.arc_only), XtRPointer,NULL},
};

/*
static XtResource simplegraphConstraintResources[] = {
 {XtNarcNumber,XtCArcNumber, XtRint,sizeof(int),
 XtOffset(SimpleGraphConstraints, simplegraph.misc[ARC]), XtRString,"0" },
};
*/

SimpleGraphClassRec simplegraphClassRec = {
	{
	/* core_class fields */
	(WidgetClass) &simplebaseClassRec, /* superclass */
	"SimpleGraph", /* class_name */
	sizeof(SimpleGraphRec), /* widget_size */
	NULL, /* class_init */
	NULL, /* class_part_init */
	FALSE, /* class_inited */
	Initialize, /* initialize */
	NULL, /* initialize_hook */
	XtInheritRealize, /* realize */
	NULL, /* actions */
	0, /* num_actions */
	resources, /* resources */
	XtNumber(resources), /* num_resources */
	NULLQUARK, /* xrm_class */
	TRUE, /* compress_motion */
	XtExposeCompressMaximal, /* compress_exposure */
	TRUE, /* compress_enterleave*/
	TRUE, /* visible_interest */
	Destroy, /* destroy */
	NULL, /* resize */
	Redisplay, /* expose */
	SetValues, /* set_values */
	NULL, /* set_values_hook */
	XtInheritSetValuesAlmost, /* set_values_almost */
	NULL, /* get_values_hook */
	NULL, /* accept_focus */
	XtVersion, /* version */
	NULL, /* callback_private */
	XtInheritTranslations, /* tm_table */
	NULL, /* query_geometry */
	XtInheritDisplayAccelerator, /* display_accelerator*/
	NULL, /* extension */
	},
	{
	/* simplebase_class fields */
	NULL, /* geometry_manager */
	NULL, /* change_managed */
	XtInheritInsertChild, /* insert_child */
	XtInheritDeleteChild, /* delete_child */
	NULL, /* extension */
	},
	{ 
	/* constraint_class fields */
	NULL, /* subresources */
	0, /* subresource_count */
	0, /* constraint_size */
	NULL, /* initialize */
	NULL, /* destroy */
	NULL, /* set_values */
	NULL, /* extension */
	},
	{
	XtInheritTranslations, /* default translations */
	NULL, /* syn_resources */
	0, /* num_syn_resources */
	NULL, /* syn_cont_resources */
	0, /* num_syn_cont_resources */
	XmInheritParentProcess, /* parent_process */
	NULL, /* extension */

	},
	{
	NULL,
	},
	{
	Print,
	Layout,
	},
	{
	/* SimpleGraph class fields */
	0, 
	},
};

#ifdef TOP_BOTTOM

#define TMPX   tmpx
#define TMPY   tmpy
#define WIDTH  width
#define HEIGHT height
#define H_DIST h_dist
#define V_DIST v_dist
#define H_MIN_SPACE h_min_space
#define V_MIN_SPACE v_min_space

#else

#define TMPX   tmpy
#define TMPY   tmpx
#define WIDTH  height
#define HEIGHT width
#define H_DIST v_dist
#define V_DIST h_dist
#define H_MIN_SPACE v_min_space
#define V_MIN_SPACE h_min_space

#endif

/* #define MANAGED(n) ((n)->managed && ((n)->group == -1)) */
#define MANAGED(n) ((n)->managed)

WidgetClass simplegraphWidgetClass = (WidgetClass) &simplegraphClassRec;

static void button_click(w,cd,event,continue_dispatch)
SimpleGraphWidget    w;
XtPointer *cd;
XEvent    *event;
Boolean   *continue_dispatch;
{
	bezier_find(w,event);
}

static void make_gc(SimpleGraphWidget w)
{
}

static void delete_gc(SimpleGraphWidget w)
{
}

static void Initialize(request, new)
SimpleGraphWidget request, new;
{
  /* XGCValues values;
     XtGCMask valueMask;
     int i; */
	/*
 * Make sure the widget's width and height are 
 * greater than zero.
 */
	if (request->core.width <= 0)
		new->core.width = 5;
	if (request->core.height <= 0)
		new->core.height = 5;

	new->simplegraph.cos_arrow = cos(RADIANS(new->simplegraph.arrow_angle));
	new->simplegraph.sin_arrow = sin(RADIANS(new->simplegraph.arrow_angle));

	/*
 * Create a simplegraphics context for the connecting lines.
 */

	make_gc(new);

	/*
 * Create the hidden root widget.
 */

	/*
 * Allocate the tables used by the layout
 * algorithm.
 */

   new->simplegraph.gc[0]    = new->simplebase.gc;
   new->simplegraph.gc_count = 1;

	XtAddEventHandler((Widget)new,ButtonPressMask,
	    False,(XtEventHandler)button_click,(XtPointer)new);

}

static void Destroy(w)
SimpleGraphWidget w;
{
	delete_gc(w);
	XtRemoveEventHandler((Widget)w,ButtonPressMask,
	    False,(XtEventHandler)button_click,(XtPointer)w);
}



static Boolean SetValues(current, request, new)
SimpleGraphWidget current, request, new;
{
	int redraw = TRUE;
	long w,h;
	/* int i;
	int new_gc = new->core.background_pixel != current->core.background_pixel; */


	if (new->simplegraph.arrow_angle != current->simplegraph.arrow_angle)
	{
		new->simplegraph.cos_arrow = cos(RADIANS(new->simplegraph.arrow_angle));
		new->simplegraph.sin_arrow = sin(RADIANS(new->simplegraph.arrow_angle));
	}

	if (new->simplegraph.arc_only != current->simplegraph.arc_only)
	{
		Layout((Widget)new,&w,&h);
		redraw = FALSE;
	}

	/*
 * If the minimum spacing has changed, recalculate the
 * simplegraph layout. new_layout() does a redraw, so we don't
 * need SetValues to do another one.
 */
	if (new->simplegraph.v_min_space != current->simplegraph.v_min_space ||
	    new->simplegraph.h_min_space != current->simplegraph.h_min_space){
		Layout((Widget)new,&w,&h);
		redraw = FALSE;
	}
	return (redraw);
}


/*
static int first_kid(SimpleGraphWidget w,NodeStruct *n)
{
	int i;
	for(i=0;i<n->pcnt;i++)
		if(MANAGED(&PARENTS(w,n,i)))
			return i;
	return -1;
}

static int last_kid(SimpleGraphWidget w,NodeStruct *n)
{
	int i;

	if(n->pcnt)
		for(i= n->pcnt - 1;i>=0;i--)
			if(MANAGED(&PARENTS(w,n,i)))
				return i;
	return -1;
	} 


static void line(SimpleGraphWidget w,int x1,int y1,int x2,int y2,int gc)
{
	GC topGC = w->manager.top_shadow_GC;
	GC midGC = w->manager.background_GC;
	GC botGC = w->manager.bottom_shadow_GC;
	if(gc) midGC = w->simplegraph.gc[gc % GC_COUNT];

	if(x1 == x2)
	{
		XDrawLine(XtDisplay(w),XtWindow(w),
		    topGC,x1-1,y1,x2-1,y2);
		XDrawLine(XtDisplay(w),XtWindow(w),
		    midGC,x1,y1,x2,y2);
		XDrawLine(XtDisplay(w),XtWindow(w),
		    botGC,x1+1,y1,x2+1,y2);
	}
	else
	{
		XDrawLine(XtDisplay(w),XtWindow(w),
		    topGC,x1,y1-1,x2,y2-1);
		XDrawLine(XtDisplay(w),XtWindow(w),
		    midGC,x1,y1,x2,y2);
		XDrawLine(XtDisplay(w),XtWindow(w),
		    botGC,x1,y1+1,x2,y2+1);
	}
}

static void arrow(SimpleGraphWidget fw,int x1,int y1,int x2,int y2,int gc)
{
	int x;
	int y;
	int size = 11;
	int h = size;
	int w = size;
	unsigned int d;

	GC topGC = fw->manager.top_shadow_GC;
	GC midGC = fw->manager.background_GC;
	GC botGC = fw->manager.bottom_shadow_GC;
	if(gc) midGC = fw->simplegraph.gc[gc % GC_COUNT];

	if(x1 > x2)
	{
		d = XmARROW_LEFT;
		x = (x1 + x2)/2 - size/2;
		y = y1 - size/2;
	}
	else if(x1 < x2)
	{
		d = XmARROW_RIGHT;
		x = (x2 + x1)/2 - size/2;
		y = y1 - size/2;
	}
	else if(y2 >= y1)
	{
		d = XmARROW_DOWN;
		y = (y2 + y1)/2 - size/2;
		x = x1 - size/2;
	}
	else
	{
		d = XmARROW_UP;
		y = (y1 + y2)/2 - size/2;
		x = x1 - size/2;
	}


	_XmDrawArrow(XtDisplay(fw),XtWindow(fw),
	    topGC,
	    botGC,
	    midGC,
	    x,y,w,h,1,d);

}

static int minspace = 10;
*/

static void bezier_arrow(Widget w,GC gc,XPoint* p,int npoints)
{
#define ASIZE 4

	XPoint* f;
	XPoint* t;
	double a,b,l;
	int i;
	XPoint q[3];

	double xx[3] = { 
		-ASIZE, ASIZE , -ASIZE  	};
	double yy[3] = { 
		ASIZE,       0 , -ASIZE 	};


	int n = npoints/2;
	int m = npoints/2+1;
	while(n >= 0 && m < npoints)
	{
		f = &p[n];
		t = &p[m];

		a = t->x - f->x;
		b = t->y - f->y;
		l = sqrt(a*a+b*b);
		if(l > ASIZE) break;

		n--;
		m++;
	}


	/* a = -a; */
	b = -b;

	for(i = 0 ; i < 3; i++)
	{
		double x = (a*xx[i]  + b*yy[i])/l;
		double y = (-b*xx[i] + a*yy[i])/l;

		q[i].x = round(x + f->x);
		q[i].y = round(y + f->y);

	}

	XFillPolygon(XtDisplay(w), XtWindow(w), gc,
	    q,3,Convex,CoordModeOrigin);

}

static void bezier(XPoint* p,int npoints,XPoint* control)
{
	int i;
	for (i = 0; i < npoints; i++)
	{
		double array[4];
		double u, u2, u3, x, y;
		u = (double) i / (double) (npoints - 1);                                        
		u2 = u * u;
		u3 = u2 * u;
		array[0] = -u3 + 3. * u2 - 3. * u + 1.;
		array[1] = 3. * u3 - 6. * u2 + 3. * u;
		array[2] = -3. * u3 + 3. * u2;
		array[3] = u3;
		x = array[0] * control[0].x + array[1] * control[1].x + array[2] * control[2].x + array[3] * control[3].x;
		y = array[0] * control[0].y + array[1] * control[1].y + array[2] * control[2].y + array[3] * control[3].y;

		p[i].x = round(x);
		p[i].y = round(y);
	}

}

static void connect(SimpleGraphWidget w,
XRectangle *from,XRectangle *to,
int fn, int fc,int tn,int tc,int dm,int link_data)
{

	GC gc = w->simplebase.gc;

#if TOP_BOTTOM
	int fx = from->x + ((fn+1)*from->width)/(fc+1);
	int fy = from->y + from->height;
	int tx = to->x + ((tn+1)*to->width)/(tc+1);
	int ty = to->y;
#else
	double fx = from->x + from->width;
	double fy = from->y + from->height/2.0;
	double tx = to->x ;
	double ty = to->y + to->height / 2.0;
#endif


	/* .cap_style = CapRound;  join_style = JoinRound;    */


	XPoint control[4];
	XPoint p[100];

	control[0].x = fx;
	control[0].y = fy;

	control[1].x = (fx + tx)/2;
	control[1].y = fy;

	control[2].x = (fx + tx)/2;
	control[2].y = ty;

	control[3].x = tx;
	control[3].y = ty;

	bezier(p,XtNumber(p),control);

	if(link_data != -1)
	{
		int i;
		gc = w->simplebase.links[link_data].gc;
		for(i = 0; i < w->simplegraph.gc_count; i++)
			if(w->simplegraph.gc[i] == gc)
				break;
		if(i == w->simplegraph.gc_count)
			w->simplegraph.gc[w->simplegraph.gc_count++] = gc;
	}

	XDrawLines(XtDisplay(w),XtWindow(w), 
	    gc,
	    p,XtNumber(p),CoordModeOrigin);


	bezier_arrow((Widget)w,gc,p,XtNumber(p));

}

int close_to(int x,int y, int x1, int y1, int x2, int y2)
{
	if(x1>x2) { 
		int c = x1; 
		x1 = x2; 
		x2 = c; 
	}
	if(y1>y2) { 
		int c = y1; 
		y1 = y2; 
		y2 = c; 
	}

	x1 -= 3; 
	x2 += 3;
	y1 -= 3; 
	y2 += 3;

	return ( x1 <= x && x <= x2 && y1 <= y && y <= y2);
}

static int smallest(int x, int y,XPoint* p, int n)
{
  /* int i; */
	if(n > 1 && close_to(x,y,p[0].x,p[0].y, p[n-1].x,p[n-1].y))
	{
		int m = n/2;
		int a = smallest(x,y,p,m);
		int b = smallest(x,y,p+m,n-m);
		int z = MIN(a,b);
		return MIN(z,n);
	}
	return 32000;
}

static int line_find(SimpleGraphWidget w,XEvent* event,
XRectangle *from,XRectangle *to,
NodeStruct* n1,NodeStruct* n2)
{

#if TOP_BOTTOM
	int fx = from->x + ((fn+1)*from->width)/(fc+1);
	int fy = from->y + from->height;
	int tx = to->x + ((tn+1)*to->width)/(tc+1);
	int ty = to->y;
#else
	double fx = from->x + from->width;
	double fy = from->y + from->height/2.0;
	double tx = to->x ;
	double ty = to->y + to->height / 2.0;
#endif

	int  x = event->xbutton.x;
	int  y = event->xbutton.y;
	/* int i; */
	int value = 32000;

	/* .cap_style = CapRound;  join_style = JoinRound;    */

	if( close_to(x,y,fx,fy,tx,ty))
	{


		XPoint control[4];
		XPoint p[100];

		control[0].x = fx;
		control[0].y = fy;

		control[1].x = (fx + tx)/2;
		control[1].y = fy;

		control[2].x = (fx + tx)/2;
		control[2].y = ty;

		control[3].x = tx;
		control[3].y = ty;

		bezier(p,XtNumber(p),control);

		return smallest(x,y,p,XtNumber(p));
	}

	return value;

}

static void Redisplay (SimpleGraphWidget w, XEvent *event, Region region)
{
	int i, j;
	/* int fkid; */
	int m = 0;

	/* XPoint points[3]; */
	Region rg,clip;

	XEvent ev;

	while(XCheckWindowEvent(XtDisplay(w),XtWindow(w),ExposureMask,&ev))
		XtAddExposureToRegion(&ev,region);

	rg = XCreateRegion();
	clip = XCreateRegion();

	for (i = 0; i < w -> simplebase.count; i++)
	{

		NodeStruct *child = w -> simplebase.nodes + i;
		if((child)->managed)
			XUnionRectWithRegion(&child->r,rg,rg);
	}
	XSubtractRegion(region,rg,clip);

	for(i = 0; i < w->simplegraph.gc_count; i++)
		XSetRegion(XtDisplay(w),w->simplegraph.gc[i],clip);


	for (i = 0; i < w -> simplebase.count; i++)
	{

		NodeStruct *n = w -> simplebase.nodes + i;
		if(!n->managed)
			continue;

		for (j = 0; j < n->kcnt; j++)
		{
			NodeStruct *c = &KIDS(w,n,j);
			int k = 0;

			if(!c->managed)
				continue;

			connect(w,&n->r,&c->r,
			    j,n->kcnt,
			    k,c->pcnt,
			    m++,
			    n->kids[j].link_data
			    );

		}
	}

	XDestroyRegion(clip);
	XDestroyRegion(rg);
	for(i = 0; i < w->simplegraph.gc_count; i++)
		XSetClipMask(XtDisplay(w),w->simplegraph.gc[i],None);

	NodesRedraw((SimpleBaseWidget)w,event,region);
}

static void bezier_find(SimpleGraphWidget w,XEvent *event)
{
	int i, j;
	/* int fkid; */
	int m = 0;
	int min = 32000;
	NodeStruct* m1 = 0 ;
	NodeStruct *m2 = 0;
	LinkCallbackStruct cb;

	for (i = 0; i < w -> simplebase.count; i++)
	{
		NodeStruct *n = w -> simplebase.nodes + i;
		if(!n->managed)
			continue;

		for (j = 0; j < n->kcnt; j++)
		{
			NodeStruct *c = &KIDS(w,n,j);
			if(!c->managed)
				continue;

			m = line_find(w,event,&n->r,&c->r,n,c);
			if(m < min)
			{
				min = m;
				m1 = n;
				m2 = c;

			}
		}
	}

	while(m1 && sb_is_dummy((SimpleBaseWidget)w,m1)) 
		m1 = &PARENTS(w,m1,0);

	while(m2 && sb_is_dummy((SimpleBaseWidget)w,m2)) 
		m2 = &KIDS(w,m2,0);

	cb.reason = 0;
	cb.event  = event;
	cb.data1  = m1?m1->user_data:0;
	cb.data2  = m1?m2->user_data:0;
	XtCallCallbacks((Widget)w, XtNlinkCallback, (XtPointer)&cb);
}

static void Layout(Widget w,long *maxWidth,long *maxHeight)
{
	SimpleGraphWidget gw = (SimpleGraphWidget)w;
	/* XtGeometryResult result; */
	/* Dimension replyWidth = 0, replyHeight = 0; */
	*maxWidth = 1;
	*maxHeight = 1;
	sb_clear_dummy_nodes((SimpleBaseWidget)gw);
	compute_positions(gw,1);
	set_positions(gw, maxWidth,maxHeight);
}


static int calc_level(SimpleGraphWidget w,NodeStruct *n)
{

	int i;
	int lvl = 0;

	if(n->misc[VISIT]) return -1;

	n->misc[VISIT] = True;

	for(i=0;i<n->pcnt;i++)
	{
		NodeStruct *p = &PARENTS(w,n,i);
		if(MANAGED(p))
		{
			int lev = calc_level(w,p) + 1;
			lvl = MAX(lvl,lev);
		}
	}

	n->misc[LEVEL] = lvl;
	n->misc[VISIT] = False;

	return lvl;

}

static void set_arc(SimpleGraphWidget w,NodeStruct *n,int arc)
{

	int i;

	if(n->misc[VISIT]) return;
	n->misc[VISIT] = True;

	n->misc[ARC] = arc;

	for(i=0;i<n->pcnt;i++)
	{
		NodeStruct *p = &PARENTS(w,n,i);
		if(MANAGED(p))
			set_arc(w,p,arc);
	}

	n->misc[VISIT] = False;

}

static int calc_arc(SimpleGraphWidget w,NodeStruct *n)
{

	int i;
	int a = n->misc[ARC];

	if(n->misc[VISIT]) return 0;
	n->misc[VISIT] = True;

	if(n->pcnt)
	{
		for(i=0;i<n->pcnt;i++)
		{
			NodeStruct *p = &PARENTS(w,n,i);
			if(MANAGED(p))
			{
				int b = calc_arc(w,p);
				a = MAX(a,b);
			}
		}
		set_arc(w,n,a);
	}

	n->misc[VISIT] = False;
	return a;
}

static SimpleGraphWidget sort;

static int by_arc(const void* n1,const void* n2)
{
	NodeStruct *w1 = sort->simplebase.nodes + *(int*)n1;
	NodeStruct *w2 = sort->simplebase.nodes + *(int*)n2;
	if(w1->misc[LEVEL] != w2->misc[LEVEL])
		return w1->misc[LEVEL] - w2->misc[LEVEL];

	return w1->misc[ARC] - w2->misc[ARC];

}

static int by_x(const void *n1,const void *n2)
{
	NodeStruct *w1 = sort->simplebase.nodes + *(int*)n1;
	NodeStruct *w2 = sort->simplebase.nodes + *(int*)n2;

	if(w1->misc[LEVEL] != w2->misc[LEVEL])
		return w1->misc[LEVEL] - w2->misc[LEVEL];

	return w1->TMPX - w2->TMPX;

}


static int by_level(const void *n1,const void *n2)
{
	NodeStruct *w1 = sort->simplebase.nodes + *(int*)n1;
	NodeStruct *w2 = sort->simplebase.nodes + *(int*)n2;

	return w1->misc[LEVEL] - w2->misc[LEVEL];

}


static int no_parents(SimpleGraphWidget w,NodeStruct *n)
{
	int i;

	for(i=0;i<n->pcnt;i++) {
		NodeStruct *p = &PARENTS(w,n,i);
		if(MANAGED(p)) return FALSE;
	}

	return TRUE;

}

static int no_kidss(SimpleGraphWidget w,NodeStruct *n)
{
	int i;
	for(i=0;i<n->kcnt;i++) {
		NodeStruct *p = &KIDS(w,n,i);
		if(MANAGED(p)) return FALSE;
	}

	return TRUE;

}


static void calc_level_pass2(SimpleGraphWidget w,NodeStruct *n)
{
	int i;
	int lvl1 = 12000;

	for(i=0;i<n->kcnt;i++)
	{
		NodeStruct *p = &KIDS(w,n,i);
		if(MANAGED(p))
			lvl1 = MIN(lvl1,p->misc[LEVEL]);
	}
	n->misc[LEVEL] = lvl1-1;
}

static void calc_y(SimpleGraphWidget gw,int *nodes,int *levels,
int *positions,
int max_in_a_level,int no_levels,int num_nodes,int v_dist)
{
	int i;
	int max = max_in_a_level * v_dist;

	for(i=0;i<no_levels;i++)
		positions[i] = ((max / levels[i])-
		    (max / max_in_a_level))/2;

	for(i=0;i<num_nodes;i++)
	{
		NodeStruct *n = gw->simplebase.nodes + nodes[i];
		n->TMPX = positions[n->misc[LEVEL]];
		positions[n->misc[LEVEL]] += (max / levels[n->misc[LEVEL]]);
	}

}

static int add_node(SimpleGraphWidget w,int n,int m,int lvl)
{
	int x = sb_insert_dummy_node((SimpleBaseWidget)w,n,m);
	INDEX_TO_NODE(w,x)->misc[LEVEL] = lvl;
	return x;
}

static int add_dummies(SimpleGraphWidget w,NodeStruct *n)
{
	int i;
	int more = 0;
	int lvl = n->misc[LEVEL];

	if(n->misc[VISIT]) return 0;
	n->misc[VISIT] = True;

	for(i=0;i<n->kcnt;i++)
	{
		NodeStruct *p = &KIDS(w,n,i);

		if(MANAGED(p))
		{
			int klvl = p->misc[LEVEL];
			int d = klvl - lvl;
			int l = lvl;
			int a = NODE_TO_INDEX(w,n);
			int b = NODE_TO_INDEX(w,p);

			int x = NODE_TO_INDEX(w,p);
			int y = NODE_TO_INDEX(w,n);

			while(d-- > 1)
			{
				a = add_node(w,a,b,++l);
				more = 1;
			}

			/* note: creating dummnies may change pointers*/
			p = INDEX_TO_NODE(w,x);
			n = INDEX_TO_NODE(w,y);

		}


		more = add_dummies(w,p) || more;
	}
	n->misc[VISIT] = False;
	return more;

}

static int add_dummy_nodes(SimpleGraphWidget gw)
{
	int i;
	int n = gw->simplebase.count;
	int more = 0;

	for(i=0; i < n; i++)
	{
		NodeStruct *w = gw->simplebase.nodes + i;
		if(MANAGED(w))
			more = add_dummies(gw,w) || more;
	}

	return more;
}

static void compute_positions(SimpleGraphWidget gw,int dummy)
{
	int i;
	int n = gw->simplebase.count;
	int *levels;
	int *positions;
	int count;
	int max_in_a_level = 0;
	int max_level = 0;
	int arc=0;
	int minx,miny;

	int no_levels = 0;
	int num_nodes = 0;
	int *nodes;
	int *widths;
	int *heights;
	int *kids;
	int a;
	int move_it;
	int more;
	int v_dist = 10;
	int h_dist = 10;
	int chg = TRUE;
	NodeStruct* focus = (gw->simplebase.focus>=0)? (gw->simplebase.nodes +
	    gw->simplebase.focus) : 0;

	sort = gw;


	num_nodes = 0;
	for(i=0;i<n;i++)
	{
		NodeStruct *w = gw->simplebase.nodes + i;
		if(MANAGED(w)) num_nodes++;
		w->misc[LEVEL] = w->misc[ARC] = -1;
		w->misc[VISIT] = False;
	}

	if(!num_nodes) return;

	if(focus == 0) focus = gw->simplebase.nodes;


	levels = (int*)XtCalloc(num_nodes,sizeof(int));
	positions = (int*)XtCalloc(num_nodes,sizeof(int));
	nodes = (int*)XtCalloc(num_nodes,sizeof(int));
	widths = (int*)XtCalloc(num_nodes,sizeof(int));
	heights = (int*)XtCalloc(num_nodes,sizeof(int));
	kids = (int*)XtCalloc(num_nodes,sizeof(int));

	num_nodes=0;
	for(i=0;i<n;i++)
	{
		NodeStruct *w = gw->simplebase.nodes + i;
		if(MANAGED(w)) nodes[num_nodes++] = i;
		w->misc[LEVEL] = 0;
	}
	arc = 1;

	for(i=0;i<num_nodes;i++)
	{
		NodeStruct *w = gw->simplebase.nodes + nodes[i];
		calc_level(gw,w);
		w->misc[ARC] = arc++;

		H_DIST = MAX(H_DIST,w->r.WIDTH);
		V_DIST = MAX(V_DIST,w->r.HEIGHT);
	}

	H_DIST += gw->simplegraph.H_MIN_SPACE;
	V_DIST += gw->simplegraph.V_MIN_SPACE;

	for(i=0;i<num_nodes;i++)
	{
		NodeStruct *w = gw->simplebase.nodes + nodes[i];
		if (no_parents(gw,w) && !no_kidss(gw,w))
			calc_level_pass2(gw,w);
	}

#if 1
	if(dummy && add_dummy_nodes(gw))
	{
		XtFree((XtPointer)levels);
		XtFree((XtPointer)positions);
		XtFree((XtPointer)nodes);
		XtFree((XtPointer)widths);
		XtFree((XtPointer)heights);
		XtFree((XtPointer)kids);
		compute_positions(gw,0);
		return;
	}
#endif

	qsort(nodes,num_nodes,sizeof(int),by_level);

	for(i=0;i<num_nodes;i++)
	{
		NodeStruct *w = gw->simplebase.nodes + nodes[i];
		levels[w->misc[LEVEL]]++;

		if(levels[w->misc[LEVEL]] > max_in_a_level)
		{
			max_in_a_level = levels[w->misc[LEVEL]];
			max_level = w->misc[LEVEL];
		}

		no_levels = MAX(no_levels,w->misc[LEVEL]);

		widths[w->misc[LEVEL]] = MAX(widths[w->misc[LEVEL]],w->r.WIDTH);
		heights[w->misc[LEVEL]] = MAX(heights[w->misc[LEVEL]],w->r.HEIGHT);
		kids[w->misc[LEVEL]] = MAX(kids[w->misc[LEVEL]],w->kcnt);

	}
	no_levels++;


	a = 0;
	for( i = 0 ;i<no_levels;i++)
	{
		int b = heights[i] + gw->simplegraph.V_MIN_SPACE;


		b += kids[i] * 2 ; /* +5 pixels per node with max kids in level */
		/* printf("level %d = %d %d\n",i,a,heights[i]); */
		heights[i] = a;
		a += b;
		widths[i] += gw->simplegraph.H_MIN_SPACE;
		/* printf("level %d = %d\n",i,a); */
	}

	for(i=0;i<num_nodes;i++)
	{
		NodeStruct *w = gw->simplebase.nodes + nodes[i];
		w->TMPY = heights[w->misc[LEVEL]];
		/* printf("node y = %d %d\n",w->misc[LEVEL],w->TMPY); */
	}


	for(a=0;a<2;a++)
	{
		for(i=0;i<num_nodes;i++) {
			NodeStruct *w = gw->simplebase.nodes + nodes[i];
			if(no_kidss(gw,w)) calc_arc(gw,w);
		}
		for(i=0;i<num_nodes;i++){
			NodeStruct *w = gw->simplebase.nodes + nodes[i];
			if(no_kidss(gw,w)) set_arc(gw,w,w->misc[ARC]);
		}
	}


	qsort(nodes,num_nodes,sizeof(int),by_arc);

	for(a=0;a<no_levels;a++)
		if(levels[a]==0) levels[a]=1;


	calc_y(gw,nodes,levels,positions,max_in_a_level,no_levels,num_nodes,H_DIST);

	move_it = 0;

	more = 1;
	qsort(nodes,num_nodes,sizeof(int),by_x);
	while(more--)
	{

		count = num_nodes;
		while(count--)
		{
			for(i=0;i<num_nodes;i++)
			{
				int j;
				int n = 0;
				int x = 0;
				NodeStruct *w = gw->simplebase.nodes + nodes[i];
				if( (max_level != w->misc[LEVEL]) ^ move_it)
				{


					for(j=0;j<w->pcnt;j++)
					{
						NodeStruct *p = &PARENTS(gw,w,j);
						if(MANAGED(p) && p != focus)
						{
							x += p->TMPX;
							n++;
						}
					}


					for(j=0;j<w->kcnt;j++)
					{
						NodeStruct *p = &KIDS(gw,w,j);
						if(MANAGED(p) && p != focus)
						{
							x += p->TMPX;
							n++;
						}
					}
					w->TMPX = n?x/n:x;
				}
			}



			qsort(nodes,num_nodes,sizeof(int),by_x);
			move_it = !move_it;

			chg = TRUE;
			a = 10000;
			while(chg && a--)
			{
				chg = FALSE;
				for(i=1;i<num_nodes;i++) {
					NodeStruct *w = gw->simplebase.nodes + nodes[i];
					NodeStruct *z = gw->simplebase.nodes + nodes[i-1];
					if(z->misc[LEVEL] == w->misc[LEVEL])
						if(w->TMPX-z->TMPX< widths[w->misc[LEVEL]])
						{
							if(z != focus)
								z->TMPX -= widths[w->misc[LEVEL]]/2;
							if(w != focus)
								w->TMPX += widths[w->misc[LEVEL]]/2;
							chg = TRUE;
						}
				}
			}
		}
	}


	miny = (gw->simplebase.nodes + nodes[0])->tmpy;
	minx = (gw->simplebase.nodes + nodes[0])->tmpx;

	for(i=1;i<num_nodes;i++)
	{
		NodeStruct *w = gw->simplebase.nodes + nodes[i];
		if(w->tmpx<minx) minx = w->tmpx;
		if(w->tmpy<miny) miny = w->tmpy;
	}

	minx -= 20;
	miny -= 20;

	for(i=0;i<num_nodes;i++)
	{
		NodeStruct *w = gw->simplebase.nodes + nodes[i];
		w->tmpx -= minx;
		w->tmpy -= miny;
	}


	XtFree((XtPointer)levels);
	XtFree((XtPointer)positions);
	XtFree((XtPointer)nodes);
	XtFree((XtPointer)widths);
	XtFree((XtPointer)heights);
	XtFree((XtPointer)kids);


}



static SimpleGraphWidget sort_widget = 0;

static int left_to_right(const void *a,const void *b)
{
	NodeStruct *na = sort_widget->simplebase.nodes + ((Link*)a)->node;
	NodeStruct *nb = sort_widget->simplebase.nodes + ((Link*)b)->node;
	return na->r.x - nb->r.x;
}


static void set_positions(SimpleGraphWidget gw, long *maxWidth,long *maxHeight)
{
	int i;

	for(i=0;i<gw->simplebase.count;i++)
	{
		NodeStruct *w = gw->simplebase.nodes + i;
		if(w->managed )
		{
			{
			w->r.x = w->tmpx;
			w->r.y = w->tmpy;
			*maxWidth = MAX(*maxWidth,w->tmpx + w->r.width + gw->simplegraph.h_min_space);
			*maxHeight = MAX(*maxHeight,w->tmpy + w->r.height + gw->simplegraph.v_min_space);
#if 0
			if(w->is_group)
			{
				int j;
				int	x = w->r.x;
				int	y = w->tmpy;
				for(j=0;j<gw->simplebase.count;j++)
				{
					NodeStruct *v = gw->simplebase.nodes + j;
					if(v->managed && v->group == i)
					{
						v->r.x = x;
						v->r.y = y;
						y += v->r.height;
					}
				}
			}
#endif
		}
	}
}

sort_widget = gw;
for(i=0;i<gw->simplebase.count;i++)
{
	NodeStruct *w = gw->simplebase.nodes + i;
	qsort(w->parents,w->pcnt,sizeof(Link),left_to_right);
	qsort(w->kids,w->kcnt,sizeof(Link),left_to_right);
}
}

Widget CreateGraph(par,nam,al,ac)
Widget par;
char *nam;
ArgList al;
int ac;

{
	return XtCreateWidget(nam,simplegraphWidgetClass,par,al,ac);
}

static void Print (SimpleGraphWidget w, FILE *f)
{
}

