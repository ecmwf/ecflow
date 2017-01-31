//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
//
// Copyright 2009-2017 ECMWF.
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
#include "confirm.h"
#include "gui.h"

confirm::confirm()
{
	create(gui::top());
}

confirm::~confirm()
{
}

Boolean confirm::ask(Boolean def_ok,const char* fmt,...)
{
	char buf[1024];
	va_list arg;
	va_start(arg,fmt);
	vsprintf(buf,fmt,arg);
	va_end(arg);

	return instance().modal(buf, def_ok);
}

Boolean confirm::ask(Boolean def_ok,str& msg)
{
  return instance().modal(msg.c_str(), def_ok);
}
