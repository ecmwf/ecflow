/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      : B.Raoult                                                                      */
/* Revision    : $Revision: #5 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2012 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description : Hyper text like widget.                                                       */
/*=============================================================================================*/

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/cursorfont.h>
#include "Hyper.h"
#include "HyperP.h"
#include <Xm/ScrollBar.h>
#include <stdlib.h>
#include <string.h> /* strerror */

extern void xec_compile(char *w);
extern int xec_step(char *p);

#ifndef ABS
#define ABS(a)           ((a)>=0?(a):-(a))
#endif
#ifndef MIN
#define MIN(a,b)         ((a)>(b)?(b):(a))
#endif

#define ESIZE 1024

#define NORMAL           0
#define HIGHLIGHT        1
#define NEWLINE          2

#define MAX_LINE_SIZE    1024

extern char *xec_loc1, *xec_loc2;

/* 
  Private functions 
*/

static void hilite(HyperWidget w,Boolean on);
static text_segment *find_segment(HyperWidget,int,int);
static void    free_text(text_segment*);
static void    create_gcs(HyperWidget);
static void    create_new_text(HyperWidget);
static void    xselect();
static void    cursor();
static void    activate();
static void    xincrement();
static void    add_to_text ( HyperWidget,char*,int,int);
static void    calc_new_size (HyperWidget);
static void    zoom_open (HyperWidget,text_segment*);
static void    show_selection(HyperWidget);
static void    set_selection(HyperWidget);
static void    clear_selection(HyperWidget);
static void lowcase(char *p);
static void find_visible_part(Widget,Position*,Position*,Dimension*,Dimension*);
static void set_text(HyperWidget,char (*)(XtPointer),XtPointer);

/*
  Widget class methods
*/

static void    Initialize();
static void    Redisplay();
static void    Resize();
static void    Destroy();
static Boolean SetValues();

static char defaultTranslations[] = 
"     <Btn1Down>:select()\n                 <Btn1Up>: activate()\n\
      <Motion>:cursor()\n\
      <Btn2Down>:select()\n                 <Btn2Up>: activate() \n\
 Shift<Btn5Down>: increment(1)\n       Shift<Btn4Down>: increment(-1)  \n\
      <Btn5Down>: increment(10)\n           <Btn4Down>: increment(-10) \n";

static XtActionsRec actionsList[] = {
    { "select",   (XtActionProc) xselect},
    { "activate", (XtActionProc) activate},
    { "cursor",   (XtActionProc) cursor},
    { "increment",(XtActionProc) xincrement},
};

static XtResource resources[] = {

    {XtNhighlightFont, XtCFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset(HyperWidget, hyper.highlight_font), XtRString, "fixed"},

    {XtNnormalFont, XtCFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset(HyperWidget, hyper.normal_font), XtRString, "fixed"},

    {XtNhighlightColor, XtCColor, XtRPixel, sizeof (Pixel),
    XtOffset(HyperWidget, hyper.highlight_color),XtRString, "Red"},

    {XtNselectColor, XtCColor, XtRPixel, sizeof (Pixel),
    XtOffset(HyperWidget, hyper.select_color),XtRString, "Blue"},

    {XtNnormalColor, XtCColor, XtRPixel, sizeof (Pixel),
    XtOffset(HyperWidget, hyper.normal_color),XtRString,"Black"},

    {XtNactivateCallback,XtCCallback,XtRCallback,sizeof(caddr_t),
    XtOffset (HyperWidget, hyper.activate),XtRCallback,NULL},

    {XtNzoomEffect,XtCZoom,XtRBoolean,sizeof(Boolean),
    XtOffset (HyperWidget, hyper.zoom),XtRImmediate,(XtPointer)TRUE},

#ifndef XDESIGNER
    {XtNstartHighlight,XtCTagChar,XtRUnsignedChar,sizeof(unsigned char),
    XtOffset(HyperWidget,hyper.start_of_highlight),XtRImmediate,
    (XtPointer)'{'},

    {XtNendHighlight,XtCTagChar,XtRUnsignedChar,sizeof(unsigned char),
    XtOffset (HyperWidget, hyper.end_of_highlight),XtRImmediate,
    (XtPointer)'}'},
#endif

    {XtNzoomSpeed,XtCZoomSpeed,XtRInt,sizeof(int),
    XtOffset (HyperWidget, hyper.speed),XtRImmediate,(XtPointer)4},

    {XtNmargin,XtCMargin,XtRInt,sizeof(int),
    XtOffset (HyperWidget, hyper.margin),XtRImmediate,(XtPointer)10},

};

/*---------------------------------------------------------------*/
/* Static initialisation of the class record                     */
/*---------------------------------------------------------------*/

