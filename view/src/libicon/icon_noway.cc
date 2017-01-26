//=============================================================================================
// Name        : 
// Author      : 
/* Revision    : $Revision: #1 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2017 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
// Description : 
//=============================================================================================

#include "pixmap.h"
/* XPM */
/* 
static const char * bits[] = {
"16 16 2 1",
" 	c #E7E7E7E7E7E7",
".	c #000000000000",
" ...  ... .   . ",
".   ..   ... .. ",
".    .   .. . . ",
".    .   ..   . ",
".   ..   ..   . ",
" ...  ... .   . ",
"....  .    .... ",
".   . .    .    ",
"....  .    ...  ",
".     .    .    ",
".     .... .... ",
"..... ....      ",
"  .   .         ",
"  .   ...       ",
"  .   .         ",
"  .   ....      "};
*/
static const char * bits[] = {
"16 16 3 1",	   
" 	c #E5E5E5E5E5E5",   
".	c #FFFFFF",
"#	c #FF0000",
"     ......     ",
"   ..######..   ",
"  .##########.  ",
" .############. ",
" .############. ",
".##############.",
".##############.",
".#............#.",
".#............#.",
".##############.",
".##############.",
" .############. ",
" .############. ",
"  .##########.  ",
"   ..######..   ",
"     ......     "};




static pixmap p("noway",(const char**)bits);
