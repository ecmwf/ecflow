//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #11 $ 
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

#include "why.h"
#include "node.h"
#include "error.h"
#include "selection.h"
#include "tmp_file.h"
#include "Hyper.h"
#include <stdio.h>
#include <stdarg.h>

/***********************************/

why::why(panel_window& w):
	panel(w)
{
}

why::~why()
{
  forget_all();
}

void why::clear()
{
	forget_all();
	HyperSetText(text_,"");
}

void why::show(node& n)
{
	forget_all();
	node* parent = n.parent(); // SUP-421
	while (parent) {
	  observe(parent);
	  parent = parent->parent();
	}
        std::stringstream ss;
	n.why(ss);
        HyperSetText(text_,(char*) ss.str().c_str());
}

void why::hyperCB(Widget w,XtPointer data)
{
	panel::hyper(w,data);
}

Boolean why::enabled(node& n)
{
	return n.isSimpleNode();
}