HyperClassRec  hyperClassRec = {
    {
#ifdef MOTIF
    (WidgetClass) &xmPrimitiveClassRec,  /* superclass            */
#else
    (WidgetClass) &widgetClassRec,       /* superclass            */
#endif
    "Hyper",                             /* class_name            */
    sizeof(HyperRec),                    /* widget_size           */
    NULL,                                /* class_initialize      */
    NULL,                                /* class_part_initialize */
    FALSE,                               /* class_inited          */
    Initialize,                          /* initialize            */
    NULL,                                /* initialize_hook       */
    XtInheritRealize,                    /* realize               */
    actionsList,                         /* actions               */
    XtNumber(actionsList),               /* num_actions           */
    resources,                           /* resources             */
    XtNumber(resources),                 /* num_resources         */
    NULLQUARK,                           /* xrm_class             */
    TRUE,                                /* compress_motion       */
    XtExposeCompressMaximal,             /* compress_exposure     */
    TRUE,                                /* compress_enterleave   */
    TRUE,                                /* visible_interest      */
    Destroy,                             /* destroy               */
    Resize,                              /* resize                */
    Redisplay,                           /* expose                */
    SetValues,                           /* set_values            */
    NULL,                                /* set_values_hook       */
    XtInheritSetValuesAlmost,            /* set_values_almost     */
    NULL,                                /* get_values_hook       */
    NULL,                                /* accept_focus          */
    XtVersion,                           /* version               */
    NULL,                                /* callback private      */
    defaultTranslations,                 /* tm_table              */
    NULL,                                /* query_geometry        */
    NULL,                                /* display_accelerator   */
    NULL,                                /* extension             */
    },
#ifdef MOTIF
    {
    (XtWidgetProc)_XtInherit,             /* border_highlight      */
    (XtWidgetProc)_XtInherit,             /* border_unhighligh     */
    XtInheritTranslations,                /* translations          */
/*** SOS!!!! Following line was replaced ...... */
/*    (XtWidgetProc)_XtInherit,          arm_and_activate      */
#if (XmVersion == 1001)
    (XmArmAndActivate)_XtInherit,         /* arm_and_activate      */
#else
    (XtActionProc)_XtInherit,         /* arm_and_activate      */
#endif
    NULL,                                 /* syn_resources         */
    0,                                    /* num_syn_resources     */
    NULL,                                 /* extension             */
    },
#endif
    {
    0,                                    /* ignore                */
    }
};









WidgetClass hyperWidgetClass = (WidgetClass) &hyperClassRec;

/*---------------------------------------------------------------*/
/* Create the two GCs needed                                     */
/*---------------------------------------------------------------*/

static void create_gcs(w)
HyperWidget w;
{
    XGCValues values;
    XtGCMask  valueMask;

    valueMask = GCForeground | GCBackground | GCFont;

    values.background = w->core.background_pixel;

    values.foreground = w->hyper.highlight_color;
    values.font       = w->hyper.highlight_font->fid;
    w->hyper.highlight_gc = XtGetGC((Widget)w, valueMask, &values);

    values.foreground = w->hyper.select_color;
    w->hyper.select_gc = XtGetGC((Widget)w, valueMask, &values);

    values.foreground = w->hyper.normal_color;
    values.font       = w->hyper.normal_font->fid;
    w->hyper.normal_gc = XtGetGC((Widget)w, valueMask, &values);



    valueMask = GCBackground|GCForeground|GCFunction|GCGraphicsExposures;

    values.background         = 0;
    values.foreground         = w->hyper.normal_color ^ w->core.background_pixel;
    values.graphics_exposures = False;
    values.function           = GXxor;

    w->hyper.xor_gc = XtGetGC((Widget)w, valueMask, &values);


}

/*--------------------------------------------------------------*/
/* Initialize: Create the GCs                                   */
/*--------------------------------------------------------------*/

static void Initialize (request, new)
HyperWidget request, new;
{
    /* Check the size of the widget */

    if (request->core.width == 0)
        new->core.width = 100;
    if (request->core.height == 0)
        new->core.height = 100;


    /* Create the GCs */

    create_gcs(new);

    /* No text yet */

    new->hyper.first_seg     = new->hyper.last_selected 
        = new->hyper.last_cursor =  NULL;
    new->hyper.hand     = XCreateFontCursor(XtDisplay(new),XC_hand2);

    /* Nothing found */

    new->hyper.grep_seg = NULL;
    new->hyper.grep_txt = NULL;
    new->hyper.grep_len = 0;
    new->hyper.grep_off = 0;

}

/*--------------------------------------------------------------*/
/* Free all memory allocated for the text segments              */
/*--------------------------------------------------------------*/

static void free_text(s)
text_segment *s;
{

    while(s)
    {
        text_segment *p=s->next;
        if(s->text) XtFree((XtPointer)s->text);
        XtFree((XtPointer)s);
        s = p;
    }

}

/*--------------------------------------------------------------*/
/* Destroy the widget: release all memory alocated              */
/*--------------------------------------------------------------*/

static void Destroy (w)
HyperWidget w;
{
    free_text(w->hyper.first_seg);
    XtReleaseGC((Widget)w, w->hyper.normal_gc);
    XtReleaseGC((Widget)w, w->hyper.highlight_gc);
    XtReleaseGC((Widget)w, w->hyper.xor_gc);
    XtReleaseGC((Widget)w, w->hyper.select_gc);
    XtRemoveAllCallbacks ((Widget)w,XtNactivateCallback);
}

/*--------------------------------------------------------------*/
/* Resize : not implemented                                     */
/*--------------------------------------------------------------*/


