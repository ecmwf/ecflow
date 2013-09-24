//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #11 $ 
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


#include "zombies_panel.h"
#include "node.h"
#include "host.h"
#include "ecflowview.h"
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/List.h>
extern "C" {
#include "xec.h"
}
// #include <sstream>

zombies_panel::zombies_panel(panel_window& w):
	panel(w),
	name_(0)
{
}

zombies_panel::~zombies_panel()
{
	XtFree(name_);
}

void zombies_panel::create (Widget parent, char *widget_name )
{
	zombies_form_c::create(parent,widget_name);
}

void zombies_panel::clear()
{
	XmListDeleteAllItems(list_);
	XtSetSensitive(buttons_,False);
}

void zombies_panel::show(node& n)
{
  std::vector<std::string> list;
  if (!n.serv().get_zombies_list(list)) {
    return;
  }
  clear();

  xec_AddFontListItem(list_,(char*)list[0].c_str(),true);
  for(unsigned int i= 1; i < list.size(); ++i)
    xec_AddListItem(list_,(char*)list[i].c_str());   
}


Boolean zombies_panel::enabled(node& n)
{
  return n.type() == NODE_SUPER;
}


void zombies_panel::browseCB( Widget, XtPointer data)
{
  XmListCallbackStruct *cb = (XmListCallbackStruct *) data;
  char *p = xec_GetString(cb->item);
  if(name_) XtFree(name_);
  name_ = XtNewString(node::find_name(p));
  XtSetSensitive(buttons_,name_ != 0);
  XtFree(p);
}

void zombies_panel::deleteCB( Widget, XtPointer )
{
	call(ZOMBIE_DELETE);
}

void zombies_panel::acceptCB( Widget, XtPointer )
{
	call(ZOMBIE_FOB);
}

void zombies_panel::rescueCB( Widget, XtPointer )
{
	call(ZOMBIE_RESCUE);
}

void zombies_panel::terminateCB( Widget, XtPointer )
{
	call(ZOMBIE_FAIL);
}

void zombies_panel::killCB( Widget, XtPointer )
{
	call(ZOMBIE_KILL);
}

void zombies_panel::call(int mode)
{
  if(!name_)
    XtSetSensitive(buttons_,false);

  if(get_node()) {
    get_node()->serv().zombies(mode,name_);
  } else 
    clear();
  
  post_update();
}

// static panel_maker<zombies_panel> maker(PANEL_ZOMBIES);
