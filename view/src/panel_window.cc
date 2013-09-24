//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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

#include <stdio.h>
#include "gui.h"
#include "panel_window.h"
#include "panel.h"
#include "node.h"
#include "globals.h"
#include "Tab.h"
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>

const char* kDefault = "Info";

extern "C" {
#include "xec.h"
}

panel_window::panel_window():
	panels_(0),
	node_(0),
	current_(0)
{
	create(gui::top());
	set_node(0,"Info",true);
	load_size();
	XtRealizeWidget(panel_top);
}

panel_window::panel_window(panel_window* other):
	panels_(0),
	node_(0),
	current_(0)
{
	create(gui::top());

	panel* p = panels_;
	panel* o = other->panels_;

	while(p && o)
	{
		p->copy(o);
		p = p->next_;
		o = o->next_;
	}

	other->save_size();
	set_node(other->node_,XtName(TabGetCurrent(other->tab_)),true);
	load_size();
	XtRealizeWidget(panel_top);
	XmToggleButtonSetState(detached_,other->detached(),True);
	XmToggleButtonSetState(frozen_,other->frozen(),True);
	XmToggleButtonSetState(close_on_apply_,
		XmToggleButtonGetState(other->close_on_apply_),True);
}

panel_window::panel_window(node* n,bool detached,bool frozen,const char* tab):
	panels_(0),
	node_(0),
	current_(0)
{
	create(gui::top());
	set_node(n,tab,true);
	load_size();
	XtRealizeWidget(panel_top);
	XmToggleButtonSetState(detached_,detached,True);
	XmToggleButtonSetState(frozen_,frozen,True);
}

panel_window::panel_window(panel_window* other,node* n,bool detached,bool frozen):
	panels_(0),
	node_(0),
	current_(0)
{
	create(gui::top());

	panel* p = panels_;
	panel* o = other->panels_;

	while(p && o)
	{
		p->copy(o);
		p = p->next_;
		o = o->next_;
	}

	other->save_size();
	set_node(n,XtName(TabGetCurrent(other->tab_)),true);
	load_size();
	XtRealizeWidget(panel_top);
	XmToggleButtonSetState(detached_,detached,True);
	XmToggleButtonSetState(frozen_,frozen,True);
}


panel_window::~panel_window()
{
	save_size();
	delete panels_;
	XtDestroyWidget(xd_rootwidget());
}

void panel_window::save_size()
{
	Dimension w,h;

	XtVaGetValues(form_,
		XmNwidth, &w,
		XmNheight,&h,
		NULL);

	char *n = XtName(TabGetCurrent(tab_));
	char wname[1024]; sprintf(wname,"panel_%s_width", n);
	char hname[1024]; sprintf(hname,"panel_%s_heigth",n);

	globals::set_resource(wname,w);
	globals::set_resource(hname,h);
}

void panel_window::load_size()
{
	Dimension w,h;

	char *n = XtName(TabGetCurrent(tab_));
	char wname[1024]; sprintf(wname,"panel_%s_width", n);
	char hname[1024]; sprintf(hname,"panel_%s_heigth",n);

	w = globals::get_resource(wname,500);
	h = globals::get_resource(hname,500);

	XtVaSetValues(form_,
		XmNwidth, w,
		XmNheight,h,
		NULL);
}

void panel_window::create (Widget parent, char *widget_name)
{
  panel_top_c::create(parent,widget_name);
  panels_ = panel_factory::create_all(*this,tab_);
  XtAddCallback(tab_, XmNvalueChangedCallback, tabCB, this);
}

void panel_window::tabCB(Widget w,XtPointer call)
{
	TabCallbackStruct* cb = (TabCallbackStruct*)call;
	set(find(cb->widget));
	if (!current_) return;
	if(node_)
		current_->show(*node_);
	else
		current_->clear();
}

void panel_window::set_tab(const char* tab)
{
	panel *p = find(tab);
	if (p) {
	  TabSetCurrent(tab_,p->widget(),False);
	  current_ = p;
	}
}

void panel_window::tabCB(Widget w, XtPointer client, XtPointer call)
{
	panel_window* i = (panel_window*)client;
	i->tabCB(w,call);
}

void panel_window::selection_cleared()
{
	if(detached()) return;
	set_node(0,0,true);
}

void panel_window::title()
{
  std::string name;
  name = node_ ? node_->node_name() : "-";
  if(detached()) name += " (detached)";
  if(frozen())   name += " (frozen)";  
  XtVaSetValues(xd_rootwidget(),XmNtitle,name.c_str(),NULL);

  if (!current_) return;

  name = std::string(current_->name()) + ":";
  if (node_) 
    name += node_->node_name();
  else 
    name += "-";
  set_menu(name.c_str());
}

