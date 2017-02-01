/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #1 $                                                                    */
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

#include <stdio.h>
#include <stdarg.h>
#include <Xm/Xm.h>
#include "xec.h"

/*-----------------------------------------------

	Change the title of a Label or Button

-------------------------------------------------*/

void xec_SetLabel(Widget w,const char *title)
{
	XmString	s = xec_NewString(title);
	XtVaSetValues(w,XmNlabelString,s,NULL);
	XmStringFree(s);
}

void xec_VaSetLabel(Widget w,const char *fmt,...)
{
	va_list   args;
	char      str[1000];  /* DANGER: Fixed buffer size */
	va_start(args,fmt);
	vsprintf(str, fmt, args);
	xec_SetLabel(w,str);
	va_end(args);
}
