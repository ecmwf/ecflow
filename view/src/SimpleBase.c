/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #10 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2012 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ConstrainP.h>
#include <Xm/XmP.h>
#include <Xm/DrawingAP.h>
#include <Xm/ExtObjectP.h>
#include <Xm/ScrollBar.h>
#include "SimpleBase.h"
#include "SimpleBaseP.h"

static void             Initialize();
static void             Destroy();
static void             Realize();
static Boolean          SetValues();


#ifdef linux
/* putting this into comments makes ctrl-left button for collector
   disappear !! */
static char defaultTranslations[] = "\
 Shift<Btn5Down>: increment(1)\n       Shift<Btn4Down>: increment(-1)  \n\
      <Btn5Down>: increment(10)\n           <Btn4Down>: increment(-10) \n\
<BtnDown>:DrawingAreaInput()\n              <BtnUp>:DrawingAreaInput()\n\
<Key>osfActivate:DrawingAreaInput()\n\
~s ~m ~a <Key>Return:DrawingAreaInput()\n\
~s ~m ~a <Key>space:DrawingAreaInput()\n\
<Key>F1:DrawingAreaInput()\n\
<Key>F2:DrawingAreaInput()\n";
#else

#define defaultTranslations XmInheritTranslations

#endif

static void xincrement();
static XtActionsRec actionsList[] = {
    { "increment",(XtActionProc) xincrement},
};

static XtResource resources[] = {
	{XtNblinkColor, XtCBlinkColor, XtRPixel, sizeof (Pixel),
	XtOffset(SimpleBaseWidget, simplebase.blink_color), XtRString,"Black"},

	{XtNselected, XtCSelected, XtRInt, sizeof (int),
	XtOffset(SimpleBaseWidget, simplebase.selected), XtRImmediate,(XtPointer)-1},

	{"blinkRate", "BlinkRate", XtRInt, sizeof (int),
	XtOffset(SimpleBaseWidget, simplebase.timeout), XtRImmediate,(XtPointer)500},

	{XtNgetpsCallback,XtCCallback,XtRCallback,sizeof(caddr_t),
	XtOffset (SimpleBaseWidget, simplebase.getps),XtRCallback,NULL},

	{XtNlinkCallback,XtCCallback,XtRCallback,sizeof(caddr_t),
	XtOffset (SimpleBaseWidget, simplebase.link),XtRCallback,NULL},

	{XtNpsHeader,XtCPsHeader,XtRString,sizeof(String),
	XtOffset (SimpleBaseWidget, simplebase.header),XtRImmediate,NULL},
};


SimpleBaseClassRec simplebaseClassRec = {
	{
	/* core_class fields  */
	(WidgetClass) &xmDrawingAreaClassRec,/* superclass         */
	"SimpleBase",                           /* class_name         */
	sizeof(SimpleBaseRec),                /* widget_size        */
	NULL,                             /* class_init         */
	NULL,                             /* class_part_init    */
	FALSE,                            /* class_inited       */
	Initialize,                       /* initialize         */
	NULL,                             /* initialize_hook    */
	Realize,      /* realize            */
	actionsList,                      /* actions            */
	XtNumber(actionsList),            /* num_actions        */
	resources,                        /* resources          */
	XtNumber(resources),              /* num_resources      */
	NULLQUARK,                        /* xrm_class          */
	TRUE,                             /* compress_motion    */
	XtExposeCompressMaximal,         /* compress_exposure  */
	TRUE,                             /* compress_enterleave*/
	TRUE,                             /* visible_interest   */
	Destroy,                          /* destroy            */
	NULL,                             /* resize             */
	NULL,                             /* expose             */
	SetValues,                        /* set_values         */
	NULL,                             /* set_values_hook    */
	XtInheritSetValuesAlmost,         /* set_values_almost  */
	NULL,                             /* get_values_hook    */
	NULL,                             /* accept_focus       */
	XtVersion,                        /* version            */
	NULL,                             /* callback_private   */
	defaultTranslations,              /* tm_table           */
	NULL,                             /* query_geometry     */
	XtInheritDisplayAccelerator,      /* display_accelerator*/
	NULL,                             /* extension          */
	},
	{
	/* composite_class fields */
	NULL,                            /* geometry_manager    */
	NULL,                            /* change_managed      */
	XtInheritInsertChild,            /* insert_child        */
	XtInheritDeleteChild,            /* delete_child        */
	NULL,                            /* extension           */
	},
	{ 
	/* constraint_class fields */
	NULL,                             /* subresources        */
	0,                                /* subresource_count   */
	0,                                /* constraint_size     */
	NULL,                             /* initialize          */
	NULL,                             /* destroy             */
	NULL,                             /* set_values          */
	NULL,                             /* extension           */
	},
	{
	XtInheritTranslations,   /* default translations */
	NULL,                    /* syn_resources          */
	0,               /* num_syn_resources      */
	NULL,                        /* syn_cont_resources     */
	0,                           /* num_syn_cont_resources */
	XmInheritParentProcess,  /* parent_process */
	NULL,                        /* extension              */
	},
	{
	NULL,
	},
	{
	/* SimpleBase class fields */
	NULL,
	},
};