static void Resize (w)
HyperWidget w;
{
    /* 
       For futur implementation
       May be for text warp ...
    */
}

/*--------------------------------------------------------------*/
/* Redisplay : redraw the text                                  */
/*--------------------------------------------------------------*/


static void Redisplay (w, event, region)
HyperWidget  w;
XEvent       *event;
Region        region;
{

    if(w->core.visible)
    {
        text_segment *s = w->hyper.first_seg;
        int x = w->hyper.margin;
        int y = 0;
        Boolean newline = TRUE;

        while(s)
        {

            /* change line on new lines */

            if(newline)
            {
                x = w->hyper.margin;
                y += s->height;
            }

            /* redraw only what is needed */

            if(XRectInRegion(region,x,y-s->height+s->desc,s->width,s->height)
                != RectangleOut)
            {

                XDrawImageString(XtDisplay (w), XtWindow (w), 
                    s->gc,
                    x,
                    y,
                    s->text,
                    s->length);

				if(s->type == HIGHLIGHT)
				{
					XDrawLine(XtDisplay (w), XtWindow (w), 
						s->gc,  
						x,
						y+1,
						x+s->width,
						y+1);
				}
            }

            x += s->width;

            newline = (s->type == NEWLINE);

            s = s->next;
        }


        if(w->hyper.grep_seg)
        {
            if(XRectInRegion(region,
                w->hyper.grep_x,
                w->hyper.grep_y,
                w->hyper.grep_width,
                w->hyper.grep_height) != RectangleOut)

                XFillRectangle(XtDisplay(w),XtWindow(w),
                    w->hyper.xor_gc,
                    w->hyper.grep_x,
                    w->hyper.grep_y,
                    w->hyper.grep_width,
                    w->hyper.grep_height);

        }
    }
}

/*------------------------------------------------------------------*/
/* SetValues : redraw only for font or color changes                */
/*------------------------------------------------------------------*/

static Boolean SetValues (current, request, new)
HyperWidget current, request, new;
{
    Boolean    redraw = FALSE;

#define HAS_CHANGED(a)    (new->a != current->a)

    if(
        HAS_CHANGED(core.background_pixel) ||
        HAS_CHANGED(hyper.select_color)    ||
        HAS_CHANGED(hyper.highlight_color) ||
        HAS_CHANGED(hyper.highlight_font)  ||
        HAS_CHANGED(hyper.normal_color)    ||
        HAS_CHANGED(hyper.normal_font)
        )
    {

        XtReleaseGC((Widget)new, new->hyper.normal_gc);
        XtReleaseGC((Widget)new, new->hyper.highlight_gc);
        XtReleaseGC((Widget)new, new->hyper.xor_gc);
        XtReleaseGC((Widget)new, new->hyper.select_gc);
        create_gcs(new);

        /* rebuild text */
/*
        if(HAS_CHANGED(hyper.normal_font) || 
            HAS_CHANGED(hyper.highlight_font))
			*/
            create_new_text(new);

        redraw = TRUE;
    }

    return (redraw);

#undef HAS_CHANGED

}

/*------------------------------------------------------------------*/
/* Calculate the size of the widget                                 */
/*------------------------------------------------------------------*/

static void calc_new_size (w)
HyperWidget  w;
{
    text_segment       *s = w->hyper.first_seg;
    int                 x = w->hyper.margin;
    int                 y = 0;
    int                 last_height = 0;
    Boolean             newline = TRUE;
    Dimension           maxWidth = w->hyper.margin;
    Dimension           maxHeight = w->hyper.margin;
    XtGeometryResult    result;
    Dimension           replyWidth = 0, replyHeight = 0;

    /* Get the size of the widget */

    while(s)
    {
        if(newline)
        {
            if (x > (int) maxWidth) maxWidth=x;
            x = w->hyper.margin;
            y += s->height;
            if(y > (int) maxHeight) maxHeight=y;

        }

        s->x = x;
        s->y = y - s->height;

        x += s->width;

        newline = (s->type == NEWLINE);
        last_height = s->height;

        s = s->next;
    }

    x+= w->hyper.margin;
    y+= last_height;

    if((Dimension) x > maxWidth ) maxWidth=x;
    if((Dimension) y > maxHeight) maxHeight=y;

    /* 
    Tell our parent we want a new size 
    */

    if(w->core.width != maxWidth || w->core.height != maxHeight)
    {
        result = XtMakeResizeRequest((Widget)w,maxWidth,maxHeight, 
            &replyWidth, &replyHeight) ;

        if (result == XtGeometryAlmost)
            XtMakeResizeRequest ((Widget)w, replyWidth, replyHeight,NULL, NULL);

    }
}

/*-----------------------------------------------------------------------*/
/* Find the "visible" part of a widget as the intersection of all the    */
/* windows of it's parents' windows                                      */
/*-----------------------------------------------------------------------*/

