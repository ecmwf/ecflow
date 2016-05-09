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
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/Text.h>
#include <Xm/TextP.h>
#include <Xm/Form.h>
#include <Xm/TextStrSoP.h>
#include <Xm/CutPaste.h>
#include "xec.h"
#include <sys/types.h>
#include <sys/mman.h>
#include <regex.h>
#include <setjmp.h>
#include <signal.h>

/* Avi: Added to fix: warning: implicit declaration of function ‘_XmTextUpdateLineTable’ */
extern void _XmTextUpdateLineTable(
				   Widget widget,
				   XmTextPosition start,
				   XmTextPosition end,
				   XmTextBlock block,
				   Boolean update) ; 
/* JIRA:ECFLOW-48   int update) ; */

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)(-1))
#endif

int  regexp_find(const char *word,const char *buffer,
	int nocase,int *from,int *to)
{
	regex_t re;
	regmatch_t pmatch[1];
	int e;

	if((e = regcomp(&re,word,REG_NEWLINE | REG_EXTENDED | (nocase? REG_ICASE : 0) )))
	{
		char buf[1024];
		regerror(e,&re,buf,sizeof(buf));
		return False;
	}

	if((e = regexec(&re,buffer,XtNumber(pmatch),pmatch,0)))
	{
		char buf[1024];
		regerror(e,&re,buf,sizeof(buf));
		regfree(&re);
		return False;
	}

	*from = pmatch[0].rm_so;
	*to   = pmatch[0].rm_eo;

	regfree(&re);

	return True;
}

char* xec_TextGetString(Widget w,long *length)
{
#if 0
	return XmTextGetString(w);
#else
	XmTextSource s = XmTextGetSource(w);
	*length = s->data->length;
	return s->data->ptr;
#endif
}

void xec_TextFreeString(char* p)
{
#if 0
	XtFree(p);
#endif
}

static jmp_buf env;

static void catch_sigv(int sig)
{
	fprintf(stderr,"SIGV received...\n");
	longjmp(env,1);
}

/*--------------------------------------------------
---------------------------------------------------*/

/*--------------------------------------------------

	Search the regexp 'word' in the text 'w'.
	Return TRUE in the text was found.
	if 'nocase' is TRUE the search is not case sensitive.
	if 'fromstart' is TRUE the search start from the
	fisrt char, else from the current position.
	if 'wrap' is TRUE the search is done all the text.

---------------------------------------------------*/

Boolean xec_TextSearch(Widget w,char *word,
	Boolean nocase,
	Boolean regex,
	Boolean back,
	Boolean fromstart,
	Boolean wrap)
{

	Boolean			success,more;
	XmTextPosition 	offset,dummy;
	int from,to;
	long length = 0;
	char 			*p=xec_TextGetString(w,&length);

	if(fromstart)
	{
		offset = (!regex && back) ? XmTextGetLastPosition(w) - strlen(word) : 0;
		wrap   = FALSE;
	}
	else
	{
		XmTextGetSelectionPosition(w,&dummy,&offset);
		if(dummy == offset) offset = XmTextGetInsertionPosition(w);

		if(back) { 
			if(dummy)
				offset = dummy-1; 
			else if(wrap)
			{
				offset = XmTextGetLastPosition(w) - strlen(word);
			}
		}
	}

	do
	{

		if(regex) 
		{
			success =  regexp_find(word,p+offset,nocase,&from,&to);
			if(success && ((from+offset > length) || (to + offset > length)))
				success = False;
		}
		else  {

			success = 0;

			 /* Because we use mmap, XmTextFindString may crash */

			signal(SIGSEGV,catch_sigv);
			if(setjmp(env) == 0)
			{
				success = XmTextFindString(w,offset,word, 
					back ? XmTEXT_BACKWARD : XmTEXT_FORWARD,&dummy);
			}
			signal(SIGSEGV,SIG_DFL);



			if(success) {

				from = dummy - offset;
				to   = from + strlen(word);

			}
		}
			

		if(success)
		{
			XmTextShowPosition(w,to+offset);
			XmTextSetSelection(w,from+offset,to+offset,CurrentTime);
		}

		more = wrap && !success;

		if(wrap)
		{
			wrap = FALSE;
			offset = (!regex && back) ? XmTextGetLastPosition(w)  - strlen(word): 0;
		}

	}while(more);

	xec_TextFreeString(p);

	return success;

}



