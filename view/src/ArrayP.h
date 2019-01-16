#ifndef ARRAYP_H
#define ARRAYP_H
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


#include <Xm/XmP.h>
#include <Xm/DrawingAP.h>

typedef struct _ArrayClassPart {
    int         ignore;
} ArrayClassPart;

typedef struct _ArrayClassRec {
    CoreClassPart       core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
    XmManagerClassPart  manager_class;
    XmDrawingAreaClassPart  drawing_area_class;
    ArrayClassPart     array_class;
} ArrayClassRec;

extern ArrayClassRec arrayClassRec;

typedef struct {
	int round;
	int rows;
	int cols;
} ArrayPart;


typedef struct _ArrayRec {
    CorePart        core;
    CompositePart   composite;
    ConstraintPart  constraint;
    XmManagerPart    manager;
    XmDrawingAreaPart   drawing_area;
    ArrayPart      array;
}  ArrayRec;


#define XtArrayNumChildren(w) (((ArrayWidget)w) -> composite.num_children)
#define XtArrayChild(w,i)     (((ArrayWidget)w) -> composite.children[i])

#endif /* ARRAYP_H */