static void find_visible_part(w,x,y,width,height)
Widget    w;
Position  *x;
Position  *y;
Dimension *width;
Dimension *height;
{
    Position root_x,root_y;
    Widget   p = w;

    *width  = w->core.width;
    *height = w->core.height;
    XtTranslateCoords(w,0,0,&root_x,&root_y);

    *x = 0;
    *y = 0;

    while((p = XtParent(p)))
    {
        Position  rx,ry;
        Dimension w,h;

        /* 
           make all computations in the root's
           coordinate system
        */

        XtTranslateCoords(p,0,0,&rx,&ry);

        w = p->core.width;
        h = p->core.height;

        /* 
            use the smallest rectangle
        */

        if(w < *width)  *width  = w;
        if(h < *height) *height = h;

        if(rx>root_x) root_x = rx;
        if(ry>root_y) root_y = ry;

        /* stop when reach a shell,
          don't go to top level shell */
        if(XtIsShell(p)) break;
    }

    /* Back to the widget's coordinate system */

    XtTranslateCoords(w,0,0,x,y);
    *x = root_x - *x;
    *y = root_y - *y;


}

/*-----------------------------------------------------------------------*/
/* Do a "zoom" effect animation, from the selected text segment to the  */
/* visible part of the widget                                            */
/*-----------------------------------------------------------------------*/

static void zoom_open(w,s)
HyperWidget   w;
text_segment *s;
{
    int dx1,dx2,dy1,dy2;

    Position x ;
    Position y ;
    Dimension width  ;
    Dimension height ;

    /* selected rectangle */

    Position  xs = s->x;
    Position  ys = s->y;
    Dimension ws = s->width;
    Dimension hs = s->height;


    /* get the rectangle we want to zoom to */

    find_visible_part((Widget)w,&x,&y,&width,&height);

    /* make sure selected rectangle in visible */

    if(xs<x) xs = x;
    if(ys<y) ys = y;
    if((Dimension)(xs+ws) > (Dimension)(x+width))  ws = x+width-xs;
    if((Dimension)(ys+hs) > (Dimension)(y+height)) hs = y+height-ys;

    /* get the offsets in each directions */

    dx1 = x-xs;
    dy1 = y-ys;
    dx2 = ((x+width)-(xs+ws));
    dy2 = ((y+height)-(ys+hs));

    /* in the rectangles are differents */

    if(dx1 || dy1 || dx2 || dy2)
    {
        int min = 32000; /* <-- Can be buggy */

        /* 
          work in "left,top,bottom,right" rectangles (Mac)
          rather than "x,y,width,height" (X)
          It's easier for the animation 
        */

        int xws = xs+ws;
        int yhs = ys+hs;

        /* Get smallest non-null offset */

        if(dx1) min = MIN(min,ABS(dx1));
        if(dx2) min = MIN(min,ABS(dx2));
        if(dy1) min = MIN(min,ABS(dy1));
        if(dy2) min = MIN(min,ABS(dy2));

        /* Scale offsets so minimun offset is 1 pixel */

        dx1 /= min;
        dx2 /= min;
        dy1 /= min;
        dy2 /= min;

        /* Use speed .. */

        dx1 *= w->hyper.speed;
        dx2 *= w->hyper.speed;
        dy1 *= w->hyper.speed;
        dy2 *= w->hyper.speed;

        /* Animate */

        while(min--)
        {
            XDrawRectangle(XtDisplay(w),XtWindow(w),
                w->hyper.xor_gc,xs,ys,xws-xs,yhs-ys);

            /* Needed, otherwise X calls are buffered */
            XSync(XtDisplay(w),False);

            XDrawRectangle(XtDisplay(w),XtWindow(w),
                w->hyper.xor_gc,xs,ys,xws-xs,yhs-ys);

            xs += dx1;
            ys += dy1;

            xws += dx2;
            yhs += dy2;

        }
    }

}

/*----------------------------------------------------------------------*/
/* Find the text segment at point (x,y)                                 */
/*----------------------------------------------------------------------*/
static text_segment *find_segment(w,x,y)
HyperWidget w;
int x,y;
{
    text_segment *s = w->hyper.first_seg;

    while(s)
    {
        if( s->type == HIGHLIGHT &&
            x >= s->x &&
            y >= s->y &&
            (Dimension) x <= (Dimension) (s->x + s->width) &&
            (Dimension) y <= (Dimension) (s->y + s->height) 
            )
            return s;
        s = s->next;
    }

	return NULL;
}

/*----------------------------------------------------------------------*/
/* highlight text under cursor                                          */
/*----------------------------------------------------------------------*/
static void hilite(HyperWidget w,Boolean on)
{

    text_segment *s = w->hyper.last_selected;

    if(s)
        XDrawImageString(XtDisplay (w), XtWindow (w),
            on?w->hyper.select_gc:s->gc,
            s->x,
            s->y+s->height,
            s->text, s->length);

}

/*-----------------------------------------------------------------------*/
/* Check for mouse down                                                  */
/*-----------------------------------------------------------------------*/

static void xselect (w, event, args, n_args)
HyperWidget   w;
XEvent        *event;
char          *args[];
int            n_args;
{
    text_segment *s;

    /* 
       Find if the used clicked in an 
       highlighted text 
    */

    if((s = w->hyper.last_selected = find_segment(w,event->xbutton.x,event->xbutton.y)))
        hilite(w,TRUE);
}

/*-----------------------------------------------------------------------*/
/* Check for mouse up                                                    */
/*-----------------------------------------------------------------------*/

