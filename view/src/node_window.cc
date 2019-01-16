//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #7 $ 
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

#include "node_window.h"
#include "menus.h"
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include "xnode.h"
#include "collector.h"
#include "host.h"
#ifdef AIX
#include <X11/keysym.h>
#endif
node_window::node_window()
{
}

node_window::~node_window()
{
}

void node_window::linkCB(Widget w,XtPointer from,XtPointer cb_data)
{
	LinkCallbackStruct* cb = (LinkCallbackStruct*)cb_data;
	node_window* t = (node_window*)from;

		
	xnode* x1 = (xnode*)cb->data1;
	xnode* x2 = (xnode*)cb->data2;

	node* n1 = x1?x1->get_node():0;
	node* n2 = x2?x2->get_node():0;

	t->link(cb->event,n1,n2);
}

void node_window::inputCB(Widget w,XtPointer from,XtPointer cb_data)
{
  XmDrawingAreaCallbackStruct* cb = (XmDrawingAreaCallbackStruct*)cb_data;

  // printf("inputCB %d\n", cb->event->type);
  if(cb->event->type == ButtonPress || cb->event->type == KeyPress) {
    node_window* t = (node_window*)from;
    t->click(cb->event); 
  }
}

void node_window::click1(node* n,Boolean shift,Boolean control)
{
  // printf("raw_click1 %p %d %d\n",n,shift,control);
  if(control && n)     collector::show(*n);
  else if (shift && n) this->click2(n,0,control);
  else                 selection::notify_new_selection(n);
}

void node_window::click2(node* n,Boolean shift,Boolean control)
{
}

void node_window::click3(node* n,Boolean shift,Boolean control)
{
}

void node_window::raw_click1(XEvent* event,xnode* x)
{
	node*  n = x ? x->get_node() : 0;
	selection::menu_node(n);
	unsigned int modifiers = event->xbutton.state;
	Boolean shift   = (modifiers & ShiftMask) != 0;
	Boolean control = (modifiers & ControlMask) != 0;
	click1(n,shift,control);
}

void node_window::raw_click2(XEvent* event,xnode* x)
{
	node*  n = x ? x->get_node() : 0;
	selection::menu_node(n);
	unsigned int modifiers = event->xbutton.state;
	Boolean shift   = (modifiers & ShiftMask) != 0;
	Boolean control = (modifiers & ControlMask) != 0;
	if(n) this->click2(n,shift,control);
}

void node_window::raw_click3(XEvent* event,xnode* x)
{
	node*  n = x ? x->get_node() : 0;
	selection::menu_node(n);
	unsigned int modifiers = event->xbutton.state;
	Boolean shift   = (modifiers & ShiftMask) != 0;
	Boolean control = (modifiers & ControlMask) != 0;

	if(n) click3(n,shift,control);
	if( shift && n)
	{
	  XmMenuPosition(menu2(),(XButtonPressedEvent*)event);
	  XtManageChild(menu2());
	}
	else if(n)
	{
	  // menus::show(node_widget(),event,n); // 20141119
	  menus::show(menu1(),event,n);
	  //XmMenuPosition(g_cmd_menu,(XButtonPressedEvent*)event);
	  //XtManageChild(g_cmd_menu);
	} else {
	  XmMenuPosition(menu1(),(XButtonPressedEvent*)event);
	  XtManageChild(menu1());
	}
}

node* next_node(node* n)
{
  node *out = n;
  while (n) {
    if (n->type() == NODE_TASK   || 
	n->type() == NODE_FAMILY ||
	n->type() == NODE_SUITE  ||
	n->type() == NODE_ALIAS)
      return n;
    n = n->next();
  }
  return out;
}

node* next_host(node* n, bool first) {
  host *h = 0x0;
  if (!n) return n;
  if (first) h = extent<host>::first();
  else h = &n->serv();
  while ((h= h->extent<host>::next())) {
    if (h->top())
      return h->top();    
  }
  if (first) return n;
  return next_host(n, true);
}

void node_window::keypress(XEvent* event)
{
  xnode* x     = (xnode*)NodeFind(node_widget(),event);
  node*  n = 0x0;
  KeySym keysym = XLookupKeysym(&(event->xkey), 0);

  if (keysym == XK_KP_Space || keysym == XK_space) {
    raw_click1(event,x);
    // } else if (keysym == XK_F2) { raw_click2(event,x);
  } else if (keysym == XK_KP_Enter || keysym == XK_Return) {
    if ((event->xbutton.state & ShiftMask) != 0)
      n = selection::current_node();
    if (n) 
      menus::show(node_widget(),event,n);
    else
      raw_click3(event,x);
  } else if (keysym == XK_Up) {
    node* first = n = selection::current_node();
    if (!n) return;
    n = n->parent();

    if (!n) /* reach server node */
      n = next_host(selection::current_node(), true);

    if (!n) return;
    selection::notify_new_selection(n);

    n = n->kids();
    if (n==first) n = n->parent();

    if (n) click1(n,0,0);
  } else if (keysym == XK_Down) {
    n = selection::current_node();
    if (!n) return;
    n = n->next();
    if (!n) {
      n = selection::current_node()->parent();
      if (n) n = n->next();
    }
    if (!n) /* reach server node */
      n = next_host(selection::current_node(), false);    
    if (n) click1(n,0,0);
  } else if (keysym == XK_Left) {
    n = selection::current_node();
    if (!n) return;
    n = n->parent(); 
    if (!n) /* reach server node */
      n = next_host(selection::current_node(), true);    
    if (!n) return;
    click2(n,0,0);
    click1(n,0,0);
  } else if (keysym == XK_Right) {
    n = selection::current_node();
    if (!n) return;
    click2(n,0,0);
    if (n->kids()) n = n->kids();
    else n = n->next();
    n = next_node(n);
    if (n) click1(n,0,0);	  
    }
}

void node_window::click(XEvent* event)
{
  int button   = event->xbutton.button;
  xnode* x     = (xnode*)NodeFind(node_widget(),event);
  
  switch(button) {
  case 1: raw_click1(event,x); break;
  case 2: raw_click2(event,x); break;
  case 3: raw_click3(event,x); break;
  default: keypress(event);
  }
}

void node_window::show_node(node&)
{
}

void node_window::new_selection(node& n)
{
  xnode* x = xnode_of(n);
  if(x && x->widget() == node_widget() ) { 
    show_node(n);
    x->select();
  }
  else 
    selection_cleared();
}

void node_window::selection_cleared()
{
  XtVaSetValues(node_widget(),XtNselected,-1,NULL);
}

void node_window::add_input_CB()
{
  XtAddCallback( node_widget(), XmNinputCallback, inputCB, this);
  XtAddCallback( node_widget(), XtNlinkCallback,  linkCB, this);
}

void node_window::link(XEvent*,node*,node*)
{
}
