//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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

#include "history.h"
#include "node.h"
#include "host.h"
#include "selection.h"
#include <Xm/List.h>
extern "C" {
#include "xec.h"
}

const int kMaxLines = 400;

history::history(panel_window& w):
	panel(w),
	timeout(30),
	last_("")
{
  panel::detach();
}

history::~history()
{
}

void history::clear()
{
	last_ = "";
	XmListDeleteAllItems(list_);
	timeout::disable();
}

void history::show(node& n)
{
	timeout::enable();
	add(n.serv());
}

void history::run()
{
	if(get_node())
		add(get_node()->serv());
	else 
		clear();
}

Boolean history::enabled(node& n)
{
  return n.type() == NODE_SUPER;
}

void history::browseCB(Widget,XtPointer data)
{
	XmListCallbackStruct* cb = (XmListCallbackStruct*)data;
	char *p = (char*)xec_GetString(cb->item);

	if(get_node())
	{
		node* n = get_node()->find_match(p);
		if(n)  selection::notify_new_selection(n);
	}
	else
		clear();

	XtFree(p); 
}

void history::add(host& h)
{
  std::list<std::string>& l = h.history(last_);
  int pos = 0;
  XtVaGetValues(list_,XmNitemCount,&pos,NULL);
  std::string prev = last_;
  std::list<std::string>::const_iterator j;
  for(j = l.begin(); j != l.end() ; ++j) 
    {
      if (j->empty()) {}
      else if (*j == "") {}
      /* filter out line with time stamp older than last */
      else if (last_ != "" && strcmp(j->c_str()+3, last_.c_str()+3) <= 0) {}
      /* filter out some commands */
      else if (j->find("command:LogCmd") != std::string::npos) {}
      else if (j->find("--log=get") != std::string::npos) {}
      else if (j->find("--news") != std::string::npos) {}
      /* add interesting lines */
      else {
	if(pos >= kMaxLines)
	  XmListDeletePos(list_,1); 
	else
	  pos++;
	
	int err = j->substr(0, 4)=="ERR:"; /* bold ? */
	xec_AddFontListItem(list_,(char*)j->c_str(),err);
	prev = *j;
      }
    }
  
  XmListSetBottomPos(list_,pos);
  l.clear();
  last_ = prev;
}

// static panel_maker<history> maker(PANEL_HISTORY);