static void activate (w, event, args, n_args)
HyperWidget   w;
XEvent        *event;
char          *args[];
int            n_args;
{
    hyperCallbackStruct cb;
    text_segment *s;

    /* 
       Find if the user clicked in an 
       highlighted text 
    */

    if((s = find_segment(w,event->xbutton.x,event->xbutton.y))
        && (s == w->hyper.last_selected))
    {
        hilite(w,FALSE);

        /* zoom if required */

        if(w->hyper.zoom) zoom_open(w,s);

        /* Fill callback struct */

        cb.text     = s->text;
        cb.length   = s->length;
        cb.reason   = HYPER_REASON;
        cb.event    = event;

        /* call callbacks */

        XtCallCallbacks((Widget)w, XtNactivateCallback, (XtPointer)&cb);
    }
    w->hyper.last_selected = NULL;
}

/*-----------------------------------------------------------------------*/
/* Check for mouse moves                                                 */
/*-----------------------------------------------------------------------*/

static void cursor (w, event, args, n_args)
HyperWidget   w;
XEvent        *event;
char          *args[];
int            n_args;
{

    text_segment *s;

    s = find_segment(w,event->xbutton.x,event->xbutton.y);

    if(s != w->hyper.last_cursor)
    {
        if(s)
            XDefineCursor(XtDisplay(w),XtWindow(w),w->hyper.hand);
        else
            XUndefineCursor(XtDisplay(w),XtWindow(w));
        hilite(w,s == w->hyper.last_selected);
        w->hyper.last_cursor = s;
    }

}

static void xincrement (h, event, args, n_args)
HyperWidget   h;
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
  Position    dh=0;

  Arg al[5];

  /* https://software.ecmwf.int/issues/browse/SUP-646 */

  printf("## mouse 1\n");
  if(!clip) return;
  swin = XtParent(clip);

  printf("## mouse 2\n");
  /* if(!swin || !XmIsScrolledWindow(swin)) return; */
  if(!swin) return;

  printf("## mouse 3\n");
  /* 20131126 if (n_args != 1) return; */
  SetArg(XmNverticalScrollBar  , &v_scroll);
  GetValues(swin);
  
  {	
    Position        x_parent,y_parent;	Position x,y;
    int min= 0, max= 80, value= 0, slider_size = 80, inc = 10, page_inc = 100;
    int arg = atoi(args[0]);
    /* SetArg(XmNminimum,&min);
    SetArg(XmNmaximum,&max);    
    SetArg(XmNvalue,&value);    
    SetArg(XmNsliderSize,&slider_size);    
    SetArg(XmNincrement,&inc);  */   
    /* SetArg(XmNpageIncrement,&page_inc);     ??? */ 
    ac = 0;
    XtSetArg(al[ac],XmNverticalScrollBar, &v_scroll );ac++;
    XtGetValues(swin,al,ac);
  
    ac = 0;
    XtSetArg(al[ac], XmNminimum,&min); ac++;
    XtSetArg(al[ac], XmNmaximum,&max); ac++;   
    XtGetValues(v_scroll, al, ac);
    XmScrollBarGetValues(v_scroll,&value,&slider_size,&inc,&page_inc);

    ac = 0;
    XtSetArg(al[ac],XmNx,&x_parent);ac++;
    XtSetArg(al[ac],XmNy,&y_parent);ac++;
    XtGetValues(swin,al,ac);

    /* GetValues(v_scroll);
    XmScrollBarGetValues(v_scroll, value, slider_size, inc, page_inc); */
    
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
    ac = 0;
    XtSetArg(al[ac],XmNx,x);ac++;
    XtSetArg(al[ac],XmNy,y);ac++;
    XtSetValues(swin,al,ac);
    XmScrollBarSetValues(v_scroll,value,slider_size, inc, page_inc,TRUE);
  }
#endif
}

/*-----------------------------------------------------------------------*/
/* Add a new text segment to the text                                    */
/*-----------------------------------------------------------------------*/
/* static void add_to_text(w,word,type) */
static void add_to_text(w,word,type,offset)	/* add offset */
HyperWidget w;
char *word;
int  type;
int offset;             /* add offset */
{
    text_segment *s = XtNew(text_segment);
    XCharStruct   char_info;
    int dir,ascent,desc;
    text_segment *p,*q;

    s->next = NULL;
    s->text = (word?XtNewString(word):NULL);
    s->type = type;
    s->gc   = (type == HIGHLIGHT ? w->hyper.highlight_gc : w->hyper.normal_gc);
    s->x    = s->y = s->width = s->height = 0;
    s->length = (word?strlen(word):0);
	if(offset>=0) s->offset = offset;           /* add offset */

    XTextExtents(
        (type == HIGHLIGHT ? w->hyper.highlight_font : w->hyper.normal_font),
        word,
        s->length,
        &dir,&ascent,&desc,&char_info);

    s->height = ascent + desc;
    s->desc   = desc;
    s->width  = char_info.width;

    if((p = w->hyper.first_seg))
    {
        while(p)
        {
            q=p;
            p=p->next;
        }
        q->next = s;
    }
    else w->hyper.first_seg = s;
}

