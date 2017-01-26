//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include "ecflowview.h"
#include "flags.h"
#include "globals.h"
#include "str.h"
#include "selection.h"

#ifdef linux
extern "C" char *cuserid(char*);
#endif

flags::~flags()
{
}

Boolean eventFlag::eval(node* n)
{
  if (!n) return False;
  return n->status() == status_;
}

Boolean statusFlag::eval(node* n)
{
  if (!n) return False;
  XECFDEBUG printf("statusFlag: %d %d %d \n", n->isSimpleNode() ? 1:0,n->status(),status_);
  return n->isSimpleNode() && (n->status() == status_);
}

Boolean typeFlag::eval(node* n)
{
  if (!n) return False;
  if (type_ == NODE_REPEAT) {
    int i = n->type();
    return i == NODE_REPEAT ||
      i == NODE_REPEAT_E || i == NODE_REPEAT_I ||
      i == NODE_REPEAT_S || i == NODE_REPEAT_D;    
  }
  return n->type() == type_;
}

Boolean procFlag::eval(node* n)
{
  if (!n) return False;
  return (n->*proc_)();
}

Boolean userFlag::eval(node* n)
{
  /*	static char* names[] = {
	    (char*) "CDPUSER",
	    (char*) "CDPOPER",
	    (char*) "CDPADMIN",
	};

	const char* v = n->variable(names[level_]).c_str();
	if(v) {
		static str me(cuserid(0));
		return strcmp(me.c_str(),v) == 0;
	}
  */
  return (globals::user_level() == level_);
}

Boolean selectionFlag::eval(node* n)
{
  return selection::current_node() != 0 && selection::current_node() != n;
}
