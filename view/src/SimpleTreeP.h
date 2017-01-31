#ifndef SIMPLETREEP_H
#define SIMPLETREEP_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #4 $                                                                    */
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


#include "SimpleBaseP.h"
#include <Xm/DrawingAP.h>


typedef struct _SimpleTreeClassPart {
    int         ignore;
} SimpleTreeClassPart;

typedef struct _SimpleTreeClassRec {
    CoreClassPart           core_class;
    CompositeClassPart      composite_class;
    ConstraintClassPart     constraint_class;
    XmManagerClassPart      manager_class;
    XmDrawingAreaClassPart  drawing_area_class;
    SimpleBaseClassPart     simplebase_class;
    SimpleTreeClassPart     simpletree_class;
} SimpleTreeClassRec;

extern SimpleTreeClassRec simpletreeClassRec;

typedef struct {
    Dimension      h_min_space;
    Dimension      v_min_space;
} SimpleTreePart;


typedef struct _SimpleTreeRec {
    CorePart            core;
    CompositePart       composite;
    ConstraintPart      constraint;
    XmManagerPart       manager;
    XmDrawingAreaPart   drawing_area;
    SimpleBasePart      simplebase;
    SimpleTreePart      simpletree;
}  SimpleTreeRec;



#endif 



