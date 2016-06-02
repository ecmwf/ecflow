#ifndef SIMPLETIMEP_H
#define SIMPLETIMEP_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #4 $                                                                    */
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


#include        "SimpleBaseP.h"
#include        <Xm/DrawingAP.h>


typedef struct _SimpleTimeClassPart {
    int         ignore;
} SimpleTimeClassPart;

typedef struct _SimpleTimeClassRec {
    CoreClassPart           core_class;
    CompositeClassPart      composite_class;
    ConstraintClassPart     constraint_class;
    XmManagerClassPart      manager_class;
    XmDrawingAreaClassPart  drawing_area_class;
    SimpleBaseClassPart     simplebase_class;
    SimpleTimeClassPart     simpletime_class;
} SimpleTimeClassRec;

extern SimpleTimeClassRec simpletimeClassRec;

typedef struct {
	Pixel           foreground;      /* Color of time line */
    Dimension       v_min_space;     /* Distance between rows */
	int             second_per_pixel;/* Scale factor */
	int			    minute;          /* Current time */
	Boolean			auto_scroll;     /* Allays show current time line */
	XtIntervalId	timeout_id;      /* Time out id */
	GC				gc;              /* Gc for time line */
	int          start_date;
	int          end_date;
	int          start_time;
	int          end_time;
	int             arcs;
	int             inited;
	int             max_w;
	int             max_h;
	int             title;
	XmFontList      font;
} SimpleTimePart;


typedef struct _SimpleTimeRec {
    CorePart            core;
    CompositePart       composite;
    ConstraintPart      constraint;
    XmManagerPart       manager;
    XmDrawingAreaPart   drawing_area;
    SimpleBasePart      simplebase;
    SimpleTimePart      simpletime;
}  SimpleTimeRec;


#endif 



