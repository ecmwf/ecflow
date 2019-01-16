/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #11 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2019 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/

%{
#if defined(linux) || defined(mips) || defined(hpux) || defined(_AIX)
int yylineno;
#define NEWLINE yylineno++
#else
#define NEWLINE 
#endif

#undef ECHO
/* #define ECHO printf("<%s>",yytext) */
#define ECHO /**/

/*
on HP lex -Xa10000
p4 edit src/menul.c src/menuy.c && cd src && lex  -t menul.l > menul.c && yacc  menuy.y && mv -f y.tab.c menuy.c && cd ../ && make menu && make prof # ./env.sh cc

ecflow_client | xargs addr2line -e ecflow_client -f
*/

#if defined (linux) || defined(alpha) || defined(SVR4) || defined(SGI) || defined(_AIX)
#define yyinput input
#endif


%}

IDENT   [_0-9A-Za-z]+
NUMBER  [0-9]+
NODE    \/[_0-9A-Za-z:\/]*

%option yylineno
%array

%s LOGFILE
%e 1600
%p 8000
%a 15000
%n 5000
%o 10000

%%

!.*$            ;

<INITIAL>version         return VERSION;
<INITIAL>menu            return MENU;
<INITIAL>MENU            return MENU;
<INITIAL>NONE            return NONE;
<INITIAL>ALL             return ALL;
<INITIAL>UNKNOWN         return UNKNOWN;
<INITIAL>SUSPENDED       return SUSPENDED;
<INITIAL>COMPLETE        return COMPLETE;
<INITIAL>QUEUED          return QUEUED;
<INITIAL>SUBMITTED       return SUBMITTED;
<INITIAL>ACTIVE          return ACTIVE;
<INITIAL>ABORTED         return ABORTED;
<INITIAL>CLEAR           return CLEAR;
<INITIAL>SET             return SET;
<INITIAL>SHUTDOWN        return SHUTDOWN;
<INITIAL>HALTED          return HALTED;
<INITIAL>LOCKED          return LOCKED;
<INITIAL>SERVER          return SERVER;
<INITIAL>SMS             return SMS;
<INITIAL>NODE            return NODE;
<INITIAL>SUITE           return SUITE;
<INITIAL>FAMILY          return FAMILY;
<INITIAL>TASK            return TASK;
<INITIAL>EVENT           return EVENT;
<INITIAL>LIMIT           return LIMIT;
<INITIAL>LABEL           return LABEL;
<INITIAL>METER           return METER;
<INITIAL>VARIABLE        return VARIABLE;
<INITIAL>REPEAT          return REPEAT;
<INITIAL>ALIAS           return ALIAS;
<INITIAL>MIGRATED        return MIGRATED;
<INITIAL>HAS_TRIGGERS    return HAS_TRIGGERS;
<INITIAL>HAS_TIME        return HAS_TIME;
<INITIAL>HAS_DATE        return HAS_DATE;
<INITIAL>HAS_TEXT        return HAS_TEXT;
<INITIAL>IS_ZOMBIE       return IS_ZOMBIE;
<INITIAL>SEPARATOR       return SEPARATOR;
<INITIAL>YES             return DEFAULT_YES;
<INITIAL>NO              return DEFAULT_NO;
<INITIAL>WINDOW          return WINDOW;
<INITIAL>PLUG            return PLUG;
<INITIAL>COMP            return COMP;
<INITIAL>SELECTION       return SELECTION;

<INITIAL>USER            return USER;
<INITIAL>OPER            return OPER;
<INITIAL>ADMIN           return ADMIN;

<INITIAL>^LOG:        { BEGIN LOGFILE; }
<INITIAL>^MSG:.*$     { return JUNK; }
<INITIAL>^DBG:.*$     { return JUNK; } 
<INITIAL>^ERR:.*$     { return JUNK; } 
<INITIAL>^WAR:.*$     { return JUNK; } 
<INITIAL>^"# LOG:"     { ECHO; BEGIN LOGFILE; }
<INITIAL>^"# MSG:"     { ECHO; BEGIN LOGFILE; }
<INITIAL>^"# DBG:"     { ECHO; BEGIN LOGFILE; } 
<INITIAL>^"# ERR:"     { ECHO; BEGIN LOGFILE; } 
<INITIAL>^"# WAR:"     { ECHO; BEGIN LOGFILE; } 

<LOGFILE>^LOG:     { }
<LOGFILE>MSG:.*$     {  return JUNK; }
<LOGFILE>DBG:.*$     {  return JUNK; }
<LOGFILE>ERR:.*$     {  return JUNK; }
<LOGFILE>WAR:.*$     {  return JUNK; }

<LOGFILE>^"# LOG:"     ECHO;
<LOGFILE>^"# MSG:.*$"     { ECHO; return JUNK; }
<LOGFILE>^"# DBG:.*$"     { ECHO; return JUNK; }
<LOGFILE>^"# ERR:.*$"     { ECHO; return JUNK; }
<LOGFILE>^"# WAR:.*$"     { ECHO; return JUNK; }


<INITIAL>{IDENT}     return IDENT;