#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif

WidgetClass simplebaseWidgetClass = (WidgetClass) &simplebaseClassRec;

//static void find_visible_part(Widget w,Position *x,Position *y,
//	Dimension* width, Dimension* height)
//{
//    Position root_x,root_y;
//    Widget   p = w;
//
//    *width  = w->core.width;
//    *height = w->core.height;
//    XtTranslateCoords(w,0,0,&root_x,&root_y);
//
//    *x = 0;
//    *y = 0;
//
//    while((p = XtParent(p)))
//    {
//        Position  rx,ry;
//        Dimension w,h;
//
//        /*
//           make all computations in the root's coordinate system
//        */
//
//        XtTranslateCoords(p,0,0,&rx,&ry);
//
//        w = p->core.width;
//        h = p->core.height;
//
//        /*
//            use the smallest rectangle
//        */
//
//        if(w < *width)  *width  = w;
//        if(h < *height) *height = h;
//
//        if(rx>root_x) root_x = rx;
//        if(ry>root_y) root_y = ry;
//
//        /* stop when reach a shell, don't go to top level shell */
//        if(XtIsShell(p)) break;
//    }
//
//    /* Back to the widget's coordinate system */
//
//    XtTranslateCoords(w,0,0,x,y);
//    *x = root_x - *x;
//    *y = root_y - *y;
//}


/*-----------------------------------------------------------------*/
/* Find which gadget was called                                    */
/*-----------------------------------------------------------------*/

static void button_click(w,cd,event,continue_dispatch)
SimpleBaseWidget    w;
XtPointer *cd;
XEvent    *event;
Boolean   *continue_dispatch;
{
	int i;
	Position    x,y;
	XmDrawingAreaCallbackStruct cb;

	x = event->xbutton.x;
	y = event->xbutton.y;
	// printf("bclick\n");

	for (i = 0; i < w -> composite.num_children; i++)
	{
		Widget child = w -> composite.children[i];

		if(  XtIsManaged(child) &&
		    (x >= child->core.x && x <= child->core.x + child->core.width)
		    && (y >= child->core.y && y <= child->core.y + child->core.height))
		{
			cb.reason = -1;
			cb.event= event;
			cb.window = (Window)child;
			XtCallCallbacks((Widget)w,XmNinputCallback,(XtPointer)&cb);
		}
	}

}


/*-------------------------------------------------------------------*/
/* Blink current gadget                                              */
/*-------------------------------------------------------------------*/

#if 0
static void time_out(SimpleBaseWidget w,XtIntervalId id)
{
	if( w->simplebase.selected >= 0 && w->simplebase.selected < w->simplebase.count) {
		Node *n = w->simplebase.nodes +  w->simplebase.selected;

		if(XtIsRealized((Widget) w) && n->managed)
		{
			/* XFillRectangle */

			XDrawRectangle(XtDisplay(w),XtWindow(w),
			    w->simplebase.blink_gc,
			    n->r.x+1,
			    n->r.y+1,
			    n->r.width-2,
			    n->r.height-2);
		}
	}
	w->simplebase.timeout_id = XtAppAddTimeOut(
	    XtWidgetToApplicationContext((Widget)w),
	    w->simplebase.timeout,
	    (XtTimerCallbackProc)time_out,
	    (XtPointer)w);
}
#endif

static void clip_input_callback(clip,widget,cb)
Widget clip;
Widget widget;
XtPointer cb;
{
	XtCallCallbacks(widget,XmNinputCallback,cb);
}

static XtCallbackRec clipcb[] = {
	{ (XtCallbackProc)clip_input_callback, NULL, },
	{ NULL,NULL,},
};

/*----------------------------------------------------*/
/* Init : create blink gc                             */
/*----------------------------------------------------*/

static void Realize(SimpleBaseWidget w,XtValueMask *value_mask,
XSetWindowAttributes *attributes)
{
	Widget clip,scroll,ww;
	WidgetClass class = XtClass(w);

	while(class != (WidgetClass)simplebaseWidgetClass)
		class = class->core_class.superclass;
	class = class->core_class.superclass;
    	(class->core_class.realize) ((Widget) w, value_mask, attributes);

	ww = (Widget)w;
	while(ww){
  	          if((clip = XtParent(ww)))
 		         if((scroll = XtParent(clip)))
				if(XmIsScrolledWindow(scroll))
				{
					clipcb[0].closure = (XtPointer)w;
					XtAddCallbacks(clip,XmNinputCallback,clipcb);
				}
		ww = clip;
	}
}

