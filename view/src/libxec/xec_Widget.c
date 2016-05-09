/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #1 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2016 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/
#include <stdio.h>
#include <Xm/Xm.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include <X11/IntrinsicP.h>
#include "xec.h"


void *xec_GetUserData(Widget w)
{
	void *p;
	XtVaGetValues(w,XmNuserData,&p,NULL);
	return p;
}

/*-------------------------------------------------

	Attach a pointer to a widget

-------------------------------------------------*/

void xec_SetUserData(Widget w, void* p)
{
	XtVaSetValues(w,XmNuserData,p,NULL);
}

/*-------------------------------------------------

	Sets a color

--------------------------------------------------*/


void xec_SetColor(Widget w, Pixel p, const char* which)
{
	XtVaSetValues(w,which,p,NULL);
}

/*-------------------------------------------------

	Scroll a window to make a widget visible

--------------------------------------------------*/

#if 1

void xec_ShowWidget(Widget w)
{

	Widget v_scroll,h_scroll;


	Position 	x_parent,y_parent;
	Position 	x_widget,y_widget;
	Dimension 	h_widget,w_widget;
	Position 	x_clip,y_clip;
	Dimension 	h_clip,w_clip;
	Position 	dv=0,dh=0;
	int min,max;
	int	v_val,v_size,v_inc,v_page;
	int	h_val,h_size,h_inc,h_page;
	Widget clip;
	Widget parent;
	Widget scroll_window;

	Position x,y;


	if(!XtIsManaged(w)) return;

	if(!(parent          = XtParent(w)))      return;
	if(!(clip   		 = XtParent(parent))) return;
	if(!(scroll_window   = XtParent(clip)))   return;

	if(!XmIsScrolledWindow(scroll_window)) return;

	XtVaGetValues(scroll_window,
		XmNhorizontalScrollBar, &h_scroll,
		XmNverticalScrollBar, &v_scroll ,
		NULL);


	XtVaGetValues(parent,
		XmNx,&x_parent,
		XmNy,&y_parent,
		NULL);


	XtVaGetValues(w,
		XmNheight,&h_widget,
		XmNwidth,&w_widget,
		NULL);


	XtVaGetValues(scroll_window,
		XmNclipWindow,&clip,
		NULL);

	XtVaGetValues(clip,
		XmNheight,&h_clip,
		XmNwidth,&w_clip,
		NULL);


	XtTranslateCoords(w,0,0,	&x_widget,&y_widget);
	XtTranslateCoords(clip,0,0,&x_clip,&y_clip);


	x = x_widget - x_clip;
	y = y_widget - y_clip;


	if( y < 0 || (Dimension) (y + h_widget) > h_clip)
	{
		dv = (y + h_widget / 2)  - h_clip / 2;

		XtVaGetValues(v_scroll,
			XmNminimum,&min,
			XmNmaximum,&max,
			NULL);

		XmScrollBarGetValues(v_scroll,&v_val,&v_size,&v_inc,&v_page);

		max -= v_size;

		if( dv + v_val > max ) dv = max - v_val;
		if( dv + v_val < min ) dv = min - v_val;


	}

	if( x < 0 || (Dimension) (x + w_widget) > w_clip)
	{
		dh = (x + w_widget / 2)  - w_clip / 2;

		XtVaGetValues(h_scroll,
			XmNminimum,&min,
			XmNmaximum,&max,
			NULL);

		XmScrollBarGetValues(h_scroll,&h_val,&h_size,&h_inc,&h_page);

		max -= h_size;

		if( dh + h_val > max ) dh = max - h_val;
		if( dh + h_val < min ) dh = min - h_val;

	}


	if(dv || dh)
	{
		Position x = x_parent-dh;
		Position y = y_parent-dv;

		XtVaSetValues(parent,
			XmNx,x,
			XmNy,y,
			NULL);

		/*
		XtMoveWidget(parent,x_parent-dh,y_parent-dv);
		*/

		if(dv) XmScrollBarSetValues(v_scroll,v_val+dv,v_size,v_inc,v_page,TRUE);
		if(dh) XmScrollBarSetValues(h_scroll,h_val+dh,h_size,h_inc,h_page,TRUE);


		/* force redraw */
		/*

		XtUnmanageChild(parent);
		XtManageChild(parent);
		*/
	}


}

#endif
void xec_Invert(Widget w)
{
	Pixel fg,bg;
	XtVaGetValues(w,XmNbackground,&bg,XmNforeground,&fg,NULL);
	XtVaSetValues(w,XmNbackground,fg,XmNforeground,bg,NULL);
}

void xec_ManageAll(Widget w)
{
	CompositeWidget c = (CompositeWidget)w;
	XtManageChildren(c->composite.children,
	    c->composite.num_children);
}

void xec_UnmanageAll(Widget w)
{
	CompositeWidget c = (CompositeWidget)w;
	XtUnmanageChildren(c->composite.children,
	    c->composite.num_children);
}

