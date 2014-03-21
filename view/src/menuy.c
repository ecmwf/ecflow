
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 16 "menuy.y"

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


/* Line 189 of yacc.c  */
#line 111 "y.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     VERSION = 258,
     MENU = 259,
     IDENT = 260,
     NONE = 261,
     ALL = 262,
     UNKNOWN = 263,
     SUSPENDED = 264,
     COMPLETE = 265,
     QUEUED = 266,
     SUBMITTED = 267,
     ACTIVE = 268,
     ABORTED = 269,
     CLEAR = 270,
     SET = 271,
     SHUTDOWN = 272,
     HALTED = 273,
     LOCKED = 274,
     MIGRATED = 275,
     SELECTION = 276,
     SERVER = 277,
     SMS = 278,
     NODE = 279,
     SUITE = 280,
     FAMILY = 281,
     TASK = 282,
     EVENT = 283,
     LABEL = 284,
     METER = 285,
     REPEAT = 286,
     VARIABLE = 287,
     TRIGGER = 288,
     LIMIT = 289,
     ALIAS = 290,
     HAS_TRIGGERS = 291,
     HAS_TIME = 292,
     HAS_DATE = 293,
     HAS_TEXT = 294,
     IS_ZOMBIE = 295,
     SEPARATOR = 296,
     STRING = 297,
     DEFAULT_YES = 298,
     DEFAULT_NO = 299,
     WINDOW = 300,
     PLUG = 301,
     COMP = 302,
     USER = 303,
     OPER = 304,
     ADMIN = 305,
     NUMBER = 306,
     JUNK = 307,
     NODE_NAME = 308,
     CANCEL = 309,
     EOL = 310
   };
#endif
/* Tokens.  */
#define VERSION 258
#define MENU 259
#define IDENT 260
#define NONE 261
#define ALL 262
#define UNKNOWN 263
#define SUSPENDED 264
#define COMPLETE 265
#define QUEUED 266
#define SUBMITTED 267
#define ACTIVE 268
#define ABORTED 269
#define CLEAR 270
#define SET 271
#define SHUTDOWN 272
#define HALTED 273
#define LOCKED 274
#define MIGRATED 275
#define SELECTION 276
#define SERVER 277
#define SMS 278
#define NODE 279
#define SUITE 280
#define FAMILY 281
#define TASK 282
#define EVENT 283
#define LABEL 284
#define METER 285
#define REPEAT 286
#define VARIABLE 287
#define TRIGGER 288
#define LIMIT 289
#define ALIAS 290
#define HAS_TRIGGERS 291
#define HAS_TIME 292
#define HAS_DATE 293
#define HAS_TEXT 294
#define IS_ZOMBIE 295
#define SEPARATOR 296
#define STRING 297
#define DEFAULT_YES 298
#define DEFAULT_NO 299
#define WINDOW 300
#define PLUG 301
#define COMP 302
#define USER 303
#define OPER 304
#define ADMIN 305
#define NUMBER 306
#define JUNK 307
#define NODE_NAME 308
#define CANCEL 309
#define EOL 310




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 139 "menuy.y"

	char       *str;
	long        num;
	flags      *flg;
	item       *itm;
	menu       *men;
	action     *act;
	node*       nod;
	double      dbl;
	DateTime    dti;



