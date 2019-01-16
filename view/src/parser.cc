
#ifndef parser_H
#include "parser.h"
#endif

#ifndef gui_H
#include "gui.h"
#endif

#ifndef menus_H
#include "menus.h"
#endif

#ifndef log_event_H
#include "log_event.h"
#endif

#if 0
#ifndef __sgi
inline const char* gettxt(const char* a,const char* b) { return b; }
#endif
#endif

/* #define YYDEBUG */
/* #ifdef linux
#undef YYDEBUG
#endif
*/
#ifndef error_H
#include "error.h"
#endif

//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================


extern "C" {
   extern int yylineno;
	extern int yydebug;
	extern int yyone;
	extern FILE* yyin; 
	extern int yyparse();
}

void parser::parse(const char* fname)
{
	char buf[1024];

	yydebug  = getenv("YYDEBUG") != 0;
	yylineno = 0;

	int z = strlen(fname);
	if(fname[z-1] == 'Z' && fname[z-2] == '.')
	{
		sprintf(buf,"|zcat %s",fname);
		fname = buf;
	}

	if(fname[z-1] == 'z' && fname[z-2] == 'g' && fname[z-3] == '.')
	{
		sprintf(buf,"|zcat %s",fname);
		fname = buf;
	}

	yyin = (fname[0] == '|') ? popen(fname+1,"r") : fopen(fname, "r");
	if(yyin)
	{
		yyparse();
		if(fname[0] == '|') pclose(yyin); else fclose(yyin);
	}
	else gui::syserr(fname);
}

void parser::parse(FILE *f)
{
	extern int yylineno;
	yylineno = 0;
	yydebug  = getenv("YYDEBUG") != 0;
	yyin = f;
	yyparse();
}

void parser::parse1(FILE *f)
{
	yyone = 1;
	yydebug  = getenv("YYDEBUG") != 0;
	yyparse();
	yyone = 0;
}
