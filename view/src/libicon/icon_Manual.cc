/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #5 $                                                                    */
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
"16 16 20 1",
" 	c #E7E7E7E7E7E7",
".	c #CECECECECECE",
"X	c #BDBDB5B5B5B5",
"o	c #94949C9C9C9C",
"O	c #73738C8C7B7B",
"+	c #5A5A73736363",
"@	c #42425A5A4A4A",
"#	c #EFEFC6C6CECE",
"$	c #C6C69C9CA5A5",
"%	c #9C9C73737B7B",
"&	c #737342425252",
"*	c #7B7B7B7B7B7B",
"=	c #4A4A5A5A5A5A",
"-	c #52525A5A5A5A",
";	c #84848C8C8C8C",
":	c #A5A584848C8C",
">	c #B5B58C8C9494",
",	c #DEDEDEDEDEDE",
"<	c #A5A5A5A5A5A5",
"1	c #A5A57B7B8484",
"     .XoO+@     ",
"#$%& .XoO+@     ",
"#$%& .X+O+@ .o*=",
"#%%& .X-O+@ .;*-",
"#%%& .X-O+@ X=;=",
"#:%& .X=O+@ .@*=",
"#:%& .X=O+@ .@*=",
"#%%& .X+O+@ X@*-",
"#*%& .X;;+@ .-*=",
"#>%& .X;O+@ .;*=",
"#>%& .XoO+@ .o*=",
"#$%& .XoO+@,.<*=",
"#$%& .XoO+@ .o*=",
"#>%& .<;O+@ .;*=",
"#1%& .XOO+@ .+*=",
"#:%& .Xo;+@,.;*="};
static pixmap p("Manual",(const char**)bits);