/* Line 214 of yacc.c  */
#line 271 "y.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 283 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  22
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   198

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  69
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  22
/* YYNRULES -- Number of rules.  */
#define YYNRULES  90
/* YYNRULES -- Number of states.  */
#define YYNSTATES  152

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   310

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    64,     2,
      59,    61,     2,     2,    60,     2,    67,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    66,    56,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    65,     2,    68,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    57,    63,    58,    62,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,    10,    12,    18,    21,    23,
      26,    32,    34,    36,    39,    41,    55,    65,    68,    70,
      72,    74,    76,    78,    80,    82,    84,    86,    88,    90,
      92,    94,    96,    98,   100,   102,   104,   106,   108,   110,
     112,   114,   116,   118,   120,   122,   124,   126,   128,   130,
     132,   134,   136,   138,   140,   142,   144,   148,   152,   156,
     158,   160,   162,   164,   166,   171,   173,   180,   182,   184,
     186,   188,   190,   193,   194,   198,   200,   202,   215,   219,
     223,   227,   231,   235,   239,   243,   247,   255,   258,   260,
     262
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      70,     0,    -1,    71,    72,    -1,    72,    -1,    88,    -1,
      83,    -1,     3,    74,    74,    74,    56,    -1,    73,    72,
      -1,    73,    -1,    73,     1,    -1,     4,    74,    57,    75,
      58,    -1,     5,    -1,    42,    -1,    76,    75,    -1,    76,
      -1,    59,    78,    60,    78,    60,    79,    60,    80,    60,
      81,    60,    82,    61,    -1,    59,    78,    60,    78,    60,
      79,    60,    80,    61,    -1,    62,    77,    -1,     6,    -1,
       7,    -1,     8,    -1,     9,    -1,    10,    -1,    11,    -1,
      12,    -1,    13,    -1,    14,    -1,    15,    -1,    16,    -1,
      17,    -1,    18,    -1,    22,    -1,    23,    -1,    25,    -1,
      26,    -1,    27,    -1,    24,    -1,    28,    -1,    29,    -1,
      30,    -1,    31,    -1,    32,    -1,    33,    -1,    35,    -1,
      34,    -1,    36,    -1,    38,    -1,    37,    -1,    39,    -1,
      40,    -1,    20,    -1,    19,    -1,    48,    -1,    49,    -1,
      50,    -1,    21,    -1,    59,    78,    61,    -1,    77,    63,
      78,    -1,    77,    64,    78,    -1,    77,    -1,    42,    -1,
      42,    -1,    41,    -1,     4,    -1,    45,    59,    74,    61,
      -1,    46,    -1,    47,    59,    74,    60,    74,    61,    -1,
      42,    -1,    43,    -1,    44,    -1,    84,    -1,    85,    -1,
      84,    85,    -1,    -1,    87,    90,    90,    -1,    88,    -1,
       1,    -1,    65,    90,    66,    90,    66,    90,    90,    67,
      90,    67,    90,    68,    -1,    86,    10,    89,    -1,    86,
      11,    89,    -1,    86,    13,    89,    -1,    86,    12,    89,
      -1,    86,    14,    89,    -1,    86,     8,    89,    -1,    86,
      16,    89,    -1,    86,    15,    89,    -1,    86,    30,    65,
      89,    90,    89,    68,    -1,    86,    52,    -1,    52,    -1,
      53,    -1,    51,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   154,   154,   155,   156,   157,   160,   164,   165,   166,
     169,   172,   173,   176,   177,   180,   182,   186,   187,   188,
     189,   190,   191,   192,   193,   194,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,   205,   206,   207,   208,
     209,   210,   211,   212,   213,   214,   215,   216,   217,   218,
     219,   220,   221,   222,   223,   224,   225,   228,   229,   230,
     233,   236,   237,   238,   239,   240,   241,   244,   247,   248,
     253,   256,   257,   260,   261,   262,   263,   266,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   284,   285,   288,
     291
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "VERSION", "MENU", "IDENT", "NONE",
  "ALL", "UNKNOWN", "SUSPENDED", "COMPLETE", "QUEUED", "SUBMITTED",
  "ACTIVE", "ABORTED", "CLEAR", "SET", "SHUTDOWN", "HALTED", "LOCKED",
  "MIGRATED", "SELECTION", "SERVER", "SMS", "NODE", "SUITE", "FAMILY",
  "TASK", "EVENT", "LABEL", "METER", "REPEAT", "VARIABLE", "TRIGGER",
  "LIMIT", "ALIAS", "HAS_TRIGGERS", "HAS_TIME", "HAS_DATE", "HAS_TEXT",
  "IS_ZOMBIE", "SEPARATOR", "STRING", "DEFAULT_YES", "DEFAULT_NO",
  "WINDOW", "PLUG", "COMP", "USER", "OPER", "ADMIN", "NUMBER", "JUNK",
  "NODE_NAME", "CANCEL", "EOL", "';'", "'{'", "'}'", "'('", "','", "')'",
  "'~'", "'|'", "'&'", "'['", "':'", "'.'", "']'", "$accept", "first",
  "version", "menus", "menu", "name", "menu_items", "menu_item", "flag",
  "flags", "title", "action", "question", "answer", "logfile", "loglines",
  "logline", "date", "event", "junk", "node_name", "number", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,    59,   123,   125,    40,
      44,    41,   126,   124,    38,    91,    58,    46,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    69,    70,    70,    70,    70,    71,    72,    72,    72,
      73,    74,    74,    75,    75,    76,    76,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    78,    78,    78,
      79,    80,    80,    80,    80,    80,    80,    81,    82,    82,
      83,    84,    84,    85,    85,    85,    85,    86,    87,    87,
      87,    87,    87,    87,    87,    87,    87,    88,    88,    89,
      90
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     5,     2,     1,     2,
       5,     1,     1,     2,     1,    13,     9,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     3,     3,     1,
       1,     1,     1,     1,     4,     1,     6,     1,     1,     1,
       1,     1,     2,     0,     3,     1,     1,    12,     3,     3,
       3,     3,     3,     3,     3,     3,     7,     2,     1,     1,
       1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    76,     0,     0,    88,     0,     0,     0,     3,     0,
       5,     0,    71,     0,     0,    75,    11,    12,     0,     0,
      90,     0,     1,     2,     9,     7,    72,    75,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    87,     0,     0,
       0,     0,    89,    83,    78,    79,    81,    80,    82,    85,
      84,     0,    74,     0,     0,     0,    14,     0,     0,     6,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    51,    50,    55,    31,    32,    36,    33,
      34,    35,    37,    38,    39,    40,    41,    42,    44,    43,
      45,    47,    46,    48,    49,    52,    53,    54,     0,     0,
      59,     0,    10,    13,     0,     0,     0,    17,     0,     0,
       0,     0,     0,    56,    57,    58,     0,     0,    86,     0,
       0,    60,     0,     0,     0,     0,    63,    62,    61,     0,
      65,     0,     0,     0,     0,     0,     0,    16,    77,     0,
       0,    67,     0,    64,     0,     0,     0,    68,    69,     0,
      66,    15
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     6,     7,     8,     9,    18,    55,    56,   100,   101,
     122,   132,   142,   149,    10,    11,    12,    13,    14,    15,
      43,    21
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -46
static const yytype_int16 yypact[] =
{
       9,   -46,     6,     6,   -46,   -31,    26,    39,   -46,    18,
     -46,    16,   -46,    19,   -31,    44,   -46,   -46,     6,   -11,
     -46,   -19,   -46,   -46,   -46,   -46,   -46,   -46,     1,     1,
       1,     1,     1,     1,     1,     1,   -15,   -46,   -31,     6,
      -1,   -31,   -46,   -46,   -46,   -46,   -46,   -46,   -46,   -46,
     -46,     1,   -46,     3,   136,     2,    -1,    -4,   -31,   -46,
     -46,   -46,   -46,   -46,   -46,   -46,   -46,   -46,   -46,   -46,
     -46,   -46,   -46,   -46,   -46,   -46,   -46,   -46,   -46,   -46,
     -46,   -46,   -46,   -46,   -46,   -46,   -46,   -46,   -46,   -46,
     -46,   -46,   -46,   -46,   -46,   -46,   -46,   -46,   136,   136,
     -26,     7,   -46,   -46,   -31,     1,     5,   -46,   136,   136,
     136,   -31,     4,   -46,   -46,   -46,    13,     8,   -46,    27,
     -31,   -46,    17,    12,    10,   -31,   -46,   -46,   -46,    11,
     -46,    21,   -21,    15,     6,     6,    34,   -46,   -46,    23,
      22,   -46,    25,   -46,     6,    -2,    28,   -46,   -46,    29,
     -46,   -46
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -46,   -46,   -46,    14,   -46,    -3,    30,   -46,   -12,   -45,
     -46,   -46,   -46,   -46,   -46,   -46,    77,   -46,   -46,    81,
     -27,   -13
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -74
static const yytype_int16 yytable[] =
{
      19,    38,    44,    45,    46,    47,    48,    49,    50,   -73,
       1,    16,     2,     3,   126,    39,   -70,     1,    -8,    24,
      20,    23,     3,    25,    58,    52,    22,    28,    57,    29,
      30,    31,    32,    33,    34,    35,    53,   108,   109,   136,
     137,   147,   148,     3,    -4,   105,    40,    41,    17,    36,
      51,   127,   128,   106,    42,   129,   130,   131,    54,    59,
     102,     4,   104,   114,   115,   116,   113,   110,     4,   121,
     134,    37,   118,   119,     5,   120,   141,   124,   112,   125,
     135,     5,   144,   138,   143,   145,   103,   107,    26,   150,
     151,   111,    27,     0,     0,     0,     0,     0,   117,     0,
       0,     0,     0,     0,     0,     0,     0,   123,     0,     0,
       0,     0,   133,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   139,   140,     0,     0,     0,     0,     0,     0,     0,
       0,   146,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,     0,     0,     0,
       0,     0,     0,     0,    95,    96,    97,     0,     0,     0,
       0,     0,     0,     0,     0,    98,     0,     0,    99
};

static const yytype_int16 yycheck[] =
{
       3,    14,    29,    30,    31,    32,    33,    34,    35,     0,
       1,     5,     3,     4,     4,    18,     0,     1,     0,     1,
      51,     7,     4,     9,    51,    38,     0,     8,    41,    10,
      11,    12,    13,    14,    15,    16,    39,    63,    64,    60,
      61,    43,    44,     4,     0,    58,    57,    66,    42,    30,
      65,    41,    42,    98,    53,    45,    46,    47,    59,    56,
      58,    52,    66,   108,   109,   110,    61,    60,    52,    42,
      59,    52,    68,    60,    65,    67,    42,    60,   105,    67,
      59,    65,    60,    68,    61,    60,    56,    99,    11,    61,
      61,   104,    11,    -1,    -1,    -1,    -1,    -1,   111,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,    -1,    -1,
      -1,    -1,   125,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   144,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    48,    49,    50,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    59,    -1,    -1,    62
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,     3,     4,    52,    65,    70,    71,    72,    73,
      83,    84,    85,    86,    87,    88,     5,    42,    74,    74,
      51,    90,     0,    72,     1,    72,    85,    88,     8,    10,
      11,    12,    13,    14,    15,    16,    30,    52,    90,    74,
      57,    66,    53,    89,    89,    89,    89,    89,    89,    89,
      89,    65,    90,    74,    59,    75,    76,    90,    89,    56,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    48,    49,    50,    59,    62,
      77,    78,    58,    75,    66,    90,    78,    77,    63,    64,
      60,    90,    89,    61,    78,    78,    78,    90,    68,    60,
      67,    42,    79,    90,    60,    67,     4,    41,    42,    45,
      46,    47,    80,    90,    59,    59,    60,    61,    68,    74,
      74,    42,    81,    61,    60,    60,    74,    43,    44,    82,
      61,    61
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 154 "menuy.y"
    { menus_root((yyvsp[(2) - (2)].men)); }
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 155 "menuy.y"
    { menus_root((yyvsp[(1) - (1)].men)); }
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 161 "menuy.y"
    { (yyval.num) = menus_version(atol((yyvsp[(2) - (5)].str)), atol((yyvsp[(3) - (5)].str)), atol((yyvsp[(4) - (5)].str))); }
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 164 "menuy.y"
    { (yyval.men) = menus_chain_menus((yyvsp[(1) - (2)].men),(yyvsp[(2) - (2)].men)); }
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 166 "menuy.y"
    { (yyval.men) = (yyvsp[(1) - (2)].men); }
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 169 "menuy.y"
    { (yyval.men) = menus_create_2((yyvsp[(2) - (5)].str),(yyvsp[(4) - (5)].itm)); }
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 172 "menuy.y"
    { (yyval.str) = strdup(yytext); }
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 173 "menuy.y"
    { (yyval.str) = strdup(yytext); }
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 176 "menuy.y"
    { (yyval.itm) = menus_chain_items((yyvsp[(1) - (2)].itm),(yyvsp[(2) - (2)].itm)); }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 181 "menuy.y"
    { (yyval.itm) = menus_create_6((yyvsp[(2) - (13)].flg),(yyvsp[(4) - (13)].flg),(yyvsp[(6) - (13)].str),(yyvsp[(8) - (13)].act),(yyvsp[(10) - (13)].str),(yyvsp[(12) - (13)].num)); }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 183 "menuy.y"
    { (yyval.itm) = menus_create_6((yyvsp[(2) - (9)].flg),(yyvsp[(4) - (9)].flg),(yyvsp[(6) - (9)].str),(yyvsp[(8) - (9)].act),"",1); }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 186 "menuy.y"
    { (yyval.flg) = new_flagNot((yyvsp[(2) - (2)].flg));        }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 187 "menuy.y"
    { (yyval.flg) = new_flagNone();       }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 188 "menuy.y"
    { (yyval.flg) = new_flagAll();        }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 189 "menuy.y"
    { (yyval.flg) = new_statusFlag(STATUS_UNKNOWN);    }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 190 "menuy.y"
    { (yyval.flg) = new_statusFlag(STATUS_SUSPENDED);  }
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 191 "menuy.y"
    { (yyval.flg) = new_statusFlag(STATUS_COMPLETE);   }
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 192 "menuy.y"
    { (yyval.flg) = new_statusFlag(STATUS_QUEUED);     }
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 193 "menuy.y"
    { (yyval.flg) = new_statusFlag(STATUS_SUBMITTED);  }
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 194 "menuy.y"
    { (yyval.flg) = new_statusFlag(STATUS_ACTIVE);     }
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 195 "menuy.y"
    { (yyval.flg) = new_statusFlag(STATUS_ABORTED);    }
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 196 "menuy.y"
    { (yyval.flg) = new_eventFlag(0);      }
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 197 "menuy.y"
    { (yyval.flg) = new_eventFlag(1);        }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 198 "menuy.y"
    { (yyval.flg) = new_statusFlag(STATUS_SHUTDOWN);   }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 199 "menuy.y"
    { (yyval.flg) = new_statusFlag(STATUS_HALTED);     }
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 200 "menuy.y"
    { (yyval.flg) = new_typeFlag(NODE_SUPER) ; }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 201 "menuy.y"
    { (yyval.flg) = new_typeFlag(NODE_SUPER) ; }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 202 "menuy.y"
    { (yyval.flg) = new_typeFlag(NODE_SUITE) ; }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 203 "menuy.y"
    { (yyval.flg) = new_typeFlag(NODE_FAMILY) ; }
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 204 "menuy.y"
    { (yyval.flg) = new_typeFlag(NODE_TASK) ; }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 205 "menuy.y"
    { (yyval.flg) = new_flagOr(new_flagOr(new_typeFlag(NODE_SUITE),new_typeFlag(NODE_FAMILY)),new_typeFlag(NODE_TASK)); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 206 "menuy.y"
    { (yyval.flg) = new_typeFlag(NODE_EVENT) ; }
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 207 "menuy.y"
    { (yyval.flg) = new_typeFlag(NODE_LABEL) ; }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 208 "menuy.y"
    { (yyval.flg) = new_typeFlag(NODE_METER) ; }
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 209 "menuy.y"
    { (yyval.flg) = new_typeFlag(NODE_REPEAT) ; }
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 210 "menuy.y"
    { (yyval.flg) = new_typeFlag(NODE_VARIABLE) ; }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 211 "menuy.y"
    { (yyval.flg) = new_typeFlag(NODE_TRIGGER) ; }
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 212 "menuy.y"
    { (yyval.flg) = new_typeFlag(NODE_ALIAS) ; }
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 213 "menuy.y"
    { (yyval.flg) = new_typeFlag(NODE_LIMIT) ; }
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 214 "menuy.y"
    { (yyval.flg) = new_procFlag_node_hasTriggers(); }
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 215 "menuy.y"
    { (yyval.flg) = new_procFlag_node_hasDate(); }
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 216 "menuy.y"
    { (yyval.flg) = new_procFlag_node_hasTime(); }
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 217 "menuy.y"
    { (yyval.flg) = new_procFlag_node_hasText(); }
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 218 "menuy.y"
    { (yyval.flg) = new_procFlag_node_isZombie(); }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 219 "menuy.y"
    { (yyval.flg) = new_procFlag_node_isMigrated(); }
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 220 "menuy.y"
    { (yyval.flg) = new_procFlag_node_isLocked(); }
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 221 "menuy.y"
    { (yyval.flg) = new_userFlag(0); }
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 222 "menuy.y"
    { (yyval.flg) = new_userFlag(1); }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 223 "menuy.y"
    { (yyval.flg) = new_userFlag(2); }
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 224 "menuy.y"
    { (yyval.flg) = new_selectionFlag(); }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 225 "menuy.y"
    { (yyval.flg) = (yyvsp[(2) - (3)].flg);                     }
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 228 "menuy.y"
    { (yyval.flg) = new_flagOr((yyvsp[(1) - (3)].flg),(yyvsp[(3) - (3)].flg)); }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 229 "menuy.y"
    { (yyval.flg) = new_flagAnd((yyvsp[(1) - (3)].flg),(yyvsp[(3) - (3)].flg)); }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 233 "menuy.y"
    { (yyval.str) = strdup(yytext); }
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 236 "menuy.y"
    { (yyval.act) = menus_command(strdup(yytext));  }
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 237 "menuy.y"
    { (yyval.act) = menus_separator();              }
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 238 "menuy.y"
    { (yyval.act) = menus_sub_menu(); }
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 239 "menuy.y"
    { (yyval.act) = menus_window((yyvsp[(3) - (4)].str)); }
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 240 "menuy.y"
    { (yyval.act) = menus_internal_host_plug(); }
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 241 "menuy.y"
    { (yyval.act) = menus_internal_host_comp((yyvsp[(3) - (6)].str), (yyvsp[(5) - (6)].str)); }
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 244 "menuy.y"
    { (yyval.str) = strdup(yytext); }
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 247 "menuy.y"
    { (yyval.num) = 1; }
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 248 "menuy.y"
    { (yyval.num) = 0; }
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 261 "menuy.y"
    { if(yyone) return 0; }
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 263 "menuy.y"
    { /* extern int yylineno; printf("error line %d [%s]\n",yylineno, yytext); */ }
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 267 "menuy.y"
    { 
	    (yyval.dti).date = ((yyvsp[(11) - (12)].num) * 10000 + (yyvsp[(9) - (12)].num) * 100 + (yyvsp[(7) - (12)].num));
	    (yyval.dti).time = ((yyvsp[(2) - (12)].num)  * 10000 + (yyvsp[(4) - (12)].num) * 100 + (yyvsp[(6) - (12)].num));
	  }
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 273 "menuy.y"
    { log_event_status_event(&(yyvsp[(1) - (3)].dti),(yyvsp[(3) - (3)].nod),STATUS_COMPLETE); }
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 274 "menuy.y"
    { log_event_status_event(&(yyvsp[(1) - (3)].dti),(yyvsp[(3) - (3)].nod),STATUS_QUEUED); }
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 275 "menuy.y"
    { log_event_status_event(&(yyvsp[(1) - (3)].dti),(yyvsp[(3) - (3)].nod),STATUS_ACTIVE); }
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 276 "menuy.y"
    { log_event_status_event(&(yyvsp[(1) - (3)].dti),(yyvsp[(3) - (3)].nod),STATUS_SUBMITTED); }
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 277 "menuy.y"
    { log_event_status_event(&(yyvsp[(1) - (3)].dti),(yyvsp[(3) - (3)].nod),STATUS_ABORTED); }
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 278 "menuy.y"
    { log_event_status_event(&(yyvsp[(1) - (3)].dti),(yyvsp[(3) - (3)].nod),STATUS_UNKNOWN); }
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 279 "menuy.y"
    { log_event_event_event(&(yyvsp[(1) - (3)].dti),(yyvsp[(3) - (3)].nod),1); }
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 280 "menuy.y"
    { log_event_event_event(&(yyvsp[(1) - (3)].dti),(yyvsp[(3) - (3)].nod),0); }
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 281 "menuy.y"
    { log_event_meter_event(&(yyvsp[(1) - (7)].dti),(yyvsp[(6) - (7)].nod),(yyvsp[(5) - (7)].num)); }
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 288 "menuy.y"
    { (yyval.nod) = log_event_find(yytext); }
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 291 "menuy.y"
    { (yyval.num) = atol(yytext); }
    break;



/* Line 1455 of yacc.c  */
#line 2174 "y.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 294 "menuy.y"


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
  else if (*yytext == '[') {} 
  else if (*yytext == ':') {} 
  else if (*yytext == '/') {} 
  else printf("!%s:%d:<%s>\n",msg,yylineno,yytext);
}


