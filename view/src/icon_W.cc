#include "pixmap.h"
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #3 $                                                                    */
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

/* XPM */
static const char * bits[] = {
"16 16 50 1",
" 	c None",
".	c #E7E7E7",
"+	c #E7E3E3",
"@	c #E9CDCD",
"#	c #EEA3A3",
"$	c #FF0000",
"%	c #FA2A2A",
"&	c #F64E4E",
"*	c #F93434",
"=	c #FB1C1C",
"-	c #F46464",
";	c #FD0E0E",
">	c #EDA9A9",
",	c #E8D7D7",
"'	c #FB2222",
")	c #FE0606",
"!	c #ECB5B5",
"~	c #FA2727",
"{	c #EDABAB",
"]	c #F83E3E",
"^	c #EF9494",
"/	c #E7E5E5",
"(	c #EE9F9F",
"_	c #F36D6D",
":	c #F08D8D",
"<	c #F74646",
"[	c #ECB0B0",
"}	c #FE0404",
"|	c #FA2828",
"1	c #E8DBDB",
"2	c #E9CACA",
"3	c #FD1313",
"4	c #FD1212",
"5	c #EACACA",
"6	c #E7E6E6",
"7	c #F65555",
"8	c #E7DFDF",
"9	c #E8DCDC",
"0	c #FA2929",
"a	c #ECAFAF",
"b	c #F18484",
"c	c #F74949",
"d	c #E7E0E0",
"e	c #F27D7D",
"f	c #FE0101",
"g	c #F46868",
"h	c #F93939",
"i	c #F93131",
"j	c #FD0B0B",
"k	c #EDA6A6",
"................",
"................",
"....+@@@@@@@@...",
"....#$$%&*$$=...",
"....-;>.,'$)!...",
"....~{.+]$$^....",
".../(..-$$_.....",
"......:$$</.....",
".....[}$|1......",
"....23$45.678...",
"...90$}a..b0....",
"...c$$:.defg....",
"...$$)hij$$k....",
"................",
"................",
"................"};
static pixmap p("W",(const char**)bits);
