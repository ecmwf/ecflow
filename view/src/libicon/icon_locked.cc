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
"16 16 4 1",
" 	c #E5E5E5E5E5E5",
".	c #282828282828",
"X	c #FFFFFFFFFFFF",
"o	c #ADADADADADAD",
"                ",
"      ....      ",
"    ..    ..    ",
"   .        .   ",
"  .          .  ",
"  XXXXXXXXXXX.  ",
"  Xoooooooooo.  ",
"  Xoooo..oooo.  ",
"  Xooo....ooo.  ",
"  Xoooo..oooo.  ",
"  Xoooo..oooo.  ",
"  Xoooo..oooo.  ",
"  Xoooooooooo.  ",
"  Xoooooooooo.  ",
"  ............  ",
"  XXXXXXXXXXXX  "};
static pixmap p("locked",(const char**)bits);
