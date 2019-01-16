//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

#include "ecflowview.h"
#include "timeout.h"

extern XtAppContext app_context;

void timeout::timeoutCB(XtPointer data,XtIntervalId* id)
{
  timeout* t = (timeout*)data;
  
  if(t->actived_) 
    {
      t->running_ = True;
      t->run();
      t->running_ = False;
    }
  
  if(t->actived_) 
    t->id_ = XtAppAddTimeOut(app_context,t->frequency_*1000,timeoutCB,t);
}

timeout::timeout(double frequency):
  actived_(False),
  frequency_(frequency),
  id_(0),
  running_(False)
{
}

timeout::~timeout()
{
  disable();
}

void timeout::enable()
{
  if(!actived_ && app_context)
    {
      id_      = XtAppAddTimeOut(app_context,frequency_*1000,timeoutCB,this);
      actived_ = True;
    }
}


void timeout::disable()
{
  if(actived_ && id_)
    {
      XtRemoveTimeOut(id_);
      id_      = 0;
      actived_ = False;
    }
}

void timeout::frequency(double n)
{
  frequency_ = n;
  if(!running_ && actived_)
    {
      disable();
      enable();
    }
}

void timeout::drift(double n,double maximum)
{
  double x = frequency_ + n;
  if(x>maximum) x = maximum;
  frequency(x);
}

IMP(timeout)