static void Initialize(request, new)
SimpleBaseWidget request, new;
{
	XGCValues values;
	XtGCMask  valueMask;

	/*
   * Make sure the widget's width and height are 
   * greater than zero.
   */
	if (request->core.width <= 0)
		new->core.width = 5;
	if (request->core.height <= 0)
		new->core.height = 5;

	new->simplebase.gc = XtGetGC((Widget)new,0,0);

	valueMask = GCLineWidth ;
	    /* GCForeground | GCBackground | GCFunction | | GCGraphicsExposures; */

	values.foreground  = new->core.background_pixel^new->simplebase.blink_color;
	values.background  = values.foreground;
	values.function           = GXxor;
	values.graphics_exposures = False;
	values.line_width         = 2;

	new->simplebase.blink_gc = XtGetGC((Widget)new,valueMask,&values);

	XtAddEventHandler((Widget)new,ButtonPressMask,
	    False,(XtEventHandler)button_click,(XtPointer)new);
	XtAddEventHandler((Widget)new,KeyPressMask,
	    False,(XtEventHandler)button_click,(XtPointer)new);

	/* time_out(new,NULL); */

	new->simplebase.focus = -1;
	new->simplebase.max   = 0;
	new->simplebase.link_max  = 0;
	new->simplebase.count = 0;
	new->simplebase.link_count = 0;
	new->simplebase.nodes = NULL;
	new->simplebase.links = NULL;
	new->simplebase.work  = 0;
}


/*----------------------------------------------------------*/
/*----------------------------------------------------------*/

static void Destroy(SimpleBaseWidget w)
{
	Widget clip,scroll,ww;

	printf("Destroy(SimpleBaseWidget w)\n");

	NodeReset((Widget)w);

	XtRemoveEventHandler((Widget)w,ButtonPressMask,
	    False,(XtEventHandler)button_click,(XtPointer)w);
	XtRemoveEventHandler((Widget)w,KeyPressMask,
	    False,(XtEventHandler)button_click,(XtPointer)w);

	XtReleaseGC((Widget)w,w->simplebase.blink_gc);
	XtReleaseGC((Widget)w,w->simplebase.gc);
	/* XtRemoveTimeOut(w->simplebase.timeout_id); */

	ww = (Widget)w;
	while(ww) {
	  if((clip = XtParent(ww)))
	    if((scroll = XtParent(clip)))
			if(XmIsScrolledWindow(scroll))
			{
				clipcb[0].closure = (XtPointer)w;
				XtRemoveCallbacks(clip,XmNinputCallback,clipcb);
			}
			ww = clip;
		}

	XtFree((char*)w->simplebase.nodes);
	XtFree((char*)w->simplebase.links);
}

static void selection(SimpleBaseWidget w)
{
	if( w->simplebase.selected >= 0 && w->simplebase.selected < w->simplebase.count) {
		Node *n = w->simplebase.nodes +  w->simplebase.selected;
		if(n->managed)
		{
			XDrawRectangle(XtDisplay(w),XtWindow(w),
				w->simplebase.blink_gc,
				n->r.x-1,
				n->r.y-1,
				n->r.width+2,
				n->r.height+2);
		}
	}
}

static void clear(SimpleBaseWidget w)
{
	if( w->simplebase.selected >= 0 && w->simplebase.selected < w->simplebase.count) {
		Node *n = w->simplebase.nodes +  w->simplebase.selected;
		if(n->managed)
		{
			XClearArea(XtDisplay(w),XtWindow(w),
				n->r.x-3,
				n->r.y-3,
				n->r.width+5,
				n->r.height+5,True);
		}
	}
}

void NodesRedraw(SimpleBaseWidget w,XEvent *event, Region region)
{
	int i;
	/* int d = 0,u = 0; */

	/* Position x,y; */
	/* Dimension a,h; */

	/* find_visible_part(w,&x,&y,&a,&h); */
	/* printf("find_visible_part : %d %d %d %d \n",x,y,a,h); */

	for(i = 0; i < w->simplebase.count;i++)
	{
		Node *n = w->simplebase.nodes + i;
		if(n->managed)
				if(XRectInRegion(region, n->r.x, n->r.y, n->r.width, n->r.height))
					n->draw((Widget)w,&n->r,n->user_data);
	}

	/* printf("NodesRedraw %s: draw %d, skip %d\n",XtName(w),d,u); */

	/* XSetRegion(XtDisplay(w),w->simplebase.blink_gc,region); */
	selection(w);
	/* XSetClipMask(XtDisplay(w),w->simplebase.blink_gc,None); */

#if 0
	if( w->simplebase.selected >= 0 && w->simplebase.selected < w->simplebase.count) {
		Node *n = w->simplebase.nodes +  w->simplebase.selected;
		if(n->managed)
			if(XRectInRegion(region, n->r.x-1, n->r.y-1, n->r.width+2, n->r.height+2))
			{
				XDrawRectangle(XtDisplay(w),XtWindow(w),
					w->simplebase.blink_gc,
					n->r.x-1,
					n->r.y-1,
					n->r.width+2,
					n->r.height+2);
			}
		}
#endif
}

