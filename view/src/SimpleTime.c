/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #4 $                                                                    */
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
#include <time.h>
#include <Xm/ScrollBar.h>

#include      <X11/Intrinsic.h>
#include      <X11/IntrinsicP.h>
#include      <X11/StringDefs.h>
#include      <X11/CoreP.h>
#include    <X11/CompositeP.h>
#include      <X11/ConstrainP.h>
#include        <Xm/XmP.h>
#include        <Xm/DrawingAP.h>
#include        <Xm/ExtObjectP.h>
#include      "SimpleTime.h"
#include      "SimpleTimeP.h"

#ifndef MAX
#define   MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define   MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define DATE  0
#define TIME  1
#define ARC   2

static void             Initialize();
//static void             ConstraintInitialize();
//static Boolean          ConstraintSetValues();
//static void             Resize();
static void             Print();
static void             Layout(Widget,long*,long*);
static void             Destroy();
static Boolean          SetValues();
//static void             insert_new_node();
//static void             delete_node();
//static void             new_layout();
static void             Redisplay();
//static int              compute_positions();
//static void             shift_subsimpletime();
//static void             set_positions();
/* static void             reset(); */

void SimpleBaseShow(Widget _w,XRectangle* r,XEvent* ev);


static XtResource resources[] = {
	{XtNverticalSpace,XtCSpace, XtRDimension,sizeof (Dimension),
	XtOffset(SimpleTimeWidget, simpletime.v_min_space), XtRImmediate,(XtPointer)8  },
	{XtNpixelSecond,XtCPixelSecond, XtRInt,sizeof (int),
	XtOffset(SimpleTimeWidget, simpletime.second_per_pixel), XtRImmediate,(XtPointer)60 },
	{XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
	XtOffset(SimpleTimeWidget, simpletime.foreground), XtRString,"Red"},
	{XtNautoScroll, XtCAutoScroll, XtRBoolean, sizeof (Boolean),
	XtOffset(SimpleTimeWidget, simpletime.auto_scroll), XtRImmediate,(XtPointer)TRUE},

    { XmNfontList, XmCFontList, XmRFontList, sizeof (XmFontList),
    XtOffset (SimpleTimeWidget, simpletime.font), XmRString, 
		"-*-*-*-*-*-*-7-*-*-*-*-*-*-*"},



};


SimpleTimeClassRec simpletimeClassRec = {
	{
	/* core_class fields  */
	(WidgetClass) &simplebaseClassRec,/* superclass         */
	"SimpleTime",                           /* class_name         */
	sizeof(SimpleTimeRec),                /* widget_size        */
	NULL,                             /* class_init         */
	NULL,                             /* class_part_init    */
	FALSE,                            /* class_inited       */
	Initialize,                       /* initialize         */
	NULL,                             /* initialize_hook    */
	XtInheritRealize,                 /* realize            */
	NULL,                             /* actions            */
	0,                                /* num_actions        */
	resources,                        /* resources          */
	XtNumber(resources),              /* num_resources      */
	NULLQUARK,                        /* xrm_class          */
	TRUE,                             /* compress_motion    */
	XtExposeCompressMaximal,          /* compress_exposure  */
	TRUE,                             /* compress_enterleave*/
	TRUE,                             /* visible_interest   */
	Destroy,                          /* destroy            */
	NULL,                             /* resize             */
	Redisplay,                        /* expose             */
	SetValues,                        /* set_values         */
	NULL,                             /* set_values_hook    */
	XtInheritSetValuesAlmost,         /* set_values_almost  */
	NULL,                             /* get_values_hook    */
	NULL,                             /* accept_focus       */
	XtVersion,                        /* version            */
	NULL,                             /* callback_private   */
	XtInheritTranslations,            /* tm_table           */
	NULL,                             /* query_geometry     */
	XtInheritDisplayAccelerator,      /* display_accelerator*/
	NULL,                             /* extension          */
	},
	{
	/* simplebase_class fields */
	NULL,                 /* geometry_manager    */
	NULL,                   /* change_managed      */
	XtInheritInsertChild,            /* insert_child        */
	XtInheritDeleteChild,            /* delete_child        */
	NULL/*&compext*/,                        /* extension           */
	},
	{ 
	/* constraint_class fields */
	NULL,          /* subresources        */
	0,/* subresource_count   */
	0,       /* constraint_size     */
	NULL,             /* initialize          */
	NULL,                			  /* destroy             */
	NULL,              /* set_values          */
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
	Print,
	Layout,
	},
	{
	/* SimpleTime class fields */
	0,                               /* ignore              */
	},
};


WidgetClass simpletimeWidgetClass = (WidgetClass) &simpletimeClassRec;

static long time_to_sec(long ddate)
{
	long hh,mm,ss;
	hh = ddate / 10000; ddate %= 10000;
	mm = ddate / 100; ddate %= 100;
	ss = ddate;
	return hh*60*60 + mm * 60 + ss;
}

static long sec_to_time(long ddate)
{
	long hh,mm,ss;
	hh = ddate / (60*60); ddate %= (60*60);
	mm = ddate / 60; ddate %= 60;
	ss = ddate;
	return hh*10000 + mm * 100 + ss;
}

static long date_to_julian(long ddate)
{
	long  m1,y1,a,b,c,d,j1;

	long month,day,year;

	year = ddate / 10000;
	ddate %= 10000;
	month  = ddate / 100;
	ddate %= 100;
	day = ddate;

	if (month > 2)
	{
		m1 = month - 3;
		y1 = year;
	}
	else
	{
		m1 = month + 9;
		y1 = year - 1;
	}
	a = 146097*(y1/100)/4;
	d = y1 % 100;
	b = 1461*d/4;
	c = (153*m1+2)/5+day+1721119;
	j1 = a+b+c;

	return(j1);
}

static long julian_to_date(long jdate)
{
	long x,y,d,m,e;
	long day,month,year;

	x = 4 * jdate - 6884477;
	y = (x / 146097) * 100;
	e = x % 146097;
	d = e / 4;

	x = 4 * d + 3;
	y = (x / 1461) + y;
	e = x % 1461;
	d = e / 4 + 1;

	x = 5 * d - 3;
	m = x / 153 + 1;
	e = x % 153;
	d = e / 5 + 1;

	if( m < 11 )
		month = m + 2;
	else
		month = m - 10;


	day = d;
	year = y + m / 11;

	return year * 10000 + month * 100 + day;
}

static int x_of(SimpleTimeWidget w,int d,int t)
{
	double s;
	d = d - w->simpletime.start_date;
	t = t - w->simpletime.start_time;
	s = d * 24.0*60*60 + t;
	return s / w->simpletime.second_per_pixel + 10 + w->simpletime.max_w;
}

static void time_of(SimpleTimeWidget w,int x,int* d,int* t)
{
	double s = (x - 10 - w->simpletime.max_w) * w->simpletime.second_per_pixel;
	*d = (s / 24.0/60/60);
	*t = ( s - *d * 24.0*60*60);
	*d = *d + w->simpletime.start_date;
	*t = *t + w->simpletime.start_time;
}

static void time_out(SimpleTimeWidget w,XtIntervalId id)
{
	// time_t   t = time(0);
	/* struct tm *tt = gmtime(&t); */


	if(XtIsRealized((Widget)w) && XtIsManaged((Widget)w))
	{
#if 0
		int n = X(w,w->simpletime.second);
		XClearArea(XtDisplay(w),XtWindow(w),
		    n,0,1,w->core.height,TRUE);
#endif
	}

	/* w->simpletime.second = tt->tm_min+tt->tm_hour*60; */
	/* w->simpletime.second = tt->tm_min+tt->tm_hour*60; */

	w->simpletime.timeout_id = 
	    XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w),
	    60000,
	    (XtTimerCallbackProc)time_out,(XtPointer)w);

	/* if(w->simpletime.auto_scroll) TimeShowTime((Widget)w,TIME_NOW); */
}


static void Initialize(SimpleTimeWidget request, SimpleTimeWidget new)
{
	XGCValues values;
	XtGCMask  valueMask;

	if (request->core.width <= 0)
		new->core.width = 5;
	if (request->core.height <= 0)
		new->core.height = 5;

	valueMask = GCForeground | GCBackground;
	values.foreground = new->simpletime.foreground;
	values.background = new->core.background_pixel;
	new->simpletime.gc = XtGetGC((Widget)new, valueMask, &values);
	new->simpletime.start_time = new->simpletime.end_time = 0;
	new->simpletime.start_date = new->simpletime.end_date = 0;
	new->simpletime.inited     = 0;
	new->simpletime.arcs       = 0;

	time_out(new,0);

}

static void Destroy(SimpleTimeWidget w)
{
	XtReleaseGC((Widget)w, w->simpletime.gc);
	XtRemoveTimeOut(w->simpletime.timeout_id);
}


