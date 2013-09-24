/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #5 $                                                                    */
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

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ConstrainP.h>
#include <Xm/XmP.h>
#include <Xm/DrawingAP.h>
#include <Xm/ExtObjectP.h>
#include "SimpleTree.h"
#include "SimpleTreeP.h"

#include <Xm/ScrollBar.h>
#include <stdlib.h>

#ifndef MAX
#define   MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define   MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define LAYOUT 0

static void             Initialize();
/*static void             ConstraintInitialize();
  static Boolean          ConstraintSetValues();*/
static void             Destroy();
static void             Print();
static Boolean          SetValues();
/*static XtGeometryResult GeometryManager();
static void             ChangeManaged();
static void             insert_new_node();
static void             delete_node();*/
static void Layout(Widget,long*,long*);
static void             Redisplay();
static void compute_rect(SimpleTreeWidget,Node*,int,int,int,int,int,XRectangle*);
static void set_positions(SimpleTreeWidget,long*,long*);
/* static void change_vertical(SimpleTreeWidget tw, Node *w,Boolean v); ??? */

static XtResource resources[] = {
	{XtNhorizontalSpace,XtCSpace,XtRDimension,sizeof(Dimension),
	XtOffset(SimpleTreeWidget, simpletree.h_min_space), XtRString,"6" },
	{XtNverticalSpace,XtCSpace, XtRDimension,sizeof (Dimension),
	XtOffset(SimpleTreeWidget, simpletree.v_min_space), XtRString,"2"  },

};

SimpleTreeClassRec simpletreeClassRec = {
	{
	/* core_class fields  */
	(WidgetClass) &simplebaseClassRec,/* superclass         */
	"SimpleTree",                           /* class_name         */
	sizeof(SimpleTreeRec),                /* widget_size        */
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
	 /* XtExposeCompressMaximal, */
	True,          
	/* compress_exposure  */
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
	/* composite_class fields */
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
	NULL,                /* destroy             */
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
	/* SimpleTree class fields */
	0,                               /* ignore              */
	},
};


WidgetClass simpletreeWidgetClass = (WidgetClass) &simpletreeClassRec;

static void Initialize(SimpleTreeWidget request, SimpleTreeWidget new)
{
	if (request->core.width <= 0)
		new->core.width = 5;
	if (request->core.height <= 0)
		new->core.height = 5;

}

static void Destroy(SimpleTreeWidget w)
{
}


static Boolean SetValues(SimpleTreeWidget current, 
SimpleTreeWidget request, SimpleTreeWidget new)
{
	int  redraw = FALSE;
	long w = 0;
	long h = 0;

	if (new->simpletree.v_min_space != current->simpletree.v_min_space ||
	    new->simpletree.h_min_space != current->simpletree.h_min_space){
		Layout((Widget)new,&w,&h);
		redraw = True;
	}

	/* printf("Redraw tree %d\n",redraw); */

	return (redraw);
}


static int first_kid(SimpleTreeWidget w,Node *n)
{
	int i;
	for(i=0;i<n->kcnt;i++)
		if(KIDS(w,n,i).managed)
			return i;
	return -1;
}

static int last_kid(SimpleTreeWidget w,Node *n)
{
	int i;

	if(n->kcnt)
		for(i= n->kcnt - 1;i>=0;i--)
			if(KIDS(w,n,i).managed)
				return i;
	return -1;
}

static void line(SimpleTreeWidget w,int x1,int y1,int x2,int y2,Region region ) 
{
	int width  = x2>x1 ? x2-x1+2 : x1-x2+2;
	int height = y2>y1 ? y2-y1+2 : y1-y2+2;
	int x      = x2>x1 ? x1 : x2;
	int y      = y2>y1 ? y1 : y2;

	if(XRectInRegion(region,x,y,width,height))
	{

		XDrawLine(XtDisplay(w), XtWindow(w), w->manager.bottom_shadow_GC,
			x1,y1,x2,y2);

		XDrawLine(XtDisplay(w), XtWindow(w), w->manager.top_shadow_GC,
			x1+1,y1+1,x2+1,y2+1);
	}
}

