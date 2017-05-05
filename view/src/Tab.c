/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #6 $                                                                    */
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

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#define  RADIANS(x)  (M_PI * (x) / 180.0)
#define  ROUND(X)  (((X) >= 0) ? (int)((X)+0.5) : (int)((X)-0.5))


#include      <X11/Intrinsic.h>
#include      <X11/IntrinsicP.h>
#include      <X11/StringDefs.h>
#include      <X11/CoreP.h>
#include    <X11/CompositeP.h>
#include      <X11/ConstrainP.h>
#include        <Xm/XmP.h>
#include        <Xm/DrawingAP.h>
#include      "Tab.h"
#include      "TabP.h"

#ifndef MAX
#define   MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define   MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

static void             Initialize();
static void             Resize();
static void             Destroy();
static void             ReDisplay();
static void             Click();
static Boolean          SetValues();
static XtGeometryResult GeometryManager();
static void             ChangeManaged();
static void             new_layout(TabWidget,Boolean);

static XtResource resources[] = {
	{XmNvalueChangedCallback,XmCValueChangedCallback,XtRCallback,
	 sizeof(XtPointer), XtOffset(TabWidget,tab.cb), XtRCallback, 
	 NULL },

	{XmNopenCallback,XmCValueChangedCallback,XtRCallback,
	 sizeof(XtPointer), XtOffset(TabWidget,tab.open_cb), XtRCallback, 
	 NULL },

	{XmNcloseCallback,XmCValueChangedCallback,XtRCallback,
	 sizeof(XtPointer), XtOffset(TabWidget,tab.close_cb), XtRCallback,
	 NULL },

	{XmNfontList,XmCFontList,XmRFontList,sizeof(XmRFontList),
	 XtOffset(TabWidget,tab.font),XmRString,(XtPointer)"fixed" },

	{ "Back", "back", XmRPixel,sizeof(Pixel),
	  XtOffset(TabWidget,tab.back),XmRString,"#bcbcbcbcbcbc"},

	{ "Blue", "blue", XmRPixel,sizeof(Pixel),
	  XtOffset(TabWidget,tab.blue),XmRString,"blue"},

	{ "drawer", "Drawer", XmRBoolean,sizeof(Boolean),
	  XtOffset(TabWidget,tab.drawer),XmRString,"false"},
};


static XtActionsRec actions[] = {
	{"Click",Click}
};


static char translations[] =
"<Btn1Down>: Click()";

#define USE_MANAGER

TabClassRec tabClassRec = {
	{
	/* core_class fields  */
	(WidgetClass) &xmDrawingAreaClassRec,/* superclass         */
	"Tab",                           /* class_name         */
	sizeof(TabRec),                /* widget_size        */
	NULL,                             /* class_init         */
	NULL,                             /* class_part_init    */
	FALSE,                            /* class_inited       */
	Initialize,                       /* initialize         */
	NULL,                             /* initialize_hook    */
	XtInheritRealize,                 /* realize            */
	actions,                             /* actions            */
	XtNumber(actions),                                /* num_actions        */
	resources,                        /* resources          */
	XtNumber(resources),              /* num_resources      */
	NULLQUARK,                        /* xrm_class          */
	TRUE,                             /* compress_motion    */
	XtExposeCompressMaximal,          /* compress_exposure  */
	TRUE,                             /* compress_enterleave*/
	TRUE,                             /* visible_interest   */
	Destroy,                          /* destroy            */
	Resize,                           /* resize             */
	ReDisplay,                        /* expose             */
	SetValues,                        /* set_values         */
	NULL,                             /* set_values_hook    */
	XtInheritSetValuesAlmost,         /* set_values_almost  */
	NULL,                             /* get_values_hook    */
	NULL,                             /* accept_focus       */
	XtVersion,                        /* version            */
	NULL,                             /* callback_private   */
	translations,            /* tm_table           */
	NULL,                             /* query_geometry     */
	XtInheritDisplayAccelerator,      /* display_accelerator*/
	NULL,                             /* extension          */
	},
	{
	/* composite_class fields */
	GeometryManager,                 /* geometry_manager    */
	ChangeManaged,                   /* change_managed      */
	XtInheritInsertChild,            /* insert_child        */
	XtInheritDeleteChild,            /* delete_child        */
	NULL ,                           /* extension           */
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
	/* Tab class fields */
	0,                               /* ignore              */
	},
};