/*-----------------------------------------------------------------------*/
/* Rebuild the text structure. Called when the font changes              */
/*-----------------------------------------------------------------------*/

static void create_new_text(w)
HyperWidget   w;
{
    text_segment *s = w->hyper.first_seg;

    w->hyper.first_seg = w->hyper.last_selected = w->hyper.last_cursor = NULL;

    while(s)
    {
        /* add_to_text(w,s->text,s->type); */
        add_to_text(w,s->text,s->type, -1);	/*don't update offset */
        s = s->next;
    }
    free_text(s);
    calc_new_size(w);
}

/*-----------------------------------------------------------------------*/
/* Build the text. Gets the chars from the funtion "get_next_char"       */
/* using "data" as a parameter                                           */
/*-----------------------------------------------------------------------*/

static void set_text(w,get_next_char,data)
HyperWidget   w;
char (*get_next_char)(XtPointer);
XtPointer data;
{
    char word[MAX_LINE_SIZE];
    int  i = 0;
    char soh = w->hyper.start_of_highlight;
    char eoh = w->hyper.end_of_highlight;
    char c;
    int  mode = NORMAL;
	int offset = 0;		/* add offset */

    free_text(w->hyper.first_seg);
    w->hyper.first_seg = w->hyper.last_selected = w->hyper.last_cursor = NULL;
    w->hyper.grep_seg = NULL;
    w->hyper.grep_txt = NULL;
    w->hyper.grep_len = 0;
    w->hyper.grep_off = 0;

    while((c = (get_next_char)(&data)))
    {
        /* New line */
      if(c == '\n')
        {
	  word[i]=0;
	  /* if(i) add_to_text(w,word,mode); */
	  if(i) {				/* add offset */
	    add_to_text(w,word,mode,offset);	/* add offset */
	    offset+=i;                          /* increment offset */
	  }					/* add offset */
	  /* add_to_text(w,NULL,NEWLINE); */
	  add_to_text(w,NULL,NEWLINE, offset);	/* add offset */
	  i = 0;
        }
      
        /* Start of highlight */

      else if(c == soh)
        {
	  word[i]=0;
	  /* if(i) add_to_text(w,word,mode); */
	  if(i) {									/* add offset */
	    add_to_text(w,word,mode,offset);	/* add offset */
	    offset += i;                       /* increment offset */
	  }										/* add offset */
	  mode = HIGHLIGHT;
	  i = 0;
        }
      
      /* End of highlight */
      
      else if(c == eoh)
        {
	  word[i]=0;
	  /* if(i) add_to_text(w,word,mode); */
	  if(i) {						/* add offset */
	    add_to_text(w,word,mode,offset); /* add offset */
	    offset += i+2;         /* increment offset, 2 to iclude tags */
	  }							/* add offset */
	  mode = NORMAL;
	  i = 0;
        }
      else 
        {
	  if(c=='\t') c = ' ';
	  word[i++] = c;
	  if(i==MAX_LINE_SIZE)
            {
	      word[--i]=0;
	      /* add_to_text(w,word,mode); */
	      add_to_text(w,word,mode,offset);	/* add offset */
	      i=0;
	      word[i++] = c;
            }
        }
    }

    /* flush .. */

    if(i)
    {
        word[i]=0;
        add_to_text(w,word,mode,offset);
    }

    calc_new_size(w);

	/* br advised, vk realized 02 Jun 94 */
    if(XtIsRealized((Widget) w))
	    XClearArea(XtDisplay(w),XtWindow(w),0,0,0,0,True);

}

/*-----------------------------------------------------------------------*/
/* Create a new HyperWidget                                              */
/*-----------------------------------------------------------------------*/

Widget CreateHyper(parent,name,al,ac)
Widget parent;
char   *name;
ArgList al;
int     ac;
{
    return XtCreateWidget(name,hyperWidgetClass,parent,al,ac);
}


/*-----------------------------------------------------------------------*/
/* Load the text from a file                                             */
/*-----------------------------------------------------------------------*/

/* provides chars to "set_text" routine */

static char get_from_file(XtPointer d)
{
	FILE **f = (FILE**)d;
    int n =  getc(*f);
    return (n==EOF?0:(char)n);
}

/* Public routine */

void HyperLoadFile(widget,fname)
Widget widget;
char   *fname;
{
/*#ifndef linux
    extern char *sys_errlist[];
    #endif*/

    FILE *f = fopen(fname,"r");
    if(f)
    {
        set_text((HyperWidget)widget,get_from_file,(XtPointer)f);
        fclose(f);
    }
    else
    {
        char msg[1024];
        sprintf(msg,"%s: %s",fname,strerror(errno)); /* sys_errlist[errno]); */
        XtWarning(msg);
    }

}

/*-----------------------------------------------------------------------*/
/* Load text from memory buffer                                          */
/*-----------------------------------------------------------------------*/

/* provides chars to "set_text" routine */

static char get_from_buffer(XtPointer d)
{
	char **buffer = (XtPointer)d;
    char c = **buffer;
    (*buffer)++;
    return c;
}

/* Public routine */

void HyperSetText(widget,text)
Widget  widget;
char *text;
{
    set_text((HyperWidget)widget,get_from_buffer,(XtPointer)text);
}