/*-------------------------------------------------

	Copy the content of a Text in a buffer.

--------------------------------------------------*/

char *xec_GetText(w,buf)
Widget w;
char   buf[];
{
	char *q = (char*)XmTextGetString(w);

	strcpy(buf,q);
	XtFree((XtPointer)q);

	return buf;
}


/*-------------------------------------------------

	Load a file into a text widget.
	if include is TRUE the text if included.

--------------------------------------------------*/

int xec_LoadText(Widget Text,const char *fname,Boolean include)
{
	FILE    *fp = NULL;
	char    *p;
	long    length;
	int 	ret = 0;


	errno = 0;

	if (!fname) return -1;

	if ((fp = fopen(fname,"r")))
	{
		fseek(fp,0L,2);
		if (errno)
		{
			ret = errno;
			fclose(fp);
			return errno = ret;
		}

		length=ftell(fp);
		if (errno)
		{
			ret = errno;
			fclose(fp);
			return errno = ret;
		}

		fseek(fp,0L,0);
		if (errno)
		{
			ret = errno;
			fclose(fp);
			return errno = ret;
		}

		p = (char*)XtMalloc(length+1);
		p[length] = 0;

		fread(p,length,1,fp);
		if (errno)
		{
			ret = errno;
			fclose(fp);
			return errno = ret;
		}

		XmTextDisableRedisplay(Text);
		if (include)
			xec_ReplaceTextSelection(Text,p,FALSE);
		else
		{
			XmTextSetInsertionPosition(Text,0);
			XmTextSetSelection(Text,0,0,CurrentTime);
			XmTextSetString(Text,p);
		}
		XmTextEnableRedisplay(Text);

		XtFree(p);
		fclose(fp);
	} 
	else 
	{
		perror(fname);
		ret = errno;
		if (!include) XmTextSetString(Text,"");
	}
	return errno = ret;

}

/* ================================================== */


typedef struct mapped_text {
	XmTextSource    source_;
	FILE*           file_;
	Widget          text_;
	XmSourceDataRec save_;
} mapped_text;

void* xec_MapText(Widget w,const char *fname,int* z)
{
	FILE    *fp = NULL;
	long    length;
	/*int 	ret = 0;  warning: variable ‘ret’ set but not used [-Wunused-but-set-variable] */
	char *m;
	mapped_text *p;
	XEvent ev;
	XmTextBlockRec block;
	int i; 


	errno = 0;

	if (!fname) return NULL;

	if ((fp = fopen(fname,"r")))
	{
		fseek(fp,0L,2);
		if (errno)
		{
			fclose(fp);
			return NULL;
		}

		length=ftell(fp);
		if (errno)
		{
			fclose(fp);
			return NULL;
		}

		fseek(fp,0L,0);
		if (errno)
		{
			fclose(fp);
			return NULL;
		}

		/* f = w; 
		   while(f && !XmIsForm(f)) 
		   f = XtParent(f); 		   
		   if(!f) return NULL; */
		

		m = mmap(NULL,length,PROT_READ,MAP_SHARED,fileno(fp),0);
		if((void*)m == MAP_FAILED)
		{
			perror(fname);
			return NULL;
		}
		(*z) = 0;
		for(i = 0 ; i < length; i++)
			if(m[i] == 0) (*z)++;

		XmTextDisableRedisplay(w);
		XmTextClearSelection(w,CurrentTime);
		XmTextSetInsertionPosition(w,0);
		XmTextSetTopCharacter(w,0);
		XmTextShowPosition(w,0);
		XmTextEnableRedisplay(w);
		XmTextSetString(w,"");
	
		p = XtNew(mapped_text);

		/* p->text1_ = XmCreateText(f,"dummy",0,0); */
		/* p->text2_ = XmCreateText(f,"dummy",0,0); */

		p->source_               = XmTextGetSource(w);
		p->file_                 = fp;
		p->text_                 = w;
		p->save_                 = *p->source_->data;

		p->source_->data->ptr        = m;
		p->source_->data->length     = length;
		p->source_->data->maxlength  = length;
		p->source_->data->value      = m;
		p->source_->data->old_length = length;
		p->source_->data->gap_start  = 0;
		p->source_->data->gap_end    = 0;


		((XmTextWidget)w)->text.needs_refigure_lines = True;
		/* _XmTextNumLines((XmTextWidget)w); */

		block.ptr    = m;
		block.length = length;
		block.format = XmFMT_8_BIT;

		_XmTextUpdateLineTable(w,0, XmTextGetLastPosition(w),&block,1);

#if 1
		/* ((XmTextWidget)w)->text.needs_refigure_lines = True; */
		/* printf("Num lines : %d\n",_XmTextNumLines((XmTextWidget)w)); */
		/* _XmTextValueChanged((XmTextWidget)w,&ev); */
		
		memset(&ev,0,sizeof(ev));
		ev.type = Expose;
		ev.xexpose.display = XtDisplay(w);
		ev.xexpose.window  = XtWindow(w);

		XSendEvent(XtDisplay(w),XtWindow(w),True,ExposureMask,&ev);
#endif

		return p;

	}
	return NULL;

}