static void make_visible(TabWidget tw,Widget w);

WidgetClass tabWidgetClass = (WidgetClass) &tabClassRec;

static void Initialize(TabWidget request, TabWidget new)
{

	XGCValues       values;
        XtGCMask        valueMask = 0;

	XFontStruct     *fs = (XFontStruct *) NULL;

	/*
	 * Make sure the widget's width and height are 
	 * greater than zero.
	 */
	if (request->core.width <= 0)
		new->core.width = 5;
	if (request->core.height <= 0)
		new->core.height = 5;

	_XmFontListGetDefaultFont(new->tab.font,&fs);
	if(fs != NULL)
	{
		valueMask    |= GCFont;
		values.font  = fs->fid;
	}
      
	new->tab.gc     = XtGetGC((Widget)new,valueMask,&values);
	new->tab.current = 0;

	new->tab.hmargin = 8;
	new->tab.vmargin = 3;

	new->tab.top    = 2;
	new->tab.bottom = 2;
	new->tab.delta  = new->tab.hmargin;
}

static void Destroy(TabWidget w)
{
	XtReleaseGC((Widget)w,w->tab.gc);
}

static void Resize(TabWidget w)
{
	XmDrawingAreaCallbackStruct cb;
	new_layout(w,False);
	cb.reason = XmCR_RESIZE;
	cb.event = NULL;
	cb.window = XtWindow (w);
	XtCallCallbackList((Widget)w,w->drawing_area.resize_callback, &cb);
	make_visible(w,w->tab.current);
	if(XtIsRealized((Widget)w))
		XClearArea(XtDisplay(w),XtWindow(w),0,0,0,0,True);
}

static Boolean SetValues(TabWidget current, TabWidget request, TabWidget new)
{
	new_layout(new,False);
	return (False);
}

static XtGeometryResult GeometryManager(Widget w, 
	XtWidgetGeometry *request, 
	XtWidgetGeometry *reply)
{
	new_layout((TabWidget)XtParent(w),True);
	return XtGeometryYes;
}

static void ChangeManaged(TabWidget tw)
{
	new_layout(tw,True);
}

static char* name_of(Widget w)
{
	if(XmIsScrolledWindow(w))
	{
		static char name[90];
		strcpy(name,XtName(w));
		name[strlen(name)-2] = 0;
		return name;
	}
	else
		return XtName(w);	
		
}

static void new_layout(TabWidget tw,Boolean geometry)
{
        /* Dimension w = tw->core.width; */
	/* Dimension h = tw->core.height; */
	int t;
	int i;

	Dimension mw = 0;
	Dimension mh = 0;
	Dimension ww = 0;

	Dimension width  = tw->core.width;
	Dimension height = tw->core.height;

	tw->tab.title = 0;


	for(i=0;i<tw->composite.num_children;i++)
	{
		Widget c = tw->composite.children[i];
		if(XtIsManaged(c))
		{
			char *n = name_of(c);
			XmString s = XmStringCreateSimple(n);
			int   h = XmStringHeight(tw->tab.font, s) + 2 * tw->tab.vmargin;
			int   w = XmStringWidth(tw->tab.font, s)  + 2 * tw->tab.hmargin;
			XmStringFree(s);

			ww += w;

			mh  = MAX(mh,c->core.height);
			mw  = MAX(mh,c->core.width);

			tw->tab.title = MAX(tw->tab.title,h);

		}
	}

	if(tw->tab.drawer && !geometry )
		height = tw->tab.title;
	else
		height = tw->tab.title + tw->tab.top + tw->tab.bottom + mh;

	width  = 2 * tw->tab.hmargin + MAX(ww,mw);

    if(geometry)
    if(tw->core.width < width || tw->core.height < height)
    {
        Dimension           maxWidth = width, maxHeight = height;
        XtGeometryResult    result;
        Dimension           replyWidth = 0, replyHeight = 0;

        result = XtMakeResizeRequest(
            (Widget)tw,
            maxWidth,
            maxHeight,
            &replyWidth, &replyHeight);

        if (result == XtGeometryAlmost)
            XtMakeResizeRequest (
                (Widget)tw,
                replyWidth,
                replyHeight,NULL, NULL);
    }


	width  = tw->core.width;
	t      = (tw->tab.title + tw->tab.top + tw->tab.bottom);
	height = tw->core.height - t;

	for(i=0;i<tw->composite.num_children;i++)
	{
		Widget c = tw->composite.children[i];
		_XmConfigureObject((Widget)c,0,t,width,height,0);
	}

}