static int new_link_data(SimpleBaseWidget w)
{
	LinkData *d;
	if(w->simplebase.link_count >= w->simplebase.link_max)
	{
		w->simplebase.link_max += w->simplebase.link_max/2 + 128;
		w->simplebase.links = 
		    (LinkData*)XtRealloc((XtPointer)w->simplebase.links,
		    w->simplebase.link_max*sizeof(LinkData));
		memset(w->simplebase.links + w->simplebase.link_count, 0,
		(w->simplebase.link_max - w->simplebase.link_count)*sizeof(LinkData));
	}

	d            = &w->simplebase.links[w->simplebase.link_count];
	d->gc        = w->simplebase.gc;
	d->user_data = 0;

	return w->simplebase.link_count++;
}

int NodeCreate(Widget _w,DrawProc draw,SizeProc size,void *data)
{
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	Node *n;
	if(w->simplebase.count >= w->simplebase.max)
	{
		w->simplebase.max += w->simplebase.max/2 + 128;
		w->simplebase.nodes = 
		    (Node*)XtRealloc((XtPointer)w->simplebase.nodes,
		    w->simplebase.max*sizeof(Node));
		memset(w->simplebase.nodes + w->simplebase.count, 0,
		    (w->simplebase.max - w->simplebase.count)*sizeof(Node));
	}

	n = &w->simplebase.nodes[w->simplebase.count];
#if 0
	n-> group    = -1;
#endif
	n->r.width  = 30;
	n->r.height = 20;
	n->draw = draw;
	n->size = size;
	n->user_data = data;
	/* n->size(_w,&n->r,data); */

	return w->simplebase.count++;
}


static Boolean manage_proc(SimpleBaseWidget w)
{
	WidgetClass class = XtClass(w);
	WidgetClass sclass = simplebaseClassRec.core_class.superclass;

	long width  = w->core.width;
	long height = w->core.height;
	Position pwidth; 
	Position pheight;

	w->simplebase.work = 0;

	while(class != sclass)
	{
		SimpleBaseClassRec *bc = (SimpleBaseClassRec*)class;

		if(bc->simplebase_class.layout)
			(*(bc->simplebase_class.layout))((Widget)w,&width,&height);

		class = class->core_class.superclass;

	}

	pwidth  = width; if(pwidth != width)   pwidth  = 0x7f00;
	pheight = height;if(pheight != height) pheight = 0x7f00;

	if(pwidth != w->core.width || pheight != w->core.height)
	{
		Dimension           replyWidth = 0, replyHeight = 0;

		XtGeometryResult result = XtMakeResizeRequest((Widget)w,
		    pwidth,pheight,
		    &replyWidth, &replyHeight);

		if (result == XtGeometryAlmost)
			XtMakeResizeRequest ((Widget)w, replyWidth, replyHeight,
			    NULL, NULL);
	}

	if(XtIsRealized((Widget) w))
	{
		/* printf("XClearArea 1 %s\n",XtName(w)); */
		XClearArea(XtDisplay(w),XtWindow(w),0,0,0,0,True);
	}

	return True;

}

static void enqueue_manage_proc(SimpleBaseWidget w)
{
	if(w->simplebase.work == 0)
		w->simplebase.work = XtAppAddWorkProc(
		    XtWidgetToApplicationContext((Widget)w),
		    (XtWorkProc)manage_proc,(XtPointer)w);
}

void NodeChanged(Widget _w,int node)
{
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	if( w == 0 || node < 0 || node >= w->simplebase.count) return;

	if(XtIsRealized((Widget)w))
	{
		Node *p = w->simplebase.nodes + node;
		if(p->managed) 
		{
			/* printf("XClearArea 2 %s\n",XtName(w)); */
			XClearArea(XtDisplay(w),XtWindow(w),
				p->r.x,p->r.y,
				p->r.width,p->r.height,True);
		}
	}
}

void NodeSetFocus(Widget _w,int node)
{
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	if( w == 0 || node >= w->simplebase.count) return;
	w->simplebase.focus = node;
	enqueue_manage_proc(w);
}

void *NodeFind(Widget _w,XEvent *ev)
{
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	int i;
	for(i = 0; i < w->simplebase.count;i++)
	{
		Node *n = w->simplebase.nodes + i;
		if(n->managed)
			if(ev->xbutton.x >= n->r.x && ev->xbutton.x <= n->r.x + n->r.width &&
			    ev->xbutton.y >= n->r.y && ev->xbutton.y <= n->r.y + n->r.height)
				return n->user_data;
	}
	return NULL;
}

void SimpleBaseShow(Widget _w,XRectangle* r,XEvent* ev)
{
  /* SimpleBaseWidget sw = (SimpleBaseWidget)_w; */

	Widget v_scroll,h_scroll;
	Position        x_parent,y_parent;
	Position        x_clip,y_clip;
	Position        x_event,y_event;
	Dimension       h_clip,w_clip;
	Position        dv=0,dh=0;
	int min,max;
	int     v_val,v_size,v_inc,v_page;
	int     h_val,h_size,h_inc,h_page;
	Widget clip = NULL;
	Widget scroll_window = NULL;
	Arg al[5];
	int ac;
	/* Node *w; */
        Position x_node ,y_node;
	Dimension h_node,w_node;

	Position x,y;

	Widget ww = _w;
	while(ww) {
		if(!(clip            = XtParent(ww)))    return;
		if(!(scroll_window   = XtParent(clip)))  return;
		if(XmIsScrolledWindow(scroll_window))    break;
		if(!(ww = XtParent(ww))) return;

	}

    x_node = r->x;
    y_node = r->y;
    h_node = r->height;
    w_node = r->width;

	ac = 0;
	XtSetArg(al[ac],XmNhorizontalScrollBar, &h_scroll );ac++;
	XtSetArg(al[ac],XmNverticalScrollBar, &v_scroll );ac++;
	XtGetValues(scroll_window,al,ac);

	ac = 0;
	XtSetArg(al[ac],XmNx,&x_parent);ac++;
	XtSetArg(al[ac],XmNy,&y_parent);ac++;
	XtGetValues(_w,al,ac);

	ac = 0;
	XtSetArg(al[ac],XmNclipWindow,&clip);ac++;
	XtGetValues(scroll_window,al,ac);

	ac = 0;
	XtSetArg(al[ac],XmNheight,&h_clip);ac++;
	XtSetArg(al[ac],XmNwidth,&w_clip);ac++;
	XtGetValues(clip,al,ac);


	XtTranslateCoords(_w,x_node,y_node,        &x_node,&y_node);
	XtTranslateCoords(clip,0,0,&x_clip,&y_clip);
	if(ev) XtTranslateCoords(_w,ev->xbutton.x,ev->xbutton.y, &x_event,&y_event);


	x = x_node - x_clip;
	y = y_node - y_clip;


	if( y < 0 || y + h_node > h_clip || ev)
	{
		if(ev)
			dv = (y + h_node / 2)  - (y_event - y_clip);
		else
			dv = (y + h_node / 2)  - h_clip / 2;

		ac = 0;
		XtSetArg(al[ac],XmNminimum,&min);ac++;
		XtSetArg(al[ac],XmNmaximum,&max);ac++;
		XtGetValues(v_scroll,al,ac);

		XmScrollBarGetValues(v_scroll,&v_val,&v_size,&v_inc,&v_page);

		max -= v_size;

		if( dv + v_val > max ) dv = max - v_val;
		if( dv + v_val < min ) dv = min - v_val;


	}
	if( x < 0 || x + w_node > w_clip || ev)
	{
		if(ev)
			dh = (x + w_node / 2)  - (x_event - x_clip);
		else
			dh = (x + w_node / 2)  - w_clip / 2;

		ac = 0;
		XtSetArg(al[ac],XmNminimum,&min);ac++;
		XtSetArg(al[ac],XmNmaximum,&max);ac++;
		XtGetValues(h_scroll,al,ac);

		XmScrollBarGetValues(h_scroll,&h_val,&h_size,&h_inc,&h_page);

		max -= h_size;

		if( dh + h_val > max ) dh = max - h_val;
		if( dh + h_val < min ) dh = min - h_val;

	}


	if(dv || dh)
	{
		Position x = x_parent-dh;
		Position y = y_parent-dv;

		ac = 0;
		XtSetArg(al[ac],XmNx,x);ac++;
		XtSetArg(al[ac],XmNy,y);ac++;
		XtSetValues(_w,al,ac);


		if(dv) XmScrollBarSetValues(v_scroll,v_val+dv,
			v_size,v_inc,v_page,TRUE);
		if(dh) XmScrollBarSetValues(h_scroll,h_val+dh,
			h_size,h_inc,h_page,TRUE);
	}
}

void NodeShow(Widget _w,int node)
{
	SimpleBaseWidget sw = (SimpleBaseWidget)_w;
	Node *w;

	if( node < 0 || node >= sw->simplebase.count) return;
	w = sw->simplebase.nodes + node;
	if(!w->managed) return;

	SimpleBaseShow(_w,&w->r,NULL);
}

void NodeHideAll(Widget _w)
{
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	int i;
	for(i = 0; i < w->simplebase.count;i++)
	{
		Node *n = w->simplebase.nodes + i;
		n->managed = False;
	}
	NodeUpdate(_w);
}

