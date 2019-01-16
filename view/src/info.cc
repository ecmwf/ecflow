//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #10 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include "info.h"
#include "node.h"
#include "error.h"
#include "selection.h"
#include "Hyper.h"
#include <stdio.h>
#include "ecf_node.h"

info::info(panel_window& w):
  panel(w)
{
}

info::~info()
{
}

void info::clear()
{
  forget_all();
  HyperSetText(text_,(char*)"No node selected.");
}

void info::show(node& n)
{
  forget_all();
  std::stringstream ss;
  n.info(ss);
  HyperSetText(text_,(char*) ss.str().c_str());
}

void info::hyperCB(Widget w,XtPointer data)
{
  panel::hyper(w,data);
}

Boolean info::enabled(node& n)
{
  return n.hasInfo();
}
