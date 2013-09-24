#ifndef ARRAY_H
#define ARRAY_H
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


#define XtNround   "round"
#define XtCRound   "Round"
#define XtNrows    "rows"
#define XtNcolumns "columns"
#define XtCRowCol  "RowCol"

extern WidgetClass  arrayWidgetClass;

typedef struct _ArrayClassRec *ArrayWidgetClass;
typedef struct _ArrayRec      *ArrayWidget;

#ifdef _NO_PROTO

extern Widget CreateArray();

#else
 
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

Widget    CreateArray(Widget,String,Arg*,int);
#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
 
#endif /* _NO_PROTO */


#endif /* ARRAY_H */