Boolean NodeVisibility(Widget _w,int node,Boolean vis)
{
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	if (!w) {
	  return False;
	}
	Node *p = w->simplebase.nodes + node;
	if( node < 0 || node >= w->simplebase.count) return vis;
	if (0 == p) { 
	  fprintf(stderr, "unexpected\n");
	  return False;
	}
	if(p->managed == vis) return vis;

	/* if(node == w->simplebase.selected) selection(w); */

	p->managed = vis;

	if(vis && !p->inited)
	{
		p->size(_w,&p->r,p->user_data);
		p->inited = True;
	}

	enqueue_manage_proc(w);
	return !vis;
}

void NodeNewSizeAll(Widget _w)
{
  int i;
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	for(i = 0; i < w->simplebase.count;i++)
		NodeNewSize(_w,i);
	if(XtIsRealized(_w))
		XClearArea(XtDisplay(_w),XtWindow(_w),0,0,0,0,True);
}

void NodeNewSize(Widget _w,int node)
{
	XRectangle next,old;
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	Node *p ;
	if( w == 0 || node < 0 || node >= w->simplebase.count) return;

	p = w->simplebase.nodes + node;

	if( !p->managed)
	{
		p->inited = False;
		return;
	}


	old = next = p->r;
	p->size(_w,&next,p->user_data);

	if(next.x == p->r.x && next.y == p->r.y &&
		next.width ==  p->r.width &&  next.height ==  p->r.height)
			return;

	if(node == w->simplebase.selected) clear(w);

	p->r = next;

	/* if(node == w->simplebase.selected) selection(w);  */

	/* if( !p->managed) return; */

	if(!XtIsRealized(_w))
		return;

	XClearArea(XtDisplay(_w),XtWindow(_w),old.x,old.y,
		old.width,old.height,True);

	XClearArea(XtDisplay(_w),XtWindow(_w),p->r.x,p->r.y,
		p->r.width,p->r.height,True);

	if(p->managed)
		enqueue_manage_proc(w);
}



void NodeUpdate(Widget _w)
{
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	if(w->simplebase.work) {
		XtRemoveWorkProc(w->simplebase.work);
		w->simplebase.work = 0;
	}
	/* printf("update\n"); */
	manage_proc(w);
}



void NodeReset(Widget _w)
{
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	int i;
	for(i = 0; i < w->simplebase.count;i++)
	{
		Node *n = w->simplebase.nodes + i;
		if(n->parents) XtFree((XtPointer)n->parents);
		if(n->kids) XtFree((XtPointer)n->kids);
	}
	w->simplebase.count    = 0;
	w->simplebase.link_count    = 0;
	w->simplebase.selected = -1;
	w->simplebase.focus    = -1;
	memset(w->simplebase.nodes,0,w->simplebase.max*sizeof(Node));
	memset(w->simplebase.links,0,w->simplebase.link_max*sizeof(LinkData));
	NodeUpdate(_w);
}

void NodeReserve(Widget _w,int count)
{
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	if(count > w->simplebase.max)
	{
		w->simplebase.max = count;
		w->simplebase.nodes = 
		    (Node*)XtRealloc((XtPointer)w->simplebase.nodes,
		    w->simplebase.max*sizeof(Node));
		memset(w->simplebase.nodes + w->simplebase.count, 0,
		    (w->simplebase.max - w->simplebase.count)*sizeof(Node));
	}
}


void NodeAddRelation(Widget _w,int pnode,int knode)
{
	int i;
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	Node *p = w->simplebase.nodes + pnode;
	Node *k = w->simplebase.nodes + knode;

	if( pnode < 0 || pnode >= w->simplebase.count) return;
	if( knode < 0 || knode >= w->simplebase.count) return;

	for(i = 0 ; i < p->kcnt; i++)
		if(p->kids[i].node == knode)
			return;

	if(k->pcnt >= k->pmax)
	{
		k->pmax += k->pmax/2 + 1;
		k->parents = (Link*)XtRealloc((XtPointer)k->parents,k->pmax*sizeof(Link));
	}

	if(p->kcnt >= p->kmax)
	{
		p->kmax += p->kmax/2 + 1;
		p->kids = (Link*)XtRealloc((XtPointer)p->kids,p->kmax*sizeof(Link));
	}

	p->kids[p->kcnt].link_data    = -1;
	p->kids[p->kcnt++].node  = knode;

	k->parents[k->pcnt].link_data   = -1;
	k->parents[k->pcnt++].node = pnode;
	
	/*printf("NodeAddRelation %d %d (%d)\n",pnode,knode,kind);*/
}

void* NodeGetRelationData(Widget _w,int pnode,int knode)
{
	int i;
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	Node *p = w->simplebase.nodes + pnode;
	/* Node *k = w->simplebase.nodes + knode; */

	if( pnode < 0 || pnode >= w->simplebase.count) return 0;
	if( knode < 0 || knode >= w->simplebase.count) return 0;

	for(i = 0 ; i < p->kcnt; i++)
		if(p->kids[i].node == knode)
		{
			if( p->kids[i].link_data == -1)
				return 0;
			return w->simplebase.links[p->kids[i].link_data].user_data;
		}


	/* Check for dummies */
	for(i = 0 ; i < p->kcnt; i++)
	{
		Node* z = &KIDS(w,p,i);
		if(sb_is_dummy(w,z))
		{
			void *d = NodeGetRelationData(_w,NODE_TO_INDEX(w,z),knode);
			if(d) return d;
		}
	}


	return 0;
}