void xec_UnmapText(void *x)
{
	if(x)
	{
		XmTextBlockRec block;
		mapped_text* p = (mapped_text*)x;
		Widget w = p->text_;

		((XmTextWidget)w)->text.needs_refigure_lines = True;
		/* _XmTextNumLines((XmTextWidget)w); */

		block.ptr    = NULL;
		block.length = 0;
		block.format = XmFMT_8_BIT;
		_XmTextUpdateLineTable(w,0, XmTextGetLastPosition(w),&block,1);

		XmTextDisableRedisplay(w);
		XmTextClearSelection(w,CurrentTime);
		XmTextSetInsertionPosition(w,0);
		XmTextSetTopCharacter(w,0);
		XmTextShowPosition(w,0);

		XmTextEnableRedisplay(w);

		munmap(p->source_->data->ptr,p->source_->data->length);
		*p->source_->data = p->save_;
		fclose(p->file_);

		XtFree((char*)p);

		XmTextSetString(w,"");
	}

}

/*-------------------------------------------------

	Save a text into a file

--------------------------------------------------*/

/* Open file, save text and close file */

int xec_SaveText(Widget w,char *fname)
{
	FILE    *fp = NULL;
	char    *p = XmTextGetString(w);
	int 	ret;

	errno = 0;

	if (!fname) return -1;

	if ((fp = fopen(fname,"w")))
		if (fwrite(p,strlen(p),1,fp)) fclose(fp);

	ret = errno;
	XtFree(p);
	return errno = ret;
}


/* File is already opened: just save text */

int xec_DumpText(FILE *fp,Widget w)
{
	char    *p = XmTextGetString(w);

	errno = 0;
	fwrite(p,strlen(p),1,fp);
	XtFree((XtPointer)p);
	return errno;
}


/*-------------------------------------------------

	Print a text

--------------------------------------------------*/

void xec_PrintText(Widget w,char * cmd)
{
	char    *tmp = tmpnam(NULL);
	char	buf[1024];

	xec_SaveText(w,tmp);
	if(cmd)
		sprintf(buf,"%s %s",cmd,tmp);
	else
		sprintf(buf,"lpr %s",tmp);

	system(buf);
	unlink(tmp);


}

/*-------------------------------------------------

	Replace the selection by a char*
	If sel the inserted text is selected

--------------------------------------------------*/

void xec_ReplaceTextSelection(Widget w,char *p,Boolean sel)
{
	XmTextPosition	from,to;

	XmTextGetSelectionPosition(w,&from,&to);
	if(from == to) from = to = XmTextGetInsertionPosition(w);
	XmTextReplace(w,from,to,p);
	if(sel)
		XmTextSetSelection(w,from,from+strlen(p),CurrentTime);
	else
		XmTextSetSelection(w,from+strlen(p),from+strlen(p),CurrentTime);
	XmTextSetInsertionPosition(w,from+strlen(p));
}

