#ifndef SIMPLEGRAPHP_H
#define SIMPLEGRAPHP_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #4 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2019 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/



#include  <Xm/XmP.h>
#include  <Xm/DrawingAP.h>
#include  "SimpleBaseP.h"

#define GC_COUNT 10

typedef struct _SimpleGraphClassPart {
    int         ignore;
} SimpleGraphClassPart;

typedef struct _SimpleGraphClassRec {
    CoreClassPart           core_class;
    CompositeClassPart      composite_class;
    ConstraintClassPart     constraint_class;
    XmManagerClassPart      manager_class;
    XmDrawingAreaClassPart  drawing_area_class;
    SimpleBaseClassPart     simplebase_part;
    SimpleGraphClassPart    simplegraph_class;
} SimpleGraphClassRec;

extern SimpleGraphClassRec simplegraphClassRec;


typedef struct {
    Dimension      h_min_space;
    Dimension      v_min_space;
    Dimension      arrow_length;
    Dimension      arrow_angle;
    Boolean        arrow_filled;
	int            gc_count;
    GC             gc[GC_COUNT];
    float          cos_arrow;
    float          sin_arrow;
    Boolean        mode;
    Widget         arc_only;
} SimpleGraphPart;


typedef struct _SimpleGraphRec {
    CorePart           core;
    CompositePart      composite;
    ConstraintPart     constraint;
    XmManagerPart      manager;
    XmDrawingAreaPart  drawing_area;
    SimpleBasePart     simplebase;
    SimpleGraphPart    simplegraph;
}  SimpleGraphRec;


#endif 



