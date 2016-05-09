/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #1 $                                                                    */
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

#include <X11/X.h>
#include <X11/cursorfont.h>
#include <Xm/Xm.h>

/*----------------------------------------------------

	Set the cursor to a watch shape. 
	Usualy pass the Top Shell as argument.

----------------------------------------------------*/

void xec_SetWatchCursor(w)
Widget w;
{
	static Cursor watch = 0;

	if(!watch)
		watch = XCreateFontCursor(XtDisplay(w),XC_watch);

	XDefineCursor(XtDisplay(w),XtWindow(w),watch);
	XmUpdateDisplay(w);
}

/*----------------------------------------------------

	Reset the cursor to its default shape. 
	Usualy pass the Top Shell as argument.

----------------------------------------------------*/

void xec_ResetCursor(w)
Widget w;
{
	XUndefineCursor(XtDisplay(w),XtWindow(w));
	XmUpdateDisplay(w);
}