static Boolean SetValues(SimpleTimeWidget current,SimpleTimeWidget request,
	SimpleTimeWidget  new)
{
	int       redraw = FALSE;

	/*
  * If the minimum spacing has changed, recalculate the
  * simpletime layout. new_layout() does a redraw, so we don't
  * need SetValues to do another one.
  */

	if (new->simpletime.v_min_space != current->simpletime.v_min_space ||
	    new->simpletime.second_per_pixel != current->simpletime.second_per_pixel )
	{
		long width,height;
		Layout((Widget)new,&width,&height);
		if(width != new->core.width || height != new->core.height)
		{
			Dimension           replyWidth = 0, replyHeight = 0;

			XtGeometryResult result = XtMakeResizeRequest((Widget)new,
				width,height,
				&replyWidth, &replyHeight);

			if (result == XtGeometryAlmost)
				XtMakeResizeRequest ((Widget)new, replyWidth, replyHeight,
					NULL, NULL);
		}
		redraw = True;
	}
	return (redraw);
}


static void line_in(SimpleTimeWidget w,int x1,int y1,int x2,int y2)
{
	XDrawLine(XtDisplay(w), XtWindow(w), w->manager.bottom_shadow_GC,
	    x1,y1,x2,y2);
	XDrawLine(XtDisplay(w), XtWindow(w), w->manager.top_shadow_GC,
	    x1+1,y1+1,x2+1,y2+1);
}

//static void line_out(SimpleTimeWidget w,int x1,int y1,int x2,int y2)
//{
//	XDrawLine(XtDisplay(w), XtWindow(w), w->simpletime.gc,
//	    x1,y1,x2,y2);
//}

static void Redisplay (SimpleTimeWidget w, XEvent *event, Region region)
{
	int              i;
	int				n;
	int y;
	XEvent			ev;
	/* int max  = 0; */

	int d,t;
	int date = 0;

	/*
   * If the SimpleTime widget is visible, visit each managed child.
   */

	while(XCheckWindowEvent(XtDisplay(w),XtWindow(w),ExposureMask,&ev))
		XtAddExposureToRegion(&ev,region);

	d = w->simpletime.start_date;
	t = w->simpletime.start_time;

	t /= 60*60;
	t *= 60*60;

	while( d <= w->simpletime.end_date )
	{
		int width;
		n = x_of(w,d,t);

		if(n >= w->simpletime.max_w) {
			int hh = sec_to_time(t) / 10000;
			XmString s,z,sdat,stim,sep;
			char dat[80];
			char tim[80];

			line_in(w,n,w->simpletime.title,n,w->core.height);

			dat[0] = tim[0] = 0;

			if(date == 0 || d != date)
			{
				int yy,mm,dd;
				int x = julian_to_date(d);
				yy = x / 10000; x %= 10000;
				mm = x / 100; x %= 100;
				dd = x;

				sprintf(dat,"%d-%02d-%02d",yy,mm,dd);
			}

			sprintf(tim,"%02dh",hh);

			sdat = XmStringCreateSimple(dat);
			stim = XmStringCreateSimple(tim);
			sep  = XmStringSeparatorCreate();

			z    = XmStringConcat(sdat,sep);
			s    = XmStringConcat(z,stim);

			date = d;
			width = XmStringWidth(w->simpletime.font,s);

			XmStringDraw(XtDisplay(w),XtWindow(w),
				w->simpletime.font,
				s,
				w->manager.bottom_shadow_GC,
				n - width / 2,
				5,
				width,
				XmALIGNMENT_CENTER, 
				XmSTRING_DIRECTION_L_TO_R, 0);

			XmStringFree(s);
			XmStringFree(z);
			XmStringFree(sdat);
			XmStringFree(stim);
			XmStringFree(sep);
		}

		t += 60*60;
		if(t >= 24*60*60)
		{
			d++;
			t -= 24*60*60;
		}

		if( d == w->simpletime.end_date && t > w->simpletime.end_time)
			break;
	}
	
	for (i = 0; i < w -> simplebase.count; i++)
	{

		NodeStruct *n = w -> simplebase.nodes + i;
		int j;
		if(!n->managed)
			continue;

		for (j = 0; j < n->kcnt; j++)
		{
			NodeStruct *c = &KIDS(w,n,j);

			if(!c->managed)
				continue;

#if 0
			if(n->kids[j].link_data != -1)
			{
				GC gc = w->simplebase.links[n->kids[j].link_data].gc;
				XDrawLine(XtDisplay(w), XtWindow(w), 
					gc,
					n->r.x + n->r.width/2 , 
					n->r.y + n->r.height/2 ,
					c->r.x + c->r.width/2  ,
					c->r.y + c->r.height/2);
			 }
#endif

				
		}

	}

	y = w->simpletime.max_h + w->simpletime.title;
	 for (i = 0; i < w ->simpletime.arcs; i++)
	 {
		line_in(w,0,y, w->core.width, y);
		y +=  w->simpletime.max_h;
	}

	NodesRedraw((SimpleBaseWidget)w,event,region);

	
}

