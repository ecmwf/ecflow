#ifndef SIMPLEBASEP_H
#define SIMPLEBASEP_H
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


#include "SimpleBase.h"
#include <Xm/DrawingAP.h>

typedef void (*PrintProc) (Widget,FILE*);
typedef void (*LayoutProc)(Widget,long*,long*);
typedef void (*ResetProc)(Widget);


typedef struct LinkData {
	GC     gc;
	void* user_data;
} LinkData;

typedef struct Link {
	int   node;
	int   link_data;
} Link;

typedef struct NodeStruct {
    XRectangle   r;
    XtPointer    user_data;
    DrawProc     draw;
    SizeProc     size;
    Boolean      managed;
    Boolean      inited;
    int          pmax;
    int          pcnt;
    int          kmax;
    int          kcnt;
    Link         *parents;
    Link         *kids;
    int          tmpx;
    int          tmpy;
    int          misc[4];
#if 0
    Boolean      is_group;
	int          group;
#endif
} NodeStruct;

typedef struct _SimpleBaseClassPart {
    PrintProc       print;
    LayoutProc      layout;
	ResetProc       reset;
} SimpleBaseClassPart;

typedef struct _SimpleBaseClassRec {
    CoreClassPart           core_class;
    CompositeClassPart      composite_class;
    ConstraintClassPart     constraint_class;
    XmManagerClassPart      manager_class;
    XmDrawingAreaClassPart  drawing_area_class;
    SimpleBaseClassPart     simplebase_class;
} SimpleBaseClassRec;

extern SimpleBaseClassRec simplebaseClassRec;

typedef struct {

    int            max;
    int            count;
    NodeStruct           *nodes;

	LinkData       *links;
	int             link_max;
	int             link_count;

    Pixel          blink_color;
    GC             blink_gc;
    GC             gc;
    int            selected;
    int            focus;
    XtIntervalId   timeout_id;
    int            timeout;
    XtCallbackList getps;
	XtCallbackList link;
    String         header;
    XtWorkProcId   work;

} SimpleBasePart;


typedef struct _SimpleBaseRec {
    CorePart            core;
    CompositePart       composite;
    ConstraintPart      constraint;
    XmManagerPart       manager;
    XmDrawingAreaPart   drawing_area;
    SimpleBasePart      simplebase;
}  SimpleBaseRec;


#define NODE_TO_INDEX(w,n) ((n)-(w)->simplebase.nodes)
#define INDEX_TO_NODE(w,i) (&((w)->simplebase.nodes[i]))


#define KIDS(w,n,i)    (*INDEX_TO_NODE(w,n->kids[i].node))
#define PARENTS(w,n,i) (*INDEX_TO_NODE(w,n->parents[i].node))


int  sb_new_dummy_node(SimpleBaseWidget w);
void sb_clear_dummy_nodes(SimpleBaseWidget w);
int  sb_insert_dummy_node(SimpleBaseWidget w,int p,int k);

int sb_find_kid_index(SimpleBaseWidget w,NodeStruct* p,NodeStruct *k);
int sb_find_parent_index(SimpleBaseWidget w,NodeStruct* k,NodeStruct *p);


Boolean sb_is_dummy(SimpleBaseWidget w,NodeStruct*p);


#endif



