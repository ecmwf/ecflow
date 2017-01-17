/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #8 $                                                                    */
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
"16 16 7 1",
" 	c None",
".	c #E7E7E7",
"+	c #000000",
"@	c #5F5F5F",
"#	c #FFFFFF",
"$	c #DFDFDF",
"%	c #7B7B7B",
".......++@......",
"......@##+......",
".....@##+.......",
"....@##+#+$.....",
"...@##+###+.....",
"..++++##..#+....",
"..+%###.##.#+...",
"..+###.##.###+..",
"...+####.##.#+..",
"....+##.##.##+..",
".....+###.##+...",
"......+#.##+....",
".......+##+.....",
"......%+#+......",
".....%%++.......",
"......++........"};
static pixmap p("Script",(const char**)bits);
