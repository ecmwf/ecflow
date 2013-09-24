#ifndef SIMPLETREE_H
#define SIMPLETREE_H
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


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif
#ifndef MOTIF
#define MOTIF
#endif

#include "SimpleBase.h"

extern WidgetClass  simpletreeWidgetClass;

typedef struct _SimpleTreeClassRec *SimpleTreeWidgetClass;
typedef struct _SimpleTreeRec      *SimpleTreeWidget;

#define XtNhorizontalSpace    "horizontalSpace"
#define XtNverticalSpace      "verticalSpace"
#define XtNparentNode         "parentNode"
#define XtCParentNode         "ParentNode"
#define XtNselected           "selected"
#define XtCSelected           "Selected"
#define XtNvertical            "vertical"
#define XtCVertical            "Vertical"

extern Widget CreateTree(Widget,char*,Arg*,int);
extern void NodeTreeFlip(Widget,int);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif 
