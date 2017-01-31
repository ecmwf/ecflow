#ifndef TAB_H
#define TAB_H
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


#define XtNround   "round"
#define XtCRound   "Round"
#define XtNrows    "rows"
#define XtNcolumns "columns"
#define XtCRowCol  "RowCol"

#define XmNopenCallback  "openCallback"
#define XmNcloseCallback "closeCallback"

extern WidgetClass  tabWidgetClass;

typedef struct _TabClassRec *TabWidgetClass;
typedef struct _TabRec      *TabWidget;

#ifdef _NO_PROTO

extern Widget CreateTab();

#else
 
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

Widget    CreateTab(Widget,String,Arg*,int);
Widget    TabGetCurrent(Widget);
void      TabSetCurrent(Widget,Widget,Boolean);

void TabOpen(Widget);
void TabClose(Widget);

Boolean TabClosed(Widget);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
 
#endif /* _NO_PROTO */

typedef struct {
    int      reason;
    XEvent  *event;
    Widget   widget;
}TabCallbackStruct;


#endif /* TAB_H */