static void Redisplay (SimpleTreeWidget w, XEvent *event, Region region)
{
	int i, j;
	int fkid;
	int lkid;

#if 0
	Node    *child;
	XEvent  ev;
	XmRegion r = (XmRegion)region;

	printf("Before: %p\n",r);
	if(r)
	for(i=0;i<r->numRects;i++)
	{
		XmRegionBox* x = &r->rects[i];
		printf("  %d-%d %d-%d\n", x->x1,x->x2,x->y1,x->y2);
	}

	while(XCheckWindowEvent(XtDisplay(w),XtWindow(w),ExposureMask,&ev))
		XtAddExposureToRegion(&ev,region);

	printf("After:\n");
	if(r)
	for(i=0;i<r->numRects;i++)
	{
		XmRegionBox* x = &r->rects[i];
		printf("  %d %d %d %d\n", x->x1,x->y1,x->x2,x->y2);
	}
#endif


	for (i = 0; i < w -> simplebase.count; i++)
	{
		Node *child = w -> simplebase.nodes + i;

		fkid = first_kid(w,child);

		if(!child->misc[LAYOUT]) /* Vertical layout */
		{

			if(fkid>=0)
			{
				line(w,
				    child->r.x + child->r.width, 
				    child->r.y + child->r.height / 2,
				    KIDS(w,child,fkid).r.x,
				    child->r.y + child->r.height / 2
				    /* KIDS(w,child,fkid).r.y +  */
				    /* KIDS(w,child,fkid).r.height/2 */
					,region);

				lkid = last_kid(w,child);

				if(lkid != fkid)
				{
					int x =  child->r.x + child->r.width + 
					(KIDS(w,child,fkid).r.x - (child->r.x + child->r.width)) / 2;

					line(w,
					    x, 
					    child->r.y + child->r.height / 2,
					    x,
					    KIDS(w,child,lkid).r.y + 
					    KIDS(w,child,lkid).r.height/2,region);

					for (j = fkid + 1 ; j < child->kcnt; j++)
						if (KIDS(w,child,j).managed)
						{

							line(
							    w,
							    x,
							    KIDS(w,child,j).r.y +
							    KIDS(w,child,j).r.height/2,
							    KIDS(w,child,j).r.x,
							    KIDS(w,child,j).r.y + 
							    KIDS(w,child,j).r.height/2,region);

						}
				}
			}
		}
		else
		{
			fkid = first_kid(w,child);

			if(fkid>=0 )
			{
				lkid = last_kid(w,child);

				line(w,
					KIDS(w,child,fkid).r.x + KIDS(w,child,fkid).r.width/2,
					child->r.y + child->r.height + w->simpletree.v_min_space/2,
					KIDS(w,child,lkid).r.x + KIDS(w,child,lkid).r.width/2, 
					child->r.y + child->r.height + w->simpletree.v_min_space/2,region);

				line(
					w,
					KIDS(w,child,fkid).r.x + KIDS(w,child,fkid).r.width/2,
					child->r.y + child->r.height,
					KIDS(w,child,fkid).r.x + KIDS(w,child,fkid).r.width/2,
					KIDS(w,child,fkid).r.y,region);

				for (j = fkid + 1; j <  child->kcnt; j++)
					if (KIDS(w,child,j).managed)
					{

						line(
						    w,
						    KIDS(w,child,j).r.x + KIDS(w,child,j).r.width/2,
							child->r.y + child->r.height + w->simpletree.v_min_space/2,
						    KIDS(w,child,j).r.x + KIDS(w,child,j).r.width/2,
						    KIDS(w,child,j).r.y,region);
					}
			}

		}

	}

	NodesRedraw((SimpleBaseWidget)w,event,region);
}



static void Layout(Widget w,long *maxWidth,long *maxHeight)
{
	SimpleTreeWidget tw = (SimpleTreeWidget)w;
	int                 high = (int)tw->simpletree.v_min_space;
	int                 i,n;
	int                 h_max = 0,w_max = 0, dh;

	*maxWidth = *maxHeight = 5;

	h_max = high;

	for(i=0;i<tw->simplebase.count;i++)
	{
		Node *w = tw->simplebase.nodes + i;
		h_max = MAX(h_max,w->r.height); /* just a try */
		w_max = MAX(w_max,w->r.width);
	}

	dh = h_max / 2;

	h_max += tw->simpletree.v_min_space;
	w_max += tw->simpletree.h_min_space;

	for(n=0;n<tw->simplebase.count;n++)
	{
		for(i=0;i<tw->simplebase.count;i++)
		{
			Node *w = tw->simplebase.nodes + i;
			if(w->managed && (w->pcnt == 0))
			{
				XRectangle r;
				compute_rect(tw,w,
					(int)tw->simpletree.h_min_space,
					(int)tw->simpletree.h_min_space,
					w->r.width,
					w->r.height, 
					w->r.height, 
					&r);
			}
		}

		set_positions(tw,maxWidth,maxHeight);
#if 0
		if(*maxWidth == 0 || *maxHeight == 0)
		{
			Boolean v = (*maxWidth == 0);
			for(i=0;i<tw->simplebase.count;i++)
			{
				Node *w = tw->simplebase.nodes + i;
				if(w->managed && (w->pcnt == 0))
					change_vertical(tw,w,!v);
			}

		}
		else 
#endif
		break;
	}
}

#if 0
static void change_vertical(SimpleTreeWidget tw, Node *w,Boolean v)
{
	int i;
	if(w->misc[LAYOUT] != v )
		w->misc[LAYOUT] = v;
	else
	{
		for(i = 0; i < w->kcnt ; i++)
			if(KIDS(tw,w,i).managed)
				change_vertical(tw,&KIDS(tw,w,i),v);
	}
}
#endif

static void union_rect(XRectangle *r1,XRectangle *r2,XRectangle *r3)
{
	int dx,dy;

	dx  = MAX(r1->x+r1->width,r2->x+r2->width);
	dy  = MAX(r1->y+r1->height,r2->y+r2->height);
	r3->x = MIN(r1->x,r2->x);
	r3->y = MIN(r1->y,r2->y);

	r3->width  = dx - r3->x;
	r3->height = dy - r3->y;

}


static void compute_rect(SimpleTreeWidget tw,Node *w,
	int x,int y,int dx,int dy,int h,XRectangle *rect)
{
	int i;
	int mx = 0;
	int my = 0;

	*rect   = w->r;
	rect->x = w->tmpx = x;
	rect->y = w->tmpy = y; /* + (h - w->r.height) / 2; */
	rect->width += tw->simpletree.h_min_space;
	rect->height += tw->simpletree.v_min_space;

	if(h > w->r.height)
		rect->y = w->tmpy = y + (h - w->r.height) / 2;

	for(i = 0; i < w->kcnt; i++)
	{
		Node *z = &KIDS(tw,w,i);
		if(z->managed)
		{
			if(z->kcnt) /* && !w->misc[LAYOUT]) */
				mx = MAX(mx,z->r.width);
			my = MAX(my,z->r.height);
		}
	}

	for(i = 0; i < w->kcnt; i++)
	{
		Node *z = &KIDS(tw,w,i);
		if(z->managed)
		{
			XRectangle r;
			if(!w->misc[LAYOUT])
			{
				compute_rect(tw,z,x + dx + tw->simpletree.h_min_space ,
					y, mx, my, w->r.height,&r);
				union_rect(rect,&r,rect);
				y += r.height;

				if(y > 60000)
				{
					y = w->tmpy;
					x += r.width;
				}
			}
			else
			{
				compute_rect(tw,z,x,y + dy
				+ tw->simpletree.v_min_space, mx, my, w->r.height,
					 &r);
				union_rect(rect,&r,rect);
				x += r.width;
			}
		}
	}

}


static void set_positions(SimpleTreeWidget tw,long *maxWidth, long *maxHeight)
{
	int       i;

	for(i=0;i<tw->simplebase.count;i++)
	{
		Node *w = tw->simplebase.nodes + i;

		if(w->managed)
		{
#if 0
			if(w->tmpx != (Position)w->tmpx)
			{
				*maxWidth = 0;
				return;
			}

			if(w->tmpy != (Position)w->tmpy)
			{
				*maxHeight = 0;
				return;
			}
#endif


#if 0
			w->r.x = w->tmpx % 64000    + (w->tmpy / 64000) * (w->r.width  + 20);
			w->r.y = w->tmpy % 64000    + (w->tmpx / 64000) * (w->r.height + 20);
#endif

			w->r.x = w->tmpx;
			w->r.y = w->tmpy;

			*maxWidth = MAX(*maxWidth,
			    w->r.x + w->r.width + tw->simpletree.h_min_space);
			*maxHeight = MAX(*maxHeight,
			    w->r.y + w->r.height + tw->simpletree.v_min_space);
		}
	}
}

void NodeTreeFlip(Widget _w,int node)
{
    SimpleTreeWidget w = (SimpleTreeWidget)_w;
	Node *p = w->simplebase.nodes + node;
	if( node < 0 || node >= w->simplebase.count) return;
	p->misc[LAYOUT] = !p->misc[LAYOUT];
	NodeNewSize(_w,node);
}

Widget CreateTree(Widget par,char *nam,ArgList al,int ac)
{
	return XtCreateWidget(nam,simpletreeWidgetClass,par,al, ac);
}

static void Print (SimpleTreeWidget w, FILE *f)
{
}