/*-----------------------------------------------------------------------*/
/* Specifies start and end of highlignt chars                            */
/*-----------------------------------------------------------------------*/

#ifdef _NO_PROTO

void HyperSetTags(widget,start_highlight,end_highlight)
Widget   widget;
int start_highlight;
int end_highlight;

#else

void HyperSetTags(Widget widget,
				  int start_highlight,
				  int end_highlight)

#endif

{
    ((HyperWidget)widget)->hyper.start_of_highlight = start_highlight;
    ((HyperWidget)widget)->hyper.end_of_highlight = end_highlight;
}


/*-----------------------------------------------------------------------*/
/* convert a string to lower case                                        */
/*-----------------------------------------------------------------------*/

static void lowcase(p)
register char *p;
{
    while(*p)
    {
        if(isupper(*p)) *p += 32;
        p++;
    }
}

/*-----------------------------------------------------------------------*/
/* Returns the text of the widget                                        */
/* the memory is allocated. It must be freed by the application          */
/* If include_tags if FALSE, the special characters are not returned     */
/*-----------------------------------------------------------------------*/

#ifdef _NO_PROTO

char *HyperGetText(widget,include_tags)
Widget widget;
Boolean include_tags;

#else

char *HyperGetText(Widget widget,Boolean include_tags)

#endif
{

    HyperWidget  w = (HyperWidget)widget;
    char         *p ;
    text_segment *s = w->hyper.first_seg;
    int          len = 1;
    char         soh[2];
    char         eoh[2];

    soh[0] = w->hyper.start_of_highlight;
    eoh[0] = w->hyper.end_of_highlight;

    soh[1] = eoh[1] = 0;

    /* Get size of text */

    while(s)
    {
        len += s->length?s->length:1;
        if(include_tags && s->type == HIGHLIGHT)
            len += 2;
        s = s->next;
    }

    p = XtMalloc(len);
    *p = 0;

    s = w->hyper.first_seg;
    while(s)
    {
        if(s->length)
        {
            if(include_tags && s->type == HIGHLIGHT)
                strcat(p,soh);
            strcat(p,s->text);
            if(include_tags && s->type == HIGHLIGHT)
                strcat(p,eoh);
        }
        else
            strcat(p,"\n");
        s=s->next;
    }

    return p;

}

/*-----------------------------------------------------------------------*/
/* Only for Motif                                                        */
/* If the widget is in a XmScrolledWindow, scroll it so the selection is */
/* visible                                                               */
/*-----------------------------------------------------------------------*/

static void show_selection(h)
HyperWidget h;
{
#ifdef MOTIF

    Widget clip = XtParent(h);
    Widget swin;

    Widget h_scroll;
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



    /* check if selection exists */

    if(!h->hyper.grep_seg) return;

    /* check if the widget is in a scrolled window */
    /* the XnScrolledWindow creates a clip window  */
    /* The widget's parent is the clip window      */


    if(!clip) return;
    swin = XtParent(clip);

    if(!swin || !XmIsScrolledWindow(swin)) return;

    /* Get window scroll bars */

    SetArg(XmNhorizontalScrollBar, &h_scroll);
    SetArg(XmNverticalScrollBar  , &v_scroll);
    GetValues(swin);

    /* Get size of clip window and selection rect */

    w_clip = clip->core.width;
    h_clip = clip->core.height;

    w_grep = h->hyper.grep_width;
    h_grep = h->hyper.grep_height;

    /* Get global coordinates of clip and selection rect */

    XtTranslateCoords(clip,0,0,&x_clip,&y_clip);
    XtTranslateCoords((Widget)h,h->hyper.grep_x,h->hyper.grep_y,&x_grep,&y_grep);

    /* offset of selection within clip window */

    x = x_grep - x_clip;
    y = y_grep - y_clip;


    /* selection y coordinate is not visible */

    if( y < 0 || (Dimension) (y + h_grep) > h_clip)
    {
        /* the widget must be moved verticaly by dv pixels */

        dv = (y + h_grep / 2)  - h_clip / 2;

        SetArg(XmNminimum,&min);
        SetArg(XmNmaximum,&max);

        GetValues(v_scroll);

        XmScrollBarGetValues(v_scroll,&v_val,&v_size,&v_inc,&v_page);

        max -= v_size;

        if( dv + v_val > max ) dv = max - v_val;
        if( dv + v_val < min ) dv = min - v_val;


    }

    /* selection x coordinate is not visible */

    if( x < 0 || (Dimension) (x + w_grep) > w_clip)
    {
        /* the widget must be moved horizontaly by dh pixels */

        dh = (x + w_grep / 2)  - w_clip / 2;

        SetArg(XmNminimum,&min);
        SetArg(XmNmaximum,&max);
        GetValues(h_scroll);

        XmScrollBarGetValues(h_scroll,&h_val,&h_size,&h_inc,&h_page);

        max -= h_size;

        if( dh + h_val > max ) dh = max - h_val;
        if( dh + h_val < min ) dh = min - h_val;

    }

    /* if the widget must be moved */

    if(dv || dh)
    {
        Position x = h->core.x-dh;
        Position y = h->core.y-dv;

        /* move it */

        SetArg(XmNx,x);
        SetArg(XmNy,y);
        SetValues((Widget)h);

        /* update scroll bars */

        if(dv) XmScrollBarSetValues(v_scroll,v_val+dv,v_size,v_inc,
            v_page,TRUE);
        if(dh) XmScrollBarSetValues(h_scroll,h_val+dh,h_size,h_inc,
            h_page,TRUE);
    }

#endif /* MOTIF */
}