void* NodeSetRelationData(Widget _w,int pnode,int knode,void *data)
{
	int i;
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	Node *p = w->simplebase.nodes + pnode;
	/* Node *k = w->simplebase.nodes + knode; */

	if( pnode < 0 || pnode >= w->simplebase.count) return 0;
	if( knode < 0 || knode >= w->simplebase.count) return 0;

	for(i = 0 ; i < p->kcnt; i++)
		if(p->kids[i].node == knode)
		{
			void *old = 0;
			if( p->kids[i].link_data == -1)
				p->kids[i].link_data = new_link_data(w);
			else
				old = w->simplebase.links[p->kids[i].link_data].user_data;
			w->simplebase.links[p->kids[i].link_data].user_data = data;
			return old;
		}

	return 0;
}

GC NodeSetRelationGC(Widget _w,int pnode,int knode,GC rgc)
{
	int i;
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	Node *p = w->simplebase.nodes + pnode;
	/* Node *k = w->simplebase.nodes + knode; */
	GC gc = w->simplebase.gc;

	if( pnode < 0 || pnode >= w->simplebase.count) return gc;
	if( knode < 0 || knode >= w->simplebase.count) return gc;

	for(i = 0 ; i < p->kcnt; i++)
		if(p->kids[i].node == knode)
		{
			GC old = gc;
			if( p->kids[i].link_data == -1)
				p->kids[i].link_data = new_link_data(w);
			else
				old = w->simplebase.links[p->kids[i].link_data].gc;
			w->simplebase.links[p->kids[i].link_data].gc = rgc;
			enqueue_manage_proc(w);
			return old;
		}

	return gc;
}


static Boolean SetValues(SimpleBaseWidget current, 
SimpleBaseWidget request, 
SimpleBaseWidget new)
{
	int       redraw = FALSE;
	XGCValues values;
	XtGCMask  valueMask;

	if (new->simplebase.blink_color != current->simplebase.blink_color ||
	    new->core.background_pixel !=
	    current->core.background_pixel){
		valueMask         = GCForeground | GCBackground | 
		    GCFunction | GCLineWidth;
		values.foreground = new->simplebase.blink_color;
		values.background = new->core.background_pixel;
		values.function   = GXxor;
		values.line_width   = 2;
		XtReleaseGC((Widget)new,new->simplebase.blink_gc);
		new->simplebase.blink_gc = XtGetGC((Widget)new, valueMask, &values);
		redraw = TRUE;
	}

	if(XtIsRealized((Widget)new) && XtIsManaged((Widget)new))
		if(new->simplebase.selected != current->simplebase.selected)
		{
			clear(current);
			clear(new);
		}

	/* printf("Redraw %d\n",redraw); */

	return (redraw);
}

static void drawDummy(Widget w,XRectangle* r,void* d)
{
}

static void sizeDummy(Widget w,XRectangle* r,void* d)
{
	r->width = r->height = 0;
}

int sb_new_dummy_node(SimpleBaseWidget gw)
{
	int i;
	int n = gw->simplebase.count;
	/* int more = 0; */
	Node* z = 0;

	for(i=0; i < n; i++)
	{
		Node* w = gw->simplebase.nodes + i;
		if(w->draw == drawDummy && !w->managed)
		{
			printf("Recycle dummy %d\n",i);
			z = w;
			break;
		}
	}

	if(z == 0)
	{
		printf("Create dummy\n");
		i =  NodeCreate((Widget)gw,drawDummy,sizeDummy,0);
		z = INDEX_TO_NODE(gw,i);
		z->kids    = XtNew(Link);
		z->parents = XtNew(Link);
		z->kcnt = z->kmax = z->pcnt = z->pmax = 1;
	}

	z->r.width = z->r.height = 0;

	z->kids[0].node    = -1;
	z->parents[0].node = -1;

	z->managed = False;
	z->inited  = False;

	z->kids[0].link_data    = -1;
	z->parents[0].link_data = -1;

	return NODE_TO_INDEX(gw,z);
}

