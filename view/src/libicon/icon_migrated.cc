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


#include "pixmap.h"
/* XPM */
static const char * bits[] = {
"16 16 3 1",
" 	c #E79DE79DE79D",
".	c #000000000000",
"X	c #F7DEFBEE79E7",
"                ",
"     .....      ",
"    .XXXXX.     ",
"   .XXXXXXX.    ",
"  .XXXXXXXXX.   ",
" .XXXXXXXXXXX.  ",
" .XXXX...XXXX.  ",
" .XXXX. .XXXX.  ",
" .XXXX...XXXX.  ",
" .XXXXXXXXXXX.  ",
"  .XXXXXXXXX.   ",
"   .XXXXXXX.    ",
"    .XXXXX.     ",
"     .........  ",
"                ",
"                "};
static pixmap p("migrated",(const char**)bits);