static void draw(TabWidget tw, Widget c,int* x,int* y,int k)
{
	char *n = name_of(c);
	XmString s = XmStringCreateSimple(n);
	XPoint points[10];
	int count = 4;
	int minx = 0;
	int maxx = 0;
	int xx = 0;
	int i;

	int sw    = XmStringWidth(tw->tab.font, s);
	int step  = sw + 2 * tw->tab.hmargin;

	points[0].x = *x - tw->tab.hmargin / 2;
	points[0].y = tw->tab.title + tw->tab.top ;

	points[1].x = tw->tab.hmargin ;
	points[1].y = -tw->tab.title;

	points[2].x = sw + tw->tab.hmargin;
	points[2].y = 0;

	points[3].x = tw->tab.hmargin ;
	points[3].y = tw->tab.title;

	if(points[0].x + points[1].x + points[2].x + points[3].x > tw->core.width  )
	{
		int ww = 5;
		int xx = points[0].x + points[1].x;

		points[2].x = tw->core.width - xx - ww;
		points[2].y = 0;

		points[3].x = -ww;
		points[3].y = tw->tab.title / 3;

		points[4].x = ww;
		points[4].y = tw->tab.title / 3;

		points[5].x = -ww;
		points[5].y = tw->tab.title - 2 * (tw->tab.title / 3);

		count = 6;

		if(k < tw->tab.last)
			tw->tab.last = k;

		if(points[2].x < points[1].x)
		{
			(*x) += step;
			return;
		}
	}

	if(points[0].x < 0)
	{
		int ww = 5;

		points[0].x = 0;
		points[0].y = tw->tab.title + tw->tab.top ;

		points[1].x = ww;
		points[1].y = -tw->tab.title / 3;

		points[2].x = -ww;
		points[2].y = -tw->tab.title / 3;

		points[3].x = ww;
		points[3].y = -tw->tab.title + 2 * (tw->tab.title / 3);

		points[4].x = *x - tw->tab.hmargin / 2 + 2*tw->tab.hmargin + sw - ww ;
		points[4].y = 0;

		points[5].x = tw->tab.hmargin ;
		points[5].y = tw->tab.title;

		count = 6;

		if(k > tw->tab.first)
			tw->tab.first = k;

		if(points[4].x < 0 )
		{
			(*x) += step;
			return;
		}
	}

	minx = points[0].x;
	maxx = points[0].x;
	xx   = points[0].x;

	for(i = 1; i < count; i++)
	{
		xx += points[i].x;
		if(xx < minx) minx = xx;
		if(xx > maxx) maxx = xx;
	}
	
	XSetForeground(XtDisplay(tw),
		tw->tab.gc,
		(c == tw->tab.current) ? 
			tw->core.background_pixel:
			tw->tab.back);
		
	XFillPolygon(
		XtDisplay(tw),
		XtWindow(tw),
		tw->tab.gc,
		points,
		count,
		Convex,
		CoordModePrevious
	);

	XSetForeground(XtDisplay(tw),
		tw->tab.gc,
		tw->manager.foreground);

	XDrawLines(
		XtDisplay(tw),
		XtWindow(tw),
		/* tw->tab.gc, */
		tw->manager.bottom_shadow_GC,
		points,
		count,
		CoordModePrevious
	);

	XSetForeground(XtDisplay(tw),
		tw->tab.gc,
		(c == tw->tab.current) ? 
			tw->tab.blue:
			tw->manager.foreground);

	{
		char buf[1024];
		int i = strlen(n);

		while(--i >= 0 && sw > (maxx - minx - 2 * tw->tab.hmargin))
		{
			XmStringFree(s);
			strncpy(buf,n,i);
			buf[i] = '.';
			buf[i+1] = '.';
			buf[i+2] = '.';
			buf[i+3] = 0;

			s  = XmStringCreateSimple(buf);
			sw = XmStringWidth(tw->tab.font, s);
		}

		

	XmStringDraw(XtDisplay(tw),
		XtWindow(tw),
		tw->tab.font,
		s,
		tw->tab.gc,
		minx ,
		tw->tab.vmargin + tw->tab.top + tw->tab.vmargin/3,
		maxx - minx,
		XmALIGNMENT_CENTER,
		XmSTRING_DIRECTION_L_TO_R,
		NULL);

	}

	XSetForeground(XtDisplay(tw),
		tw->tab.gc,
		tw->manager.foreground);

	XmStringFree(s);

	(*x) += step;
}

