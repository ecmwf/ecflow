/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #8 $                                                                    */
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

#include "pixmap.h"
/* XPM */
static const char * bits[] = {
"16 16 13 1",
" 	c None",
".	c #E7E7E7",
"+	c #7B7B7B",
"@	c #000000",
"#	c #4A4239",
"$	c #8C8C8C",
"%	c #FFFFFF",
"&	c #CECECE",
"*	c #BDB5B5",
"=	c #E8E8E8",
"-	c #C9C7C7",
";	c #8C5A39",
">	c #FFCE9C",
"................",
"................",
"....+@##$.......",
"...++%%%++......",
"...#%%%%%+&.....",
"..*+%%%%=+*.....",
"..*+%%%=-+*.....",
"...#%===-+&.....",
"...$#---+$......",
"....$@#@++*.....",
".........;;>....",
"..........;;>...",
"...........;;*..",
"............;;..",
".............&..",
"................"};
static pixmap p("Search",(const char**)bits);
