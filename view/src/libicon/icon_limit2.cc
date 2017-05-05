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
"8 8 6 1",
" 	c #E5E5E5E5E5E5",
".	c #00000000EEEE",
"X	c #00000000FFFF",
"o	c #000000000000",
"O	c #FFFFFFFFFFFF",
"+	c #000000008888",
"  ....  ",
" .XXXXo ",
".XOXXXX+",
".XXXXXX+",
".XXXXXX+",
".XXXXXX+",
" +XXXX+ ",
"  ++++  "};
static pixmap p("limit2",(const char**)bits);
