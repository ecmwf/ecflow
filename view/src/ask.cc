//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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
#include "ask.h"
#include "gui.h"
#include <Xm/Text.h>

ask::ask()
{
  create(gui::top());
}

ask::~ask()
{
}

bool ask::show(str& val,std::string msg)
{
  static std::string m = msg;
  return instance().show(m.c_str(),val);
}

bool ask::show(const char* msg,str& val)
{
  XmTextSetString(value_, (char*) val.c_str());
  if (!modal(msg, true))
    return false;
  char *p=XmTextGetString(value_);;
  val = p;
  XtFree(p);
  return true;
}
