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

#include "edit_repeat.h"
#include "repeat_node.h"
#include "host.h"
#include "ecflowview.h"
#include <Xm/List.h>
#include <Xm/Text.h>
extern "C" {
#include "xec.h"
}

edit_repeat::edit_repeat(panel_window& w):
	panel(w),
	loading_(false),
	index_(-1), 
	indexs_("")
{
}

edit_repeat::~edit_repeat()
{
}

void edit_repeat::clear()
{
	loading_ = true;
	XmListDeleteAllItems(list_);     
	index_ = -1;
	indexs_ = "";
	loading_ = false;
}

void edit_repeat::show(node& n)
{
	repeat_node& m = (repeat_node&)n;

	loading_ = true;

	str80 buf;
	int end = m.last();
	int cur = m.current();
	int inc = m.step();
	XmListDeleteAllItems(list_);     

	if(end > 50 && m.can_use_text())
	{
		use_text_ = true;
		char buf[1024];
		char buf1[1024];
		char buf2[1024];
		m.value(buf1,0);
		m.value(buf2,end-1);
		sprintf(buf,"Enter a value between %s and %s (step %d):",buf1,buf2,inc);
		xec_SetLabel(label_,buf);
		XtUnmanageChild(show_list_);
		XtManageChild(show_text_);
		m.value(buf,cur);
		XmTextSetString(text_,buf);
	}
	else
	{
		use_text_ = false;
		XtManageChild(show_list_);
		XtUnmanageChild(show_text_);

		for(int i=0 ; i < end ; i++)
		{
			m.value(buf,i);
			xec_AddListItem(list_,buf);
		}

		XmListSelectPos(list_,cur+1,True);
	}

	loading_ = false;
}

Boolean edit_repeat::enabled(node& n)
{ 
  int i = n.type();
  return i == NODE_REPEAT || 
    i == NODE_REPEAT_E || 
    i == NODE_REPEAT_S ||
    i == NODE_REPEAT_D || 
    i == NODE_REPEAT_I;
}

void edit_repeat::applyCB(Widget,XtPointer)
{
  if(get_node()) {
    char *p = 0x0;
    if(use_text_) {
      p = XmTextGetString(text_);
    }

    if (get_node()->__node__()) /* ecflow */
      get_node()->serv().command(clientName, "--alter", "change", "repeat",
			       p ? p : indexs_.c_str(),
			       get_node()->parent_name().c_str(), 
			       NULL);
    else
      get_node()->serv().command("alter", "-R", 
				 get_node()->full_name().c_str(), p, NULL);

    if (p) XtFree(p);

  } else { 
    clear();
  }
  submit();
}

void edit_repeat::browseCB(Widget,XtPointer data)  
{
	XmListCallbackStruct *cb = (XmListCallbackStruct *) data;
	char *p = xec_GetString(cb->item); 
	if(get_node())
	  indexs_ = p;
	else
	  indexs_ = "";
	XtFree(p);

	if(loading_) return;
	freeze();
}

