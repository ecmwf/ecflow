#ifndef SIMPLETIME_H
#define SIMPLETIME_H
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

#include "SimpleBase.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern WidgetClass  simpletimeWidgetClass;

typedef struct _SimpleTimeClassRec *SimpleTimeWidgetClass;
typedef struct _SimpleTimeRec      *SimpleTimeWidget;

/* constraint resources */

#define XtNstartSimpleTime          "startSimpleTime"
#define XtNendSimpleTime            "endSimpleTime"
#define XtCSimpleTime               "SimpleTime"


/* Tieme widget resources */

/* Pixel between rows of widgets */

#define XtNverticalSpace      "verticalSpace"

/* Scale factor */

#define XtNpixelSecond		  "pixelSecond"
#define XtCPixelSecond		  "PixelSecond"

/* always make current simpletime visible */

#define XtNautoScroll		  "autoScroll"
#define XtCAutoScroll		  "AutoScroll"

extern Widget CreateTime(Widget,char*,Arg*,int);

#define TIME_NOW	(-1)      /* Used in SimpleTimeShowSimpleTime */


typedef struct DateTime {
	int date;
	int time;
} DateTime ;

void     TimeSetTime(Widget,int,DateTime);
DateTime TimeGetTime(Widget,int);
void     TimeAdd(DateTime*,int);
int      TimeDiff(DateTime,DateTime);

void*    TimeFindByY(Widget,XEvent*);

void TimeEventTime(Widget,XEvent*,DateTime*);
void TimeShowTime(Widget,DateTime,XEvent*);
void TimeRange(Widget,DateTime*,DateTime*);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif
