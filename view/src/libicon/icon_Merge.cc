/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #5 $                                                                    */
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
"16 16 4 1",
" 	c #FFFFFFFFFFFF",
".	c #E7E7E7E7E7E7",
"X	c #E5E5E5E5E5E5",
"o	c #000000000000",
" ...............",
" ...XXXooooooo..",
" ...XXoo     o..",
" .XXXoooooo  o..",
" .XXoo    o  oX.",
" .Xo o    o  oX.",
" .oooo    o  o..",
" .o       o  o..",
" .o       o  o..",
" .o       o  o..",
" .o       o  o..",
" .o       o  o..",
" .o       oooo..",
" .o       o.....",
" ..oooooooo.....",
"                "};
static pixmap p("Merge",(const char**)bits);
