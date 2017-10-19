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

/*=====================

Include regexp once for compatibility

======================*/

#include <stdio.h>

#define ESIZE 1024
#define NO_REGEXP
#ifdef NO_REGEXP

static char *loc1,*loc2;
static int len;

static compile(w,buf,end,dummy)
char *w,*buf;
int end;
int dummy;
{
    strcpy(buf,w);
    len = strlen(w);
}

static step(w,buf)
char *w;
char *buf;
{
    loc1 = w;
    while(*loc1)
    {
        if(strncmp(loc1,buf,len) == 0)
        {
            loc2 = loc1+len;
            return 1;
        }
        loc1++;
    }
    return 0;
}


#else

/* size of regexp buffer */


#define INIT        register char *sp = instring;
#define GETC()      (*sp++)
#define PEEKC()     (*sp)
#define UNGETC(c)   (--sp)
#define RETURN(c)   return NULL; 
#define ERROR(c)    fprintf(stderr,"Warning regexp error %d\n",c)

#include <regexp.h>

#endif

static char  expbuf[ESIZE];
char *xec_loc1;
char *xec_loc2;

void xec_compile(w)
char *w;
{
	compile(w,expbuf,&expbuf[ESIZE],'\0');
}

int xec_step(p)
char *p;
{
	int s =  step(p,expbuf);
	xec_loc1 = loc1;
	xec_loc2 = loc2;
	return s;
}
