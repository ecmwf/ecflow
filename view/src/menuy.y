/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #17 $                                                                    */
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

%{
#include "lexyacc.h"
#include "ecflow.h"
int yyone = 0;
extern int yylineno;

#ifdef __cplusplus
 void yylex(void);
#endif

#if defined(_AIX) 
 extern char yytext[];
 int yydebug;
#elif defined(hpux) 
 void yyerror(char * msg);
 int yydebug;
#elif defined(YYBISON) 
 extern char yytext[]; /* ARRAY */
 void yyerror(char * msg);
 int yydebug;
#elif defined (linux)
 extern char yytext[];
 void yyerror(char * msg);
 int yydebug;
#elif defined(alpha) || defined(SGI) 
 extern char yytext[]; /* ARRAY */
#elif defined(SVR4) 
 extern char yytext[]; /* ARRAY */
#elif defined(alpha) 
 extern unsigned char yytext[];
#else
 extern char yytext[];
 void yyerror(char * msg);
 int yydebug;
#endif
%}

%token VERSION
%token MENU
%token IDENT
%token NONE
%token ALL
%token UNKNOWN
%token SUSPENDED
%token COMPLETE
%token QUEUED
%token SUBMITTED
%token ACTIVE
%token ABORTED
%token CLEAR
%token SET
%token SHUTDOWN
%token HALTED
%token LOCKED
%token MIGRATED
%token SELECTION

%token SERVER
%token SMS
%token NODE
%token SUITE
%token FAMILY
%token TASK
%token EVENT
%token LABEL
%token METER
%token REPEAT
%token VARIABLE
%token TRIGGER
%token LIMIT
%token ALIAS

%token HAS_TRIGGERS
%token HAS_TIME
%token HAS_DATE
%token HAS_TEXT

%token IS_ZOMBIE

%token SEPARATOR
%token STRING
%token DEFAULT_YES
%token DEFAULT_NO

%token WINDOW
%token PLUG
%token COMP

%token USER
%token OPER
%token ADMIN


%token NUMBER
%token JUNK
%token EOL
%token NODE_NAME
%token CANCEL
%token ANY

%type<flg> flags;
%type<flg> flag;

%type<str> question;
%type<str> title;
%type<act> action;
%type<str> name;
%type<num> answer;
%type<num> version;

%type<itm> menu_items;
%type<itm> menu_item;

%type<men> menu;
%type<men> menus;

%type<nod> node_name;
%type<num> number;

%type<dti> date;


%start first

%union {
	char       *str;
	long        num;
	flags      *flg;
	item       *itm;
	menu       *men;
	action     *act;
	node*       nod;
	double      dbl;
	DateTime    dti;
};


%%

first : version menus { menus_root($2); }
      | menus { menus_root($1); }
      | junk
      | logfile
      ;

version : VERSION name name name ';'
          { $$ = menus_version(atol($2), atol($3), atol($4)); } 
          ;

menus : menu menus { $$ = menus_chain_menus($1,$2); }
	  | menu
	  | menu error { $$ = $1; }
	  ;

menu : MENU name '{' menu_items '}' { $$ = menus_create_2($2,$4); }
     ;

name : IDENT  { $$ = strdup(yytext); }
     | STRING { $$ = strdup(yytext); }
     ;

menu_items : menu_item menu_items { $$ = menus_chain_items($1,$2); }
		   | menu_item
		   ;

menu_item : '(' flags ',' flags ',' title ',' action ',' question ',' answer ')'
			{ $$ = menus_create_6($2,$4,$6,$8,$10,$12); }
		| '('  flags ',' flags ','  title ',' action ')'
			{ $$ = menus_create_6($2,$4,$6,$8,"",1); }
          ;

flag         : '~' flag        { $$ = new_flagNot($2);        }
  | NONE            { $$ = new_flagNone();       }
  | ALL             { $$ = new_flagAll();        }
  | UNKNOWN         { $$ = new_statusFlag(STATUS_UNKNOWN);    }
  | SUSPENDED       { $$ = new_statusFlag(STATUS_SUSPENDED);  }
  | COMPLETE        { $$ = new_statusFlag(STATUS_COMPLETE);   }
  | QUEUED          { $$ = new_statusFlag(STATUS_QUEUED);     }
  | SUBMITTED       { $$ = new_statusFlag(STATUS_SUBMITTED);  }
  | ACTIVE          { $$ = new_statusFlag(STATUS_ACTIVE);     }
  | ABORTED         { $$ = new_statusFlag(STATUS_ABORTED);    }
  | CLEAR           { $$ = new_eventFlag(0);      }
  | SET             { $$ = new_eventFlag(1);        }
  | SHUTDOWN        { $$ = new_statusFlag(STATUS_SHUTDOWN);   }
  | HALTED          { $$ = new_statusFlag(STATUS_HALTED);     }
  | SERVER         { $$ = new_typeFlag(NODE_SUPER) ; }
  | SMS            { $$ = new_typeFlag(NODE_SUPER) ; }
  | SUITE          { $$ = new_typeFlag(NODE_SUITE) ; }
  | FAMILY         { $$ = new_typeFlag(NODE_FAMILY) ; }
  | TASK           { $$ = new_typeFlag(NODE_TASK) ; }
  | NODE           { $$ = new_flagOr(new_flagOr(new_typeFlag(NODE_SUITE),new_typeFlag(NODE_FAMILY)),new_typeFlag(NODE_TASK)); }
  | EVENT          { $$ = new_typeFlag(NODE_EVENT) ; }
  | LABEL          { $$ = new_typeFlag(NODE_LABEL) ; }
  | METER          { $$ = new_typeFlag(NODE_METER) ; }
  | REPEAT         { $$ = new_typeFlag(NODE_REPEAT) ; }
  | VARIABLE       { $$ = new_typeFlag(NODE_VARIABLE) ; }
  | TRIGGER        { $$ = new_typeFlag(NODE_TRIGGER) ; }
  | ALIAS          { $$ = new_typeFlag(NODE_ALIAS) ; }
  | LIMIT          { $$ = new_typeFlag(NODE_LIMIT) ; }
  | HAS_TRIGGERS   { $$ = new_procFlag_node_hasTriggers(); }
  | HAS_DATE       { $$ = new_procFlag_node_hasDate(); }
  | HAS_TIME       { $$ = new_procFlag_node_hasTime(); }
  | HAS_TEXT       { $$ = new_procFlag_node_hasText(); }
  | IS_ZOMBIE      { $$ = new_procFlag_node_isZombie(); }
  | MIGRATED       { $$ = new_procFlag_node_isMigrated(); }
  | LOCKED       { $$ = new_procFlag_node_isLocked(); }
  | USER           { $$ = new_userFlag(0); }
  | OPER           { $$ = new_userFlag(1); }
  | ADMIN          { $$ = new_userFlag(2); }
  | SELECTION       { $$ = new_selectionFlag(); }
  | '(' flags ')'   { $$ = $2;                     }
