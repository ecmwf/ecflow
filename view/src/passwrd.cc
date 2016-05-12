//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include <stdio.h>
#include <stdarg.h>
#include "passwrd.h"
#include "gui.h"
#include "str.h"
extern "C" {
#include "xec.h"
}
#include "ecflowview.h"

#include <Xm/Text.h>

#ifdef linux
extern "C" char* cuserid(char*);
#endif


passwrd::passwrd()
{
	create(gui::top());
}

passwrd::~passwrd()
{
}


Boolean passwrd::ask(const str& title,str& user,str& pass)
{
	return instance().prompt(title,user,pass);
}

Boolean passwrd::prompt(const str& title,str& user,str& pass)
{

	XmTextSetString(user_,(char*)(user.c_str()[0]?user.c_str():cuserid(0)));
	XmTextSetString(passwd_,(char*)(pass.c_str()));

	if(modal(title.c_str(),True))
	{
		char *p = XmTextGetString(user_);
		user = p;
		XtFree(p);

		p = XmTextGetString(passwd_);
		pass = p;
		XtFree(p);
		return True;
	}

	return False;
}

void passwrd::modifyCB( Widget, XtPointer )
{
}
