//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include "xmstring.h"


xmstring::xmstring(const char* s,const char* font):
	str_(0)
{

	if(!s) return;

	if(font == 0) font = "normal";


	if(*s == '\n')
	{
		str_ = XmStringSeparatorCreate();
		*this += xmstring(s+1,font);
		return;
	}


	if(strchr(s,'\n'))
	{

		XmString xms1;
		XmString xms2;
		XmString line;
		XmString separator;
		char     *p;
		char     *t = XtNewString(s);	/* Make a copy for strtok not to */
                                 	/* damage the original string    */

		separator = XmStringSeparatorCreate();
		p         = strtok(t,"\n");
		xms1      = XmStringCreateLtoR(p,(char*)font);

		while ((p = strtok(NULL,"\n")))
		{
			line = XmStringCreateLtoR(p,(char*)font);
			xms2 = XmStringConcat(xms1,separator);
			XmStringFree(xms1);
			xms1 = XmStringConcat(xms2,line);
			XmStringFree(xms2);
			XmStringFree(line);
		}

		XmStringFree(separator);
		XtFree(t);

		str_ =  xms1;
	}
	else str_ = XmStringCreateLtoR((char*)s,(char*)font);
}

xmstring::~xmstring()
{
	if(str_)
		XmStringFree(str_);
}

xmstring::xmstring(XmString s):
	str_(XmStringCopy(s))
{
}

xmstring::xmstring(const xmstring& other):
	str_(other.str_?XmStringCopy(other.str_):0)
{
}

xmstring& xmstring::operator=(const xmstring& other)
{
	if(str_) XmStringFree(str_);
	str_ = 0;
	if(other.str_) str_ = XmStringCopy(other.str_);
	return *this;
}

xmstring xmstring::operator+(const xmstring& other) const
{
	if(!other.str_) return *this;
	if(!str_) return other;

	xmstring x;
	x.str_ = XmStringConcat(str_,other.str_);
	return x;
}

xmstring& xmstring::operator+=(const xmstring& other)
{
	*this = *this + other;
	return *this;
}