void sb_clear_dummy_nodes(SimpleBaseWidget gw)
{
	int i;
	int n = gw->simplebase.count;
	/* int more = 0; */
	int cnt = 0;

	for(i=0; i < n; i++)
	{
		Node* w = gw->simplebase.nodes + i;
		if(w->draw == drawDummy && w->managed)
		{
			Node *p = INDEX_TO_NODE(gw,w->parents[0].node);
			Node *k = INDEX_TO_NODE(gw,w->kids[0].node);
			int j;

			cnt++;

			j = sb_find_kid_index(gw,p,w);
			if(j == -1) {
				printf("Cannot find dummy in parent\n");
				abort();
			}
			p->kids[j].node = w->kids[0].node;

			j = sb_find_parent_index(gw,k,w);
			if(j == -1) {
				printf("Cannot find dummy in kid\n");
				abort();
			}
			k->parents[j].node = w->parents[0].node;

			w->kids[0].node         = -1;
			w->parents[0].node      = -1;
			w->kids[0].link_data    = -1;
			w->parents[0].link_data = -1;
			w->managed              = False;

		}
	}
	// printf("remove_dummy_nodes: %d\n",cnt);
}

int sb_insert_dummy_node(SimpleBaseWidget gw,int np,int nk)
{
	Node *p = INDEX_TO_NODE(gw,np);
	Node *k = INDEX_TO_NODE(gw,nk);
	int a = sb_find_kid_index(gw,p,k);
	int b = sb_find_parent_index(gw,k,p);
	int x;
	Node *z;

	if(a == -1)
	{
		printf("Cannot find kid in parent\n");
		abort();
	}

	if(b == -1)
	{
		printf("Cannot find parent in kid\n");
		abort();
	}

	x  = sb_new_dummy_node(gw);
	z = INDEX_TO_NODE(gw,x);

	/* sb_new_dummy_node may have changed the pointers */

	p = INDEX_TO_NODE(gw,np); 
	k = INDEX_TO_NODE(gw,nk);

	z->managed = True;

	p->kids[a].node      = x;
	z->parents[0].node   = np;
	z->kids[0].link_data = p->kids[a].link_data;;

	k->parents[b].node      = x;
	z->kids[0].node         = nk;
	z->parents[0].link_data = k->parents[b].link_data;

	return x;
}

int sb_find_kid_index(SimpleBaseWidget w,Node* p,Node *k)
{
	int i;
	int x = NODE_TO_INDEX(w,k);

	for(i=0;i<p->kcnt;i++)
		if( p->kids[i].node == x)
			return i;

	return -1;
}

int sb_find_parent_index(SimpleBaseWidget w,Node* k,Node *p)
{
	int i;
	int x = NODE_TO_INDEX(w,p);

	for(i=0;i<k->pcnt;i++)
		if( k->parents[i].node == x)
			return i;

	return -1;
}

Boolean sb_is_dummy(SimpleBaseWidget w,Node* n)
{
	return n->draw == drawDummy;
}

int NodeNewGroup(Widget _w,DrawProc draw, SizeProc size, void *data)
{ return 0;
}
void NodeSetGroup(Widget _w,int node,int group)
{
}
int NodeGetGroup(Widget _w,int node)
{
	return -1;
}

static void xincrement (h, event, args, n_args)
Widget   h;
XEvent        *event;
char          *args[];
int            n_args;
{
#ifdef MOTIF
#define SetArg(a,b)  XtSetArg(al[ac],a,b);ac++
#define GetValues(w) XtGetValues(w,al,ac);ac=0
#define SetValues(w) XtSetValues(w,al,ac);ac=0

  Widget clip = XtParent(h);
  Widget swin;
  Widget v_scroll;

  int ac = 0;

  Position    x_grep,y_grep;
  Dimension   h_grep,w_grep;
  Position    x_clip,y_clip;
  Dimension   h_clip,w_clip;
  Position    dv=0,dh=0;
  int min,max;
  int v_val,v_size,v_inc,v_page;
  int h_val,h_size,h_inc,h_page;
  Position x,y;
  
  Arg al[5];

  int arg;

  /* printf("## mouse 1\n"); */
  if(!clip) return;
  swin = XtParent(clip);
  /* printf("## mouse 2\n"); */
  if(!swin || !XmIsScrolledWindow(swin)) return;
  /* printf("## mouse 3\n"); */
  if (n_args != 1) return;

  SetArg(XmNverticalScrollBar  , &v_scroll);
  GetValues(swin);
  
  {
    int min, max, value, slider_size, inc, page_inc;
    SetArg(XmNminimum,&min);
    SetArg(XmNmaximum,&max);    
    SetArg(XmNvalue,&value);    
    SetArg(XmNsliderSize,&slider_size);    
    SetArg(XmNincrement,&inc);    
    SetArg(XmNpageIncrement,&page_inc);    
    
    GetValues(v_scroll);
    /* XmScrollBarGetValues(v_scroll, value, slider_size, inc, page_inc); */
    
    arg = atoi(args[0]);
    dh = (abs(arg) > 5) ? page_inc : inc;

    if (arg < 0) {
      if (value - dh < min)
        value = min;
      else
        value -= dh;
    } else {
      if (value + dh > max)
        value = max;
      else
        value += dh;
    }

    XmScrollBarSetValues(v_scroll,value,slider_size, inc, page_inc,TRUE);
  }
#endif
}
