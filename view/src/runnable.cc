//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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

#include "ecflowview.h"
#include "runnable.h"

/* do a process in the "background" */

Boolean runnable::workCB(XtPointer)
{
    runnable *p = first();
    int active = 0;
    while(p) {
      runnable* n = p->next();
      
      if(p->actived_) {
	active++;
	p->run();
      }
      
      p = n;
    }
    return (active == 0);
}

runnable::runnable():
	actived_(False)
{
}

runnable::~runnable()
{
}

void runnable::enable()
{
  if(actived_) return;

  extern XtAppContext app_context;
  if(app_context != 0) 
    XtAppAddWorkProc(app_context,workCB,NULL);

  actived_ = True;
}


void runnable::disable()
{
	actived_ = False;
}

IMP(runnable)