static void Print(SimpleTimeWidget w,FILE *f)
{
}


static void calc_arc(SimpleTimeWidget tw,NodeStruct* w,int arc)
{
	int j;
	if(w->misc[ARC] == -1 && w->managed)
	{
		w->misc[ARC] = arc;
		for (j = 0; j < w->kcnt; j++)
		{
			NodeStruct *c = &KIDS(tw,w,j);
			calc_arc(tw,c,arc);
		}

		for (j = 0; j < w->pcnt; j++)
		{
			NodeStruct *c = &PARENTS(tw,w,j);
			calc_arc(tw,c,arc);
		}

	}
}

static void Layout(Widget w,long *maxWidth,long *maxHeight)
{
	SimpleTimeWidget tw = (SimpleTimeWidget)w;
//	XtGeometryResult    result;
//	Dimension           replyWidth = 0, replyHeight = 0;
	int i;
//	int nlines = 0;
//	Position		*lines;
	int arc = 0;

	XmString s = XmStringCreateSimple("0123456789:- ");
	tw->simpletime.title = XmStringHeight(tw->simpletime.font,s)*2 + 10;
	XmStringFree(s);

	*maxWidth = *maxHeight = 5;

	if(tw->simplebase.count == 0)
	{
		tw->simpletime.inited = 0;
		return;
	}


	tw->simpletime.max_w = 0;
	tw->simpletime.max_h = 0;

	for(i=0;i<tw->simplebase.count;i++)
	{
		NodeStruct  *w = tw->simplebase.nodes + i;
		if(!w->managed) continue;

		w->misc[ARC] = -1;
	    if(w->r.height> tw->simpletime.max_h) 
			tw->simpletime.max_h = w->r.height;

		if(w->misc[DATE] == 0)
			if(w->r.width > tw->simpletime.max_w)
				tw->simpletime.max_w = w->r.width;
	}

	tw->simpletime.max_h += 4;

	for(i=0;i<tw->simplebase.count;i++)
	{
		NodeStruct  *w = tw->simplebase.nodes + i;
		if(!w->managed) continue;
		if(w->misc[ARC] == -1)
			calc_arc(tw,w,arc++);
	}

	tw->simpletime.arcs = arc;

	if(tw->simpletime.second_per_pixel<1)
		tw->simpletime.second_per_pixel = 1;

	if(1) {
		Widget ww = (Widget)tw;
		Widget clip = 0,scroll = 0;
		while(ww){
		  if((clip = XtParent(ww)))
		    if((scroll = XtParent(clip))) {
					if(XmIsScrolledWindow(scroll))
						break;
					else
						clip = scroll = 0;
			  }
		}

		if(clip)
		{
			while(tw->simpletime.second_per_pixel > 1 &&
				x_of(tw,tw->simpletime.end_date,tw->simpletime.end_time) < clip->core.width)
				tw->simpletime.second_per_pixel--;

			while(x_of(tw,tw->simpletime.end_date,tw->simpletime.end_time) > clip->core.width)
				tw->simpletime.second_per_pixel++;
		}
	}

	while(x_of(tw,tw->simpletime.end_date,tw->simpletime.end_time) > 64000)
	{
#if 0
		printf("Scaling too large... %d %d %d %d\n",
			tw->simpletime.second_per_pixel,
			tw->simpletime.end_date,tw->simpletime.end_time,
			x_of(tw,tw->simpletime.end_date,tw->simpletime.end_time)
		
			);
#endif
		tw->simpletime.second_per_pixel++;
	}

	for(i=0;i<tw->simplebase.count;i++)
	{
		NodeStruct  *w = tw->simplebase.nodes + i;
		if(!w->managed) continue;

		
		if(w->misc[DATE] == 0)
			w->r.x = 0;
		else
			w->r.x = x_of(tw,w->misc[DATE],w->misc[TIME]) - w->r.width/2;

		w->r.y = tw->simpletime.title +
			w->misc[ARC] * tw->simpletime.max_h +
			(tw->simpletime.max_h - w->r.height)/2;

		if(*maxWidth < w->r.x + w->r.width)
			*maxWidth = w->r.x + w->r.width;

		if(*maxHeight < w->r.y + w->r.height)
			*maxHeight = w->r.y + w->r.height;
	}

}


