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

#include <stdio.h>
#include "window.h"
#include <Xm/PushB.h>
#include <X11/IntrinsicP.h>


extern "C" {
#include "xec.h"
}
#include "gui.h"


window::window()
  : menu_(0)
{
}

window::~window()
{
  if(menu_) 
    XtDestroyWidget(menu_);
}

void window::raise()
{
  CompositeWidget c = (CompositeWidget)shell();
  for(unsigned int i = 0 ; i < c->composite.num_children; i++)
    XtManageChild(c->composite.children[i]);
  
  XMapRaised(XtDisplay(shell()),XtWindow(shell()));
}

void window::set_menu(const char* name)
{
  if(!menu_)
    {
      menu_ = XmCreatePushButton(gui::windows(),(char*)"menu",0,0);
      xec_SetUserData(menu_,this);
      XtManageChild(menu_);
    }
  xec_SetLabel(menu_,name);
}

IMP(window)