static void ReDisplay(Widget w, XEvent *event, Region region)
{
	TabWidget tw = (TabWidget)w;

	int x = tw->tab.delta;
	int y = 0;
	int i;
	int cx = 0,cy= 0, ci = 0;

	tw->tab.first = -1;
	tw->tab.last  = tw->composite.num_children + 1;

	for(i=0;i<tw->composite.num_children;i++)
	{
		Widget c = tw->composite.children[i];
		if(XtIsManaged(c))
		{
			if(!tw->tab.current) tw->tab.current = c;

			if(c == tw->tab.current) { cx = x; cy = y; ci = i; }
			draw(tw,c,&x,&y,i);
		}
	}

	if(tw->tab.current)
	{
		GC gc = tw->tab.gc;
		int t = tw->tab.title + tw->tab.top;

		x = cx;
		y = cy;

		draw(tw,tw->tab.current,&x,&y,ci);

		/*========================*/

		XSetForeground(XtDisplay(tw),gc,
			tw->core.background_pixel);

		XDrawLine(XtDisplay(tw),
			XtWindow(tw),
			gc, 0, t, tw->core.width, t);

		XSetForeground(XtDisplay(tw),
			tw->tab.gc,
			tw->manager.foreground);

		/*========================*/

		gc = tw->manager.bottom_shadow_GC;

		XDrawLine(XtDisplay(tw),
			XtWindow(tw),
			gc,
			0,
			t,
			cx - tw->tab.hmargin/2,
			t);

		XDrawLine(XtDisplay(tw),
			XtWindow(tw),
			gc,
			x + tw->tab.hmargin/2,
			t,
			tw->core.width,
			t);
	}

	if(tw->tab.current && XtIsRealized(tw->tab.current))
		XRaiseWindow(XtDisplay(tw->tab.current),XtWindow(tw->tab.current));
}


Widget CreateTab(Widget par,char* nam,Arg* al,int ac)
{
	return XtCreateWidget(nam,tabWidgetClass,par,al,ac);
}

Widget TabGetCurrent(Widget w)
{
	TabWidget tw = (TabWidget)w;
	return tw->tab.current;
}

static int opened_size(TabWidget tw)
{
	XtWidgetGeometry preferred;
	int size;

	XtQueryGeometry(tw->tab.current,NULL,&preferred);

	if((preferred.request_mode & CWHeight) != 0)
		size = preferred.height;
	else
		size = tw->tab.current->core.height;

	return size + tw->tab.title + tw->tab.vmargin;
}

static void open_close_tab(TabWidget tw)
{
	if(tw->core.height == tw->tab.title)
		XtVaSetValues((Widget)tw,XmNheight,opened_size(tw),NULL);
	else
		XtVaSetValues((Widget)tw,XmNheight,tw->tab.title,NULL);
}

static void open_full(TabWidget tw)
{
	int size = opened_size(tw);
	if(tw->core.height < size)
		XtVaSetValues((Widget)tw,XmNheight,size,NULL);
}

