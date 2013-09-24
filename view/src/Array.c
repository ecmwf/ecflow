
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #6 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2012 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */

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
#include      "Array.h"
#include      "ArrayP.h"

#ifndef MAX
#define   MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define   MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

static void             Initialize();
static void             Resize();
static void             Destroy();
static Boolean          SetValues();
static XtGeometryResult GeometryManager();
static void             ChangeManaged();
static void             new_layout(ArrayWidget,Boolean,Boolean);
static void 			get_row_col(ArrayWidget,int*,int*);

//#define DEBUG(a) printf("%s\n",a);

static XtResource resources[] = {
	{XtNrows,XtCRowCol,XtRInt,sizeof(int),
	XtOffset(ArrayWidget,array.rows), XtRImmediate,(XtPointer)0 },
	{XtNcolumns,XtCRowCol, XtRInt,sizeof(int),
	XtOffset(ArrayWidget,array.cols), XtRImmediate,(XtPointer)0 },
	{XtNround,XtCRound, XtRInt,sizeof(int),
	XtOffset(ArrayWidget,array.round), XtRImmediate,(XtPointer)0 },
};



ArrayClassRec arrayClassRec = {
	{
	/* core_class fields  */
#ifdef USE_MANAGER
	(WidgetClass) &xmDrawingAreaClassRec,/* superclass         */
#else
	(WidgetClass) &constraintClassRec,/* superclass         */
#endif
	"Array",                           /* class_name         */
	sizeof(ArrayRec),                /* widget_size        */
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
	Resize,                           /* resize             */
	(XtExposeProc)_XmRedisplayGadgets,/* expose             */
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
	/* Array class fields */
	0,                               /* ignore              */
	},
};


WidgetClass arrayWidgetClass = (WidgetClass) &arrayClassRec;

static void Initialize(request, new)
ArrayWidget request, new;
{
	/*
   * Make sure the widget's width and height are 
   * greater than zero.
   */
	if (request->core.width <= 0)
		new->core.width = 5;
	if (request->core.height <= 0)
		new->core.height = 5;

}

static void Destroy(w)
ArrayWidget w;
{
}

static void Resize(w)
ArrayWidget w;
{
	new_layout(w,TRUE,FALSE);
}


static Boolean SetValues(current, request, new)
ArrayWidget current, request, new;
{
	if((current->array.cols != new->array.cols ) && new->array.cols)
		new->array.rows = 0;
	if((current->array.rows != new->array.rows ) && new->array.rows)
		new->array.cols = 0;

	new_layout(new,TRUE,TRUE);
	return (False);
}

static XtGeometryResult GeometryManager(w, request, reply)
Widget               w;
XtWidgetGeometry    *request;
XtWidgetGeometry    *reply;
{
	new_layout((ArrayWidget)XtParent(w),FALSE,FALSE);
	return (XtGeometryYes);
}

static void get_row_col(tw,row,col)
ArrayWidget tw;
int *row;
int *col;
{
	int i,n=0;
	int nx,ny;

	for(i=0;i<tw->composite.num_children;i++)
	{
		Widget w = tw->composite.children[i];
		if(XtIsManaged(w)) n++;
	}

	*row = *col = 0;

	if(!n) return;

	if(tw->array.rows == 0 && tw->array.cols == 0)
	{
		ny = (int)(sqrt((double)n)+0.5);
		nx = n / ny;
		if(nx*ny < n) nx++;
	}

	if(tw->array.rows)
	{
		ny = MIN(tw->array.rows,n);
		nx = n / ny;
		if(nx*ny < n) nx++;
	}

	if(tw->array.cols)
	{
		nx = MIN(tw->array.cols,n);
		ny = n / nx;
		if(nx*ny < n) ny++;
	}

	*col = nx;
	*row = ny;


}

static void ChangeManaged(tw)
ArrayWidget tw;
{
	new_layout(tw,FALSE,TRUE);
	new_layout(tw,TRUE,TRUE);
}


static void new_layout(ArrayWidget tw,Boolean move_children,Boolean geometry)
{
	Dimension width,height;
	Dimension ww=0,wh=0,oww=0,owh=0;
	int i;
	int cnt = 20;

	if(tw->array.round)
	{
		int m = 0, n = 0;
		int xmarg = 0;
		int ymarg = 0;
		int radius;
		Boolean more = True;

		for(i=0;i<tw->composite.num_children;i++)
		{
			Widget w = tw->composite.children[i];
			if(XtIsManaged(w)) {
				m++;
				xmarg = MAX(xmarg,w->core.width);
				ymarg = MAX(ymarg,w->core.height);
			}
		}

		xmarg++;
		ymarg++;
		xmarg /= 2;
		ymarg /= 2;

		width  =  5;
		height =  5;

		radius = MIN(tw->core.width,tw->core.height) / 2 - MAX(xmarg,ymarg) - 1;

		while(more && (cnt-- >0))
		{
			int ox = 0,oy = 0;
			more = False;	

			for(i=0;i<tw->composite.num_children;i++)
			{
				Widget w = tw->composite.children[i];
				if(XtIsManaged(w))
				{
					int x,y;

					wh = w->core.height;
					ww = w->core.width;

					x = xmarg + radius + ROUND(radius*sin(RADIANS(360.0/m*n)))  -ww/2;
					y = ymarg + radius + ROUND(radius*cos(RADIANS(180+360.0/m*n)))  -wh/2;

					_XmConfigureObject((Widget)w,x,y,ww,wh,0);

					if(n)
					{
						if ( ! ( (x + ww < ox || y + wh < oy) || (x > ox + oww || y > oy + owh) ))
							more = True;

							/* printf("%d %d %d %d %d %d %d %d %d %d\n",x,y,ox,oy,ww,wh,oww,owh,more,cnt); */
					}
					ox = x;oy = y; oww = ww; owh = wh;

					n++;

					width  = MAX(width,x+ww);
					height = MAX(height,x+wh);
				}
			}

			radius +=  MAX(xmarg,ymarg);
			/* more = 0; */
		}
	}
	else
	{

		Dimension w = tw->core.width;
		Dimension h = tw->core.height;
		int       m=0,nx,ny;
		Position  wx,wy,dx,dy;

		wx = 0;
		wy = 0;
		m = 0;

		get_row_col(tw,&ny,&nx);

		if(ny == 0 || nx == 0) return;


		ww = w / (Dimension) nx;
		wh = h / (Dimension) ny;

		if(wh<5) wh = 5;
		if(ww<5) ww = 5;
		dx = ww;
		dy = wh;

		if(!move_children)
			for(i=0;i<tw->composite.num_children;i++)
			{

				Widget w = tw->composite.children[i];
				if(XtIsManaged(w))
				{
					dx = MAX(dx,(Position)w->core.width);
					dy = MAX(dy,(Position)w->core.height);
				}
			}

		if(move_children)
			for(i=0;i<tw->composite.num_children;i++)
			{

				Widget w = tw->composite.children[i];
				if(XtIsManaged(w))
				{
					m ++;
					_XmConfigureObject((Widget)w,wx,wy,ww,wh,0);

					wx += dx;
					if(m % nx == 0) {
						wx = 0;
						wy += dy;
					}
				}
			}
		width  = dx*nx;
		height = dy*ny;
	}


	if(geometry)
	if(tw->core.width != width || tw->core.height != height)
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


}


Widget CreateArray(par,nam,al,ac)
Widget par;
char  *nam;
Arg   *al;
int   ac;
{
	return   XtCreateWidget(nam,arrayWidgetClass,par,al,ac);
}
