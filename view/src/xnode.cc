//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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

#include "xnode.h"

xnode::xnode(node *n):
  widget_(0),
  node_(n),
  box_(-1)
{
}

xnode::~xnode()
{
}

int xnode::getBox(Widget w)
{
  if(widget_ && widget_ != w)
    return -1;

  if(box_ == -1)
  {
    widget_ = w;
    box_ =  NodeCreate(w,drawCB,sizeCB,this);
  }
  return box_;
}

void xnode::drawCB(Widget w,XRectangle* r,void *data)
{
    ((xnode*)data)->draw(w,r);
}

void xnode::sizeCB(Widget w,XRectangle* r,void *data)
{
    ((xnode*)data)->size(w,r);
}

void xnode::select()
{
  XtVaSetValues(widget_,XtNselected,box_,NULL);
  NodeShow(widget_,box_);
}
