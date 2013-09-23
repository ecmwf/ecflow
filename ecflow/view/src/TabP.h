#ifndef TABP_H
#define TABP_H
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


#include <Xm/XmP.h>
#include <Xm/DrawingAP.h>

typedef struct _TabClassPart {
    int         ignore;
} TabClassPart;

typedef struct _TabClassRec {
    CoreClassPart       core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
    XmManagerClassPart  manager_class;
    XmDrawingAreaClassPart  drawing_area_class;
    TabClassPart     tab_class;
} TabClassRec;

extern TabClassRec tabClassRec;

typedef struct {
	XtCallbackList cb;
	XtCallbackList open_cb;
	XtCallbackList close_cb;
	XmFontList     font;
	Widget         current;
	GC             gc;
	Pixel          back;
	Pixel          blue;

	Dimension      hmargin;
	Dimension      vmargin;

	Dimension      title;
	Dimension      top;
	Dimension      bottom;

	int            delta;
	int            first;
	int            last;

	Boolean        drawer;
	Time            last_click;

} TabPart;


typedef struct _TabRec {
    CorePart        core;
    CompositePart   composite;
    ConstraintPart  constraint;
    XmManagerPart    manager;
    XmDrawingAreaPart   drawing_area;
    TabPart      tab;
}  TabRec;


#define XtTabNumChildren(w) (((TabWidget)w) -> composite.num_children)
#define XtTabChild(w,i)     (((TabWidget)w) -> composite.children[i])

#endif /* TABP_H */



