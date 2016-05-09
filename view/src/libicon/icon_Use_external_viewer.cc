/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #8 $                                                                    */
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
" 	c None",
".	c #E5E5E5",
"+	c #000000",
"@	c #FFFFFF",
"#	c #DED9D9",
"$	c #CAC6C6",
"................",
"................",
"................",
"................",
"....++.......++.",
"...+..+.....+..+",
"..+........+....",
".+++...++++.....",
"+@##+.+@##+.....",
"+##$+++##$+.....",
"+#$$+.+#$$+.....",
".+++...+++......",
"................",
"................",
"................",
"................"};
static pixmap p("Use_external_viewer",(const char**)bits);
