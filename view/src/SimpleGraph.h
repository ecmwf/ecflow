#ifndef SIMPLEGRAPH_H
#define SIMPLEGRAPH_H
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

#include "SimpleBase.h"

extern WidgetClass  simplegraphWidgetClass;

typedef struct _SimpleGraphClassRec *SimpleGraphWidgetClass;
typedef struct _SimpleGraphRec      *SimpleGraphWidget;

#define XtNhorizontalSpace    "horizontalSpace"
#define XtNverticalSpace      "verticalSpace"
#define XtCPad                "Pad"

#define XtNarcOnly             "arcOnly"
#define XtCArcOnly             "ArcOnly"

#define XtNarrowAngle       "arrowAngle"
#define XtNarrowFilled      "arrowFilled"
#define XtNarrowLength      "arrowLength"
#define XtCArrowAngle       "ArrowAngle"
#define XtCArrowFilled      "ArrowFilled"
#define XtCArrowLength      "ArrowLength"

Widget CreateGraph(Widget,char*, Arg*,int);


#endif /* SIMPLEGRAPH_H */
