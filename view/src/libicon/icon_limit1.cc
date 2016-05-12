/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #1 $                                                                    */
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
"8 8 6 1",
" 	c #E5E5E5E5E5E5",
".	c #0000EEEE0000",
"X	c #0000FFFF0000",
"o	c #000000000000",
"O	c #FFFFFFFFFFFF",
"+	c #000088880000",
"  ....  ",
" .XXXXo ",
".XOXXXX+",
".XXXXXX+",
".XXXXXX+",
".XXXXXX+",
" +XXXX+ ",
"  ++++  "};
static pixmap p("limit1",(const char**)bits);
