/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #1 $                                                                    */
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
#include <Xm/Xm.h>
#include <string.h>
#include <malloc.h>

/*-----------------------------------------------------

	Create a new XmString from a char*

	This function can deal with embedded 'newline' and
	is equivalent to the obsolete XmStringCreateLtoR,
	except it does not use non AES compliant charset
	XmSTRING_DEFAULT_CHARSET

------------------------------------------------------*/


XmString xec_NewString(const char* s)
{
	XmString xms1;
	XmString xms2;
	XmString line;
	XmString separator;
	char     *p;
	char     *t = XtNewString(s);	/* Make a copy for strtok not to */
                                 	/* damage the original string    */

	separator = XmStringSeparatorCreate();
	p         = strtok(t,"\n");
	xms1      = XmStringCreateSimple(p);

	while ((p = strtok(NULL,"\n")))
	{
		line = XmStringCreateSimple(p);
		xms2 = XmStringConcat(xms1,separator);
		XmStringFree(xms1);
		xms1 = XmStringConcat(xms2,line);
		XmStringFree(xms2);
		XmStringFree(line);
	}

	XmStringFree(separator);
	XtFree(t);
	return xms1;
}


/*-----------------------------------------------------

	Build an XmString list from char*

------------------------------------------------------*/

void xec_BuildXmStringList(XmString** list, char* p, int *count)
{
	XmString	*l = *list;

	if(!l) {
		*count = 0;
		l = (XmString*)malloc(0);
	}

	(*count)++;
	l = (XmString*)XtRealloc((char*) l,sizeof(XmString)*(*count));
	l[(*count-1)] = xec_NewString(p);

	*list = l;
}

/*-----------------------------------------------------

	Free an XmString list 

------------------------------------------------------*/

void xec_FreeXmStringList(XmString* list, int count)
{
	int i;

	if(list)
	{
		for(i=0;i<count;i++) XmStringFree(list[i]);
		XtFree((XtPointer)list);
	}
}

char *xec_GetString(XmString string)
{
	XmStringContext  context;
	char             *text;
	XmStringCharSet   charset;
	XmStringDirection dir;
	Boolean           separator;
	char             *buf = NULL;
	int               done = FALSE;

	XmStringInitContext (&context, string);
	while (!done)
		if(XmStringGetNextSegment (context, &text, &charset, &dir, &separator))
		{
			if(separator) /* Stop when next segment is a separator */
				done = TRUE;

			if(buf)
			{
				buf = XtRealloc(buf, strlen(buf) + strlen(text) + 2);
				strcat(buf, text);
			}
			else
				buf = XtNewString(text);

			XtFree((XtPointer)charset);
			XtFree((XtPointer)text);
		}
		else
			done = TRUE;

	XmStringFreeContext (context);
	return buf;

}