static void set_tab(Widget w,Widget c,Boolean tell,XEvent* ev)
{
	TabWidget tw = (TabWidget)w;
	TabCallbackStruct cb;

	while(c && XtParent(c) != w)
		c = XtParent(c);

	if(!c) return;

	if(tw->tab.current == c)
	{
	  if(tw->tab.drawer) {
	    if(ev)
	      open_close_tab(tw);
	    else
	      open_full(tw);
	  }	  
	  return;
	}

	cb.reason = XmCR_VALUE_CHANGED;
	cb.widget = c;
	cb.event  = ev;

	if(tell) 
		XtCallCallbacks(w, XmNvalueChangedCallback, &cb);

	tw->tab.current = cb.widget;

	if(tw->tab.drawer) 
		open_full(tw);

	make_visible(tw,tw->tab.current);

	if(XtIsRealized(w))
		XClearArea(XtDisplay(w),XtWindow(w),0,0,0,0,True);
}

void TabSetCurrent(Widget w,Widget c,Boolean tell)
{
	set_tab(w,c,tell,NULL);
}

void TabOpen(Widget w)
{
	open_full((TabWidget)w);
}

void TabClose(Widget w)
{
	TabWidget tw = (TabWidget)w;
	new_layout(tw,True);
	XtVaSetValues(w,XmNheight,tw->tab.title,NULL);
}

Boolean TabClosed(Widget w)
{
	TabWidget tw = (TabWidget)w;
	return tw->tab.title == tw->core.height;
}

static void Click(Widget w, XEvent *event, String *params,Cardinal *nparams)
{
	TabWidget tw = (TabWidget)w;

	int x = tw->tab.delta;
	int i;

	for(i=0;i<tw->composite.num_children;i++)
	{
		Widget c = tw->composite.children[i];
		if(XtIsManaged(c))
		{
			char *n    = name_of(c);
			XmString s = XmStringCreateSimple(n);

			int   h = XmStringHeight(tw->tab.font, s) + 2 * tw->tab.vmargin;
			int   w = XmStringWidth(tw->tab.font, s)  + 2 * tw->tab.hmargin;

			XmStringFree(s);

			if(event->xbutton.y >= tw->tab.top && 
			   event->xbutton.y <= h + tw->tab.top)
				if(event->xbutton.x >= x && event->xbutton.x <= x + w)
				{

					if(i <= tw->tab.first || i>= tw->tab.last)
						make_visible(tw,c);
					else
						set_tab((Widget)tw,c,True,event);
					break;
				}
			x += w;
		}
	}
}


static void make_visible(TabWidget tw, Widget wid)
{
	int x = 0;
	int i;
	int m = 0;
	int j = 0;

	int from,to;

	int* pos = (int*)XtCalloc(sizeof(int),tw->composite.num_children+1);

	for(i=0;i<tw->composite.num_children;i++)
	{
		Widget c = tw->composite.children[i];
		if(XtIsManaged(c))
		{
			char *n    = name_of(c);
			XmString s = XmStringCreateSimple(n);

			/* int   h = XmStringHeight(tw->tab.font, s) + 2 * tw->tab.vmargin; */
			int   w = XmStringWidth(tw->tab.font, s)  + 2 * tw->tab.hmargin; 

			XmStringFree(s);

			if(c == wid)
				j = m;
			pos[m++] = x;
			x += w;
		}
	}

	pos[m] = x;

	if(j == 0 || j == m-1)
	{
		from = pos[j]   - tw->tab.hmargin;
		to   = pos[j+1] + tw->tab.hmargin;;
	}
	else {
		from = (pos[j]+pos[j-1])/2;
		to   = (pos[j+2]+pos[j+1])/2;
	}

	tw->tab.delta = 5; /*  start from first tab */

	if(from + tw->tab.delta < 0)
	{
		tw->tab.delta = -from;	
		if(XtIsRealized((Widget)tw))
			XClearArea(XtDisplay(tw),XtWindow(tw),0,0,0,0,True);
	}

	if(to + tw->tab.delta > tw->core.width )
	{
		tw->tab.delta = -(to-tw->core.width);
		if(XtIsRealized((Widget)tw))
			XClearArea(XtDisplay(tw),XtWindow(tw),0,0,0,0,True);
	}

	XtFree((XtPointer)pos);
}