;

flags        : flag '|' flags { $$ = new_flagOr($1,$3); }
  | flag '&' flags { $$ = new_flagAnd($1,$3); }
  | flag
			 ;

title : STRING { $$ = strdup(yytext); }
      ;

action  : STRING      { $$ = menus_command(strdup(yytext));  }
        | SEPARATOR   { $$ = menus_separator();              }
	| MENU        { $$ = menus_sub_menu(); }
	| WINDOW '(' name ')'  { $$ = menus_window($3); }
	| PLUG        { $$ = menus_internal_host_plug(); }
        | COMP '(' name ',' name ')' { $$ = menus_internal_host_comp($3, $5); }
	;

question: STRING { $$ = strdup(yytext); }
		;

answer  : DEFAULT_YES    { $$ = 1; }
        | DEFAULT_NO     { $$ = 0; }
		;

/*-------------------------------------------------------------*/

logfile : loglines 
		;

loglines: logline
	| loglines logline
        ;

logline : 
| event number number { if(yyone) return 0; }
| junk
| error  { /* extern int yylineno; printf("error line %d [%s]\n",yylineno, yytext); */ }
;

date   : '[' number ':' number ':' number  number '.' number '.' number ']'
          { 
	    $$.date = ($11 * 10000 + $9 * 100 + $7);
	    $$.time = ($2  * 10000 + $4 * 100 + $6);
	  }
          ;

event : date COMPLETE   node_name  { log_event_status_event(&$1,$3,STATUS_COMPLETE); }
  | date QUEUED     node_name  { log_event_status_event(&$1,$3,STATUS_QUEUED); }
  | date ACTIVE     node_name  { log_event_status_event(&$1,$3,STATUS_ACTIVE); }
  | date SUBMITTED  node_name ANY
    { log_event_status_event(&$1,$3,STATUS_SUBMITTED); }
  | date SUBMITTED  node_name 
    { log_event_status_event(&$1,$3,STATUS_SUBMITTED); }
  | date ABORTED    node_name  { log_event_status_event(&$1,$3,STATUS_ABORTED);}
  | date UNKNOWN    node_name  { log_event_status_event(&$1,$3,STATUS_UNKNOWN);}
  | date SET        node_name  { log_event_event_event(&$1,$3,1); }
  | date CLEAR      node_name  { log_event_event_event(&$1,$3,0); }
  | date METER '[' node_name number node_name ']'  { log_event_meter_event(&$1,$6,$5); }
;

junk  : date JUNK
      | JUNK
      ;

node_name : NODE_NAME { $$ = log_event_find(yytext); }
		  ;

number    : NUMBER { $$ = atol(yytext); }
		  ;

%%

#include "menul.c"

int yywrap() {
	return 1;
}

#ifdef AIX
int yyerror(char * msg)
#else
void yyerror(char * msg)
#endif
{
  /* printf("!menu parsing issue?\n");
     printf("%s line %d last token <%s>\n",msg,yylineno,yytext); */
  if (!strncmp("MSG:", yytext, 4)) {} 
  else if (!strncmp("DBG:", yytext, 4)) {}
  else if (!strncmp("ERR:", yytext, 4)) {}
  else if (!strncmp("WAR:", yytext, 4)) {}
  else if (!strncmp("try-no:", yytext, 6)) {}
  else if (!strncmp("File", yytext, 4)) {}
  else if (!strncmp("Variable", yytext, 8)) {}
  else if (!strncmp("Directory", yytext, 9)) {}
  else if (!strncmp("Search", yytext, 6)) {}
  else if (*yytext == '[') {} 
  else if (*yytext == ':') {} 
  else if (*yytext == '/') {} 
  else printf("!%s:%d:<%s>\n",msg,yylineno,yytext);
}