<INITIAL>\" {
		int c,q = yytext[0];
		yyleng = 0;
		while((c = yyinput()) && c != q && c != '\n')
		  if(c == '\\') {
		    yytext[yyleng++] = c;
		    yytext[yyleng++] = yyinput();
		  } else 
                    yytext[yyleng++] = c;
		yytext[yyleng++] = 0;
                return STRING;
	    }

<INITIAL>\' {
		int c,q = yytext[0];
		yyleng = 0;
		while((c = yyinput()) && c != q && c != '\n')
		  if(c == '\\') {
		    yytext[yyleng++] = c;
		    yytext[yyleng++] = yyinput();
		  } else 
                    yytext[yyleng++] = c;
		yytext[yyleng++] = 0;
                return STRING;
	    }
<LOGFILE>complete:       { ECHO; return COMPLETE; }
<LOGFILE>queued:         { ECHO; return QUEUED; }
<LOGFILE>active:         { ECHO; return ACTIVE; }
<LOGFILE>command:Meter   { ECHO; return METER; }
<LOGFILE>command:Event   { ECHO; return EVENT; }
<LOGFILE>unknown:        { return UNKNOWN; }
<LOGFILE>submitted:      { ECHO; return SUBMITTED; }
<LOGFILE>aborted:        { ECHO; return ABORTED; }
<LOGFILE>set:            { ECHO; return SET; }
<LOGFILE>clear:          { ECHO; return CLEAR; }
<LOGFILE>cancel:         { ECHO; return CANCEL; }

<LOGFILE>command:.*$              { ECHO; return JUNK; }
<LOGFILE>check:.*$                { ECHO; return JUNK; }
<LOGFILE>checkpoint:.*$           { ECHO; return JUNK; }
<LOGFILE>label:.*$                { ECHO; return JUNK; }
<LOGFILE>abort:.*$                { ECHO; return JUNK; }
<LOGFILE>midnight:.*$             { ECHO; return JUNK; }
<LOGFILE>"autorepeat date":.*$    { ECHO; return JUNK; }
<LOGFILE>autocancel:.*$           { ECHO; return JUNK; }
<LOGFILE>run:.*$                  { ECHO; return JUNK; }
<LOGFILE>order:.*$                { ECHO; return JUNK; }
<LOGFILE>login:.*$                { ECHO; return JUNK; }
<LOGFILE>logout:.*$               { ECHO; return JUNK; }
<LOGFILE>recover:.*$              { ECHO; return JUNK; }
<LOGFILE>resubmit:.*$             { ECHO; return JUNK; }
<LOGFILE>requeue:.*$              { ECHO; return JUNK; }
<LOGFILE>suspend:.*$              { ECHO; return JUNK; }
<LOGFILE>resume:.*$               { ECHO; return JUNK; }
<LOGFILE>timeout:.*$              { ECHO; return JUNK; }
<LOGFILE>dir:.*$                  { ECHO; return JUNK; }
<LOGFILE>send:.*$                 { ECHO; return JUNK; }
<LOGFILE>send(.*):.*$             { ECHO; return JUNK; }
<LOGFILE>time:.*$                 { ECHO; return JUNK; }
<LOGFILE>manual:.*$               { ECHO; return JUNK; }
<LOGFILE>file:.*$                 { ECHO; return JUNK; }
<LOGFILE>editfile:.*$             { ECHO; return JUNK; }
<LOGFILE>edit:.*$                 { ECHO; return JUNK; }
<LOGFILE>translate:.*$            { ECHO; return JUNK; }
<LOGFILE>force:.*$                { ECHO; return JUNK; }
<LOGFILE>force(.*):.*$            { ECHO; return JUNK; }
<LOGFILE>play:.*$                 { ECHO; return JUNK; }
<LOGFILE>begin:.*$                { ECHO; return JUNK; }
<LOGFILE>late:.*$                 { ECHO; return JUNK; }
<LOGFILE>delete:.*$               { ECHO; return JUNK; }
<LOGFILE>alter:.*$                { ECHO; return JUNK; }
<LOGFILE>event:.*$                { ECHO; return JUNK; }
<LOGFILE>setev:.*$                { ECHO; return JUNK; }
<LOGFILE>dependencies:.*$         { ECHO; return JUNK; }
<LOGFILE>time-used:.*$          { ECHO; return JUNK; }
<LOGFILE>PID.*$                 { ECHO; return JUNK; }
<LOGFILE>--force.*$          { ECHO; return JUNK; }
<LOGFILE>--ping.*$          { ECHO; return JUNK; }
<LOGFILE>--news.*$          { ECHO; return JUNK; }
<LOGFILE>svr:.*$          { ECHO; return JUNK; }

<LOGFILE>" <--- still suspended"              ;
<LOGFILE>" by rule"                           ;
<LOGFILE>command:.*$                { return JUNK; }
<LOGFILE>try-no.*$                  { return JUNK; }

<LOGFILE>{NUMBER}     { ECHO; return NUMBER; }
<LOGFILE>{IDENT}      { ECHO; return IDENT;}
<LOGFILE>{NODE}       { ECHO; return NODE_NAME;}
<LOGFILE>[ \t]        ECHO;
<LOGFILE>.            { ECHO; return *yytext; }
<INITIAL>[ \t]         ;
<INITIAL>.            return *yytext;
\n                    { BEGIN INITIAL; NEWLINE; }
<LOGFILE>job_size:.*$          { return ANY; }

%