/*-----------------------------------------------------------------------*/
/* Clear previous selection                                              */
/*-----------------------------------------------------------------------*/

static void clear_selection(w)
HyperWidget w;
{
    if(w->hyper.grep_seg)
    {
        if(XtIsRealized((Widget)w))

            /* force a redraw */

            XClearArea(XtDisplay(w),XtWindow(w),
                w->hyper.grep_x,
                w->hyper.grep_y,
                w->hyper.grep_width,
                w->hyper.grep_height,
                TRUE);

    }
    w->hyper.grep_seg = NULL;
}

/*-----------------------------------------------------------------------*/
/* Set the new selection                                                 */
/*-----------------------------------------------------------------------*/

static void set_selection(w)
HyperWidget w;
{
    if(w->hyper.grep_seg)
    {
        text_segment *s = w->hyper.grep_seg;
        XCharStruct   char_info;
        int dir,ascent,desc;

        /* get size of the begining of
           the segment, up to the found string */

        XTextExtents(
            (s->type == HIGHLIGHT ? 
            w->hyper.highlight_font : 
            w->hyper.normal_font),
            s->text,
            w->hyper.grep_off,
            &dir,&ascent,&desc,&char_info);

        w->hyper.grep_x      = s->x + char_info.width;
        w->hyper.grep_y      = s->y + desc;
        w->hyper.grep_height = s->height;

        /* Get size of the selection */

        XTextExtents(
            (s->type == HIGHLIGHT ? 
            w->hyper.highlight_font : 
            w->hyper.normal_font),
            w->hyper.grep_txt,
            w->hyper.grep_len,
            &dir,&ascent,&desc,&char_info);


        w->hyper.grep_width  = char_info.width;

        /* force update */

        if(XtIsRealized((Widget)w))
            XClearArea(XtDisplay(w),XtWindow(w),
                w->hyper.grep_x,
                w->hyper.grep_y,
                w->hyper.grep_width,
                w->hyper.grep_height,
                TRUE);
    }
}

/*-----------------------------------------------------------------------*/
/* Select a word in the hyper widget                                     */
/* word : word to find ( or regular expression if USE_REGEX is defined)  */
/* ignore_case : if TRUE ignore case in comparaison                      */
/* from_start : if TRUE search from start of text, else search from      */
/* current selection                                                     */
/* wrap: if TRUE, continue search from the begining of text if the end   */
/* is reached                                                            */
/*-----------------------------------------------------------------------*/

#ifdef _NO_PROTO

Boolean HyperGrep(widget,word,ignore_case,from_start,wrap)
Widget   widget;
char     *word;
Boolean  ignore_case;
Boolean  from_start;
Boolean  wrap;

#else

Boolean HyperGrep(Widget widget,
		  char *word,
		  Boolean ignore_case,
		  Boolean from_start,
		  Boolean wrap)
#endif

{
    HyperWidget  h = (HyperWidget)widget;
    char         *w = word;
    char         *p;
    int          offset;
    text_segment *s;

    if(!h->hyper.first_seg) return False;

    if(ignore_case)
    {
        /* if ignore case, change word to lower case */
        w = XtNewString(word);
        lowcase(w);
    }

    /* compile the regular expression */
    xec_compile(w);


    if(ignore_case) XtFree((XtPointer)w);

    /* if from_start or no previous selection, 
       start from first segment */

    if(from_start || h->hyper.grep_seg == NULL)
    {
        offset=0;
        wrap = FALSE;
        s = h->hyper.first_seg;
    }
    else 
    {
        /* start from last selection */

        offset = h->hyper.grep_off + h->hyper.grep_len;
        s = h->hyper.grep_seg;
    }

    for(;;)
    {
        if(s->text)
        {
            if(ignore_case)
            {
                /* if ignore case, change segment to lower case */
                p = XtNewString(s->text);
                lowcase(p);
            }
			else
				p = s->text;

            /* search the string */

            if(xec_step(p+offset))
            {
                /* if found ...*/

                /* clear previous selection */
                clear_selection(h);

                h->hyper.grep_seg = s;
                h->hyper.grep_off = offset + (xec_loc1-(p+offset));
                h->hyper.grep_txt = s->text + h->hyper.grep_off;
                h->hyper.grep_len = xec_loc2-xec_loc1;

                /* set new selection */

                set_selection(h);

                /* make it visible */

                show_selection(h);

                if(ignore_case) XtFree((XtPointer)p);

                return TRUE;
            }

            if(ignore_case) XtFree((XtPointer)p);
        }

        offset = 0;
        s = s->next;

        /* if end of text and wrap mode, go to start of text */
        if(!s)
	{
            if(wrap)
            {
                wrap = FALSE;
                s = h->hyper.first_seg;
            }
            else break;
	}

    }


    return FALSE;

}
