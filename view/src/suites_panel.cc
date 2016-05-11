//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #9 $ 
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

#include "suites_panel.h"
#include "node.h"
#include "host.h"
#include "ecflowview.h"
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/List.h>
extern "C" {
#include "xec.h"
}

suites_panel::suites_panel(panel_window& w):
	panel(w)
	, done (false)
{
}

suites_panel::~suites_panel()
{
}

void suites_panel::create (Widget parent, char *widget_name )
{
  suites_form_c::create(parent,widget_name);
}

void suites_panel::clear()
{
  if (done) return; 
  XmListDeleteAllItems(list_);
}

struct suite_lister1 { 
  Widget                   list_;
  unsigned int             pos_;
  std::vector<std::string> suites_;
  void next(suite_lister1&);
  std::string name() 
  { return (pos_ < suites_.size()) ? suites_[pos_] : ""; }
  void run()
  { for (unsigned int i=0; i<suites_.size(); ++i) next(*this); }

public:
  suite_lister1(Widget list, std::vector<std::string> suites) 
    : list_(list), pos_(0), suites_(suites) {
    std::sort(suites_.begin(), suites_.end());
  }
};

void suite_lister1::next(suite_lister1& l)
{ 
  if (pos_ < suites_.size() && suites_[pos_] != "*") {
    xec_AddListItem(list_,(char*)name().c_str());
  }
  ++pos_;
}

struct suite_lister2 {
  Widget                   list_;
  unsigned int             pos_;
  std::vector<std::string> suites_;
  void next(suite_lister2& l);
  std::string name() 
  { return (pos_ < suites_.size()) ? suites_[pos_] : ""; }
  void run() 
  { for (unsigned int i=0; i<suites_.size(); ++i) next(*this); };

public:
  suite_lister2(Widget list, std::vector<std::string> suites) 
    : list_(list), pos_(0), suites_(suites) {}
};

void suite_lister2::next(suite_lister2&)
{ 
  if (pos_ < suites_.size() && suites_[pos_] != "*") {
    XmString s = XmStringCreateSimple((char*)name().c_str());
    XmListSelectItem(list_,s,False);
    XmStringFree(s);
  }
  ++pos_;
}

void suites_panel::show(node& n)
{
  if (done) return; done = true; clear();
  // XmListDeleteAllItems(list_);  
  { std::vector<std::string> sv1; n.serv().suites(SUITES_LIST,sv1); 
    suite_lister1 sl1(list_, sv1); sl1.run(); }
  { std::vector<std::string> sv2; n.serv().suites(SUITES_MINE,sv2); 
    suite_lister2 sl2(list_, sv2); sl2.run(); }
}

Boolean suites_panel::enabled(node& n)
{
  return n.type() == NODE_SUPER;
}

void suites_panel::tellCB( Widget, XtPointer )
{
  XmString *items;
  int count;
  std::vector<std::string> l;
  
  XtVaGetValues(list_,
		XmNselectedItems,    &items,
		XmNselectedItemCount,&count,
		NULL);
    
  for(int i = 0; i < count ; ++i) {
    char* p = xec_GetString(items[i]);
    l.push_back(std::string (p));
    XtFree(p);
  }
  if(get_node())
    get_node()->serv().suites(SUITES_REG, l);
  else
    clear();	
  submit();
}

void suites_panel::offCB( Widget, XtPointer )
{
	XmListDeselectAllItems(list_);	
}

void suites_panel::onCB( Widget, XtPointer )
{
	xec_ListSelectAll(list_);
}

// static panel_maker<suites_panel> maker(PANEL_SUITES);