void panel_window::new_selection(node& n)
{
	if(detached()) return;
	set_node(&n,0,true);
}

void panel_window::set(panel* c)
{
  if (!c) return;

  if(current_ && current_ != c) 
    current_->clear();

	current_ = c;

	XtUnmanageChild(tab_);

	Widget w = current_->widget();
	
	panel* p = panels_;
	while(p) {

		bool ok = (node_?p->enabled(*node_):false);
		if(ok)
			XtManageChild(p->widget()); 
		else
			XtUnmanageChild(p->widget()); 
		p = p->next_;
	}

	if(w && !XtIsManaged(w))
	{
		current_ = find(kDefault);
		w = current_->widget();
		XtManageChild(w);
	}

	TabSetCurrent(tab_,w,False);

	p = panels_;
	while(p) {
		Widget m = p->menus(menubar_);
		if(m) {
			if(p == current_)
				XtManageChild(m);
			else
				XtUnmanageChild(m);
		}
		p = p->next_;
	}

	XtManageChild(tab_);

	XtSetSensitive(save_,current_->can_save());
	XtSetSensitive(print_,current_->can_print());
}

void panel_window::cloneCB(Widget w,XtPointer)
{
	panel_window *p = new panel_window(this);
	p->xd_show();
}

void panel_window::unmapCB(Widget,XtPointer)
{
	delete this;
}

void panel_window::mapCB(Widget,XtPointer)
{
}

void panel_window::nodeCB(Widget,XtPointer data)
{
//	XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct*) data;
	/* detached_ = !cb->set; */
	title();
}

void panel_window::freezeCB(Widget,XtPointer data)
{
//	XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct*) data;
	/* frozen_ = !cb->set; */
	title();
}

void panel_window::xd_show()
{
	Map();
}

void panel_window::notification(observable* n)
{
	set(current_);
	current_->changed(*node_);
}

void panel_window::gone(observable* n)
{
	set_node(0,0,true);
}

void panel_window::adoption(observable* o,observable *n)
{
	set_node((node*)n,0,!frozen());
}

panel* panel_window::find(Widget w) 
{
	if(!w) w = TabGetCurrent(tab_);
	panel* p = panels_;
	while(p)
	{
		if(p->widget() == w) 
			return p;
		p = p->next_;
	}
	return 0;
}

void panel_window::set_node(node* n,const char* tab,bool update)
{
	// if(n == node_) return;

	forget(node_);

	panel* p = panels_;
	while(p)
	{
		p->node_ = n;
		p = p->next_;
	}
	node_ = n;
	observe(node_);

	if(tab) set_tab(tab);
	if(n && !current_->enabled(*n))
		set_tab(kDefault);

	if(update) {
		if(n)
			current_->show(*n);
		else
			current_->clear();
		set(current_);
		title();	
	}
}

void panel_window::detach()
{
	XmToggleButtonSetState(detached_,True, True);
}

void panel_window::freeze()
{
	XmToggleButtonSetState(detached_,True, True);
	XmToggleButtonSetState(frozen_,True,True);
}


void panel_window::new_window(node* n,const char* tab,bool detached,bool frozen)
{
	panel_window *p = new panel_window(n,detached,frozen,tab);
	p->xd_show();
}

void panel_window::new_window(node* n)
{
	panel_window *p = new panel_window(this,n,true,true);
	p->xd_show();
}

panel* panel_window::find(const char* name)
{
	panel* p = panels_;
	while(p)
	{
		bool ok = node_?p->enabled(*node_):false;
		if(ok && (strcmp(p->name(),name) == 0))
			return p;
		p = p->next_;
	}

	p = panels_;   
	while(p)  
	{
		if(strcmp(p->name(),kDefault) == 0)  
			return p;   
		p = p->next_;
	}

	abort();
	return 0;
}

void panel_window::submit()
{
	if(XmToggleButtonGetState(close_on_apply_))
		delete this;
}

void panel_window::resizeCB(Widget,XtPointer)
{
	save_size();
}

bool panel_window::frozen()
{
	return XmToggleButtonGetState(frozen_);
}

bool panel_window::detached()
{
	return XmToggleButtonGetState(detached_);
}

void panel_window::printCB(Widget,XtPointer)
{
	current_->print();
}

void panel_window::saveCB(Widget,XtPointer)
{
	current_->save();
}
