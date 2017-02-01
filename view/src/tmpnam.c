/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #3 $                                                                    */
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

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdio.h>

/*
 * Use /tmp instead of /usr/tmp, because L_tmpname is only 14 chars
 * on some machines (like NeXT machines) and /usr/tmp will cause
 * buffer overflows.
 */

#ifdef P_tmpdir
#   undef P_tmpdir
#endif
#define	P_tmpdir	"/tmp"

char *
tmpnam(s)
	char *s;
{
	static char name[50];
        char * file_name;
	char *mktemp();

	if (!s)
		s = name;

	file_name = getenv ("TMPDIR");

	if (file_name && strlen (file_name) < 42)
	  (void)sprintf(s, "%s/XXXXXX", file_name);	  
	else
	  (void)sprintf(s, "%s/XXXXXX", P_tmpdir);

	if (mkstemp(s))
	  return s;
	else
	  return NULL;
}

#if UTEST

int main (int argc, char ** argv) 
{
  char *name = tmpnam (NULL);
  printf ("%s\n", name);
};
#endif
