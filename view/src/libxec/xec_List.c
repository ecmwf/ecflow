/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #2 $                                                                    */
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
#include <Xm/ListP.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include "xec.h"


/*-----------------------------------------------------

	Remove one line to a list

------------------------------------------------------*/

void xec_RemoveListItem(Widget w,char *p)
{
	XmString item= XmStringCreateSimple(p);
	XmListDeleteItem(w,item);
	XmStringFree(item);
}

void xec_ListItemSelect(Widget w,const char* p)
{
	XmString item= XmStringCreateSimple((char*)p);
	int n = XmListItemPos(w,item);

	XmListDeselectAllItems(w);
	if(n) {
		int vis_count;
		XtVaGetValues(w,XmNvisibleItemCount,&vis_count,NULL);
		XmListSelectPos(w,n,False);
		if(n<((XmListWidget)w)->list.top_position || 
			n > ((XmListWidget)w)->list.top_position + vis_count)
			XmListSetPos(w,n);
	}

	XmStringFree(item);
}

#if 0
/*----------------------------------------------------

	Add to a list with max number of items

-----------------------------------------------------*/

void xec_AddListMax(Widget w,int max,char* p)
{
	ARG_DEF(1);
	int n;

	SetArg(XmNitemCount,&n);
	GetValues(w);
	if(n>=max) XmListDeletePos(w,1);
	xec_AddListItem(w,p);

	SetArg(XmNitemCount,&n);
	GetValues(w);
	XmListSetBottomPos(w,n);
}

void xec_VaAddListMax(Widget  w,int max,char *fmt,...)
{
	va_list   args;
	char      str[1000];  /* DANGER: Fixed buffer size */
				 
	va_start(args,fmt);
	vsprintf(str, fmt, args);
    xec_AddListMax(w,max,str);
	va_end(args);
}

#endif

void xec_ListSelectAll(Widget w)
{
	int n,i;
	XtVaGetValues(w,XmNitemCount,&n,NULL);
	XmListDeselectAllItems(w);
	for(i=1;i<=n;i++) XmListSelectPos(w,i,False);

}
							   

/*-----------------------------------------------------

	Add one line to a list

------------------------------------------------------*/

void xec_AddListItem(Widget w,char *p)
{
	XmString item= XmStringCreateSimple(p);
	XmListAddItemUnselected(w,item,0);
	XmStringFree(item);
}

Boolean xec_AddListItemUnique(Widget w,char *p,Boolean sel)
{
	XmString item= XmStringCreateSimple(p);
	int added = 0;
	if(!XmListItemExists(w,item))
	{
		XmListAddItemUnselected(w,item,0);
		if(sel) XmListSelectItem(w,item,0);
		added = 1;
	}
	XmStringFree(item);
	return added;
}


void xec_VaAddListItem(Widget w,char *fmt,...)
{
	va_list   args;
	char      str[1000];  /* DANGER: Fixed buffer size */
				 
	va_start(args,fmt);
	vsprintf(str, fmt, args);
	xec_AddListItem(w,str);
	va_end(args);
}

void xec_ReplaceListItem(Widget w,const char* from,const char* to)
{
	XmString a= XmStringCreateSimple((char*)from);
	XmString b= XmStringCreateSimple((char*)to);
	XmListReplaceItems(w,&a,1,&b);
	XmStringFree(a);
	XmStringFree(b);
}

#if 0

/*-----------------------------------------------------

	Replace all items in a list

------------------------------------------------------*/
void xec_SetListItems(Widget w,XmString list[],int count)
{
	ARG_DEF(2);

	SetArg(XmNitems, list );
	SetArg(XmNitemCount, count );
	SetValues(w);

}

/*---------------------------------------------------*/
int xec_DumpList(FILE *f,char *fmt,Widget w)
{
	ARG_DEF(2);
	int 	count;
	XmString *list;
	int i;
	int ret = 0;

	SetArg(XmNitems,&list);
	SetArg(XmNitemCount,&count);
	GetValues(w);

	for(i=0;i<count;i++)
	{
		char *p = (char*)xec_GetString(list[i]);
		fprintf(f,fmt,p);
		if(errno) ret = errno;
		XtFree((XtPointer)p);
	}
	return errno = ret;
}
/*-----------------------------------------------------*/

/*--------------------------------------------------

	Search the regexp 'word' in the list 'w'.
	Return TRUE in the text was found.
	if 'nocase' is TRUE the search is not case sensitive.
	if 'fromstart' is TRUE the search start from the
	fisrt char, else from the current position.
	if 'wrap' is TRUE the search is done all the text.

	---------------------------------------------------*/

Boolean xec_ListSearch(Widget w,char   *word,Boolean nocase,Boolean fromstart,Boolean wrap)
{
  /* ARG_DEF(5); */
	int first = 0;
	int count,sel_count,vis_count;
	XmString	*items,*sel_items;
	char        *p;
	int			from,to,i;

	SetArg(XmNitemCount,&count);
	SetArg(XmNselectedItemCount,&sel_count);
	SetArg(XmNitems,&items);
	SetArg(XmNselectedItems,&sel_items);
	SetArg(XmNvisibleItemCount,&vis_count);
	GetValues(w);

	if(!fromstart)
		if(sel_count) first = ((XmListWidget)w)->list.selectedIndices[0];

	while(TRUE)
	{
		for(i=first;i<count;i++)
		{
			p = xec_GetString(items[i]);
			if(regexp_find(word,p,nocase,&from,&to))
			{
				int n = i+1;

				XmListDeselectAllItems(w);
				XmListSelectPos(w,n,False);
				if(n<((XmListWidget)w)->list.top_position || 
				    n > ((XmListWidget)w)->list.top_position + vis_count)
					XmListSetPos(w,n);
				XtFree((XtPointer)p);
				return TRUE;
			}
			XtFree((XtPointer)p);
		}
		if(!wrap) break;
		first = 0;
		wrap = FALSE;
	}
	return FALSE;
}

void xec_ListSelect(Widget w,int n)
{
	ARG_DEF(5);
	int vis_count;

	SetArg(XmNvisibleItemCount,&vis_count);
	GetValues(w);

	XmListDeselectAllItems(w);
	XmListSelectPos(w,n,False);

	/* printf("xec_ListSelect %d\n",n); */

	if(n<((XmListWidget)w)->list.top_position || 
		    n > ((XmListWidget)w)->list.top_position + vis_count)

	XmListSetPos(w,n);
}

#endif

static char *fonts[] = {
	"normal","bold", };

void xec_AddFontListItem(Widget list,char *buffer,Boolean bold)
{
   int index = (bold) ? 1 : 0 ;
   XmString s = XmStringCreateLtoR(buffer,fonts[index]);
   XmListAddItem(list,s,0);
   XmStringFree(s);
}

