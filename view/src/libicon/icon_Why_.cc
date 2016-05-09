/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #5 $                                                                    */
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

#include "pixmap.h"
/* XPM */
static const char * bits[] = {
"16 16 6 1",
" 	c #E7E7E7E7E7E7",
".	c #BDBDB5B5B5B5",
"X	c #4A4A42423939",
"o	c #7B7B7B7B7B7B",
"O	c #CECECECECECE",
"+	c #8C8C8C8C8C8C",
"                ",
"                ",
"                ",
"      .XXXo     ",
"     .XO  XO    ",
"     o.   o+    ",
"     .   .X     ",
"        .XO     ",
"       .XO      ",
"       XO       ",
"      .o        ",
"                ",
"      .O        ",
"      o.        ",
"                ",
"                "};
static pixmap p("Why_",(const char**)bits);