void TimeSetTime(Widget _w,int n,DateTime dt)
{
	SimpleTimeWidget tw = (SimpleTimeWidget)_w;
	NodeStruct *w = tw->simplebase.nodes + n;

	w->misc[DATE] = date_to_julian(dt.date);
	w->misc[TIME] = time_to_sec(dt.time);

	if(!tw->simpletime.inited)
	{
		tw->simpletime.start_date = tw->simpletime.end_date = w->misc[DATE];
		tw->simpletime.start_time = tw->simpletime.end_time = w->misc[TIME];
		tw->simpletime.inited = 1;
	}

	if(w->misc[DATE] < tw->simpletime.start_date ||
		( w->misc[DATE] == tw->simpletime.start_date && 
		  w->misc[TIME] <  tw->simpletime.start_time))
	{
		tw->simpletime.start_date = w->misc[DATE];
		tw->simpletime.start_time = w->misc[TIME];
	}

	if(w->misc[DATE] > tw->simpletime.end_date ||
		( w->misc[DATE] == tw->simpletime.end_date && 
		  w->misc[TIME]  > tw->simpletime.end_time))
	{
		tw->simpletime.end_date = w->misc[DATE];
		tw->simpletime.end_time = w->misc[TIME];
	}

#if 0
	printf("TimeSetTime %d %d %d %d\n",
		w->misc[DATE],w->misc[TIME],
		 tw->simpletime.end_date, tw->simpletime.end_time
		
		);
#endif
}

DateTime TimeGetTime(Widget _w,int n)
{
	SimpleTimeWidget tw = (SimpleTimeWidget)_w;
	NodeStruct *w = tw->simplebase.nodes + n;
	DateTime dt;

	dt.date = julian_to_date(w->misc[DATE]);
	dt.time = sec_to_time(w->misc[TIME]);

	return dt;
}

void TimeAdd(DateTime* dt,int n)
{
	int dd = date_to_julian(dt->date);
	int tt = time_to_sec(dt->time);

	tt += n;
	while(tt < 0) {
		dd++;
		tt += 24*60*60;
	}

	while(tt >= 24*60*60)
	{
		dd--;
		tt -= 24*60*60;
	}

	/* printf("TimeAdd: %d %d\n",*d,*t); */

	dt->date = julian_to_date(dd);
	dt->time = sec_to_time(tt);
	/* printf("TimeAdd: %d %d\n",*d,*t); */
}

Widget CreateTime(Widget par,char *nam,ArgList al,int ac)
{
	return XtCreateWidget(nam,simpletimeWidgetClass,par,al, ac);
}

/* 
	Make the simpletime _second_ visible
	If _second_ is equal to TIME_NOW show the current simpletime
*/

void TimeShowTime(Widget _w,DateTime dt,XEvent* ev)
{
	SimpleTimeWidget h = (SimpleTimeWidget)_w;
    XRectangle r;


    int d = date_to_julian(dt.date);
    int t = time_to_sec(dt.time);

    r.x      = x_of(h,d,t);
    r.y      = ev?ev->xbutton.y:0;
    r.width  = 1;
    r.height = 1;

    SimpleBaseShow(_w,&r,ev);
}

void TimeEventTime(Widget _w,XEvent* e,DateTime *dt)
{
	int x = e->xbutton.x;
	int d,t;
	time_of((SimpleTimeWidget)_w,x,&d,&t);
	dt->date = julian_to_date(d);
	dt->time = sec_to_time(t);
}

int TimeDiff(DateTime dt1,DateTime dt2)
{
	long long x1,x2;
	int d1 = date_to_julian(dt1.date);
	int d2 = date_to_julian(dt2.date);
	int t1 = time_to_sec(dt1.time);
	int t2 = time_to_sec(dt2.time);

	x1 = d1 * 24 * 60 * 60 + t1;
	x2 = d2 * 24 * 60 * 60 + t2;

	return x1 - x2;
}

void *TimeFindByY(Widget _w,XEvent *ev)
{
	SimpleBaseWidget w = (SimpleBaseWidget)_w;
	int i;
	for(i = 0; i < w->simplebase.count;i++)
	{
		NodeStruct *n = w->simplebase.nodes + i;
		if(n->managed)
			if(
			/* ev->xbutton.x >= n->r.x && ev->xbutton.x <= n->r.x + n->r.width && */
			    ev->xbutton.y >= n->r.y && ev->xbutton.y <= n->r.y + n->r.height)
				return n->user_data;
	}
	return NULL;
}
