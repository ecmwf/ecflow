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
#include <Xm/Xm.h>
#include <stdio.h>
#include "xec.h"

/*-----------------------------------------------

	Set or reset a toggle button

-------------------------------------------------*/

void xec_SetToggle(Widget w,int set)
{
	Arg arg;

	XtSetArg(arg,XmNset,set?1:0); 
	XtSetValues(w,&arg,1);
}

int xec_GetToggle(Widget w)
{
	Boolean set;Arg arg;

	XtSetArg(arg,XmNset, &set); 
	XtGetValues(w,&arg,1);

	return set;
}
