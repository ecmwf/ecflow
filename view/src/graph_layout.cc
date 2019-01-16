//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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

#include "arch.h"
#include "graph_layout.h"
#include "node.h"
#include "reach.h"
#include "observer.h"
#include "dummy_node.h"
#include "trigger_panel.h"
#include "trigger_lister.h"
#include "selection.h"
#include "xec.h"
#include "str.h"
#include "Hyper.h"
#include "tmp_file.h"
#include <Xm/PushBG.h>
#include <Xm/SeparatoG.h>

class graph_node : public observer, public xnode {
protected:

  graph_layout& owner_;
  
  void notification(observable*)	{ redraw(); }
  void adoption(observable* o,observable* n)  { node_ = (node*)n; }
  void gone(observable*)		{ owner_.remove(this); delete this; }
  
  void draw(Widget w,XRectangle* r)  { node_->drawNode(w,r,false); }
  
  void size(Widget w,XRectangle* r)  { node_->sizeNode(w,r,false); }
  
public:
	graph_node(graph_layout&,node*); 
};

graph_node::graph_node(graph_layout& t,node *n):
	xnode(n),
	owner_(t)
{
	observe(n);
}

//===============================================================

struct node_relation {
	node*	       trigger_;
	node*	       through_;
	int	       mode_;
	node_relation* next_;

	node_relation(node* tg,node* th,int m):
		trigger_(tg), through_(th), mode_(m), next_(0) {}
};

//===============================================================

graph_layout::graph_layout(trigger_panel& t,Widget w):
	layout(t,w),
	link_(false)
{
	add_input_CB();
}

graph_layout::~graph_layout()
{
	clear();
}

Widget graph_layout::menu1()
{
	if(link_)
		return owner_.linkMenu();
	else
		return owner_.infoMenu();
}

Widget graph_layout::menu2()
{
	return menu1();
}

xnode* graph_layout::xnode_of(node& n)
{
	for(int i = 0; i < nodes_.count(); i++)
		if(nodes_[i]->get_node() == &n)
			return nodes_[i];
	return 0;
}

void graph_layout::clear()
{
	int i;
	NodeReset(widget_);
	for(i = 0; i < nodes_.count(); i++)
		delete nodes_[i];
	nodes_.clear();

	for(i = 0; i < relations_.count(); i++)
		delete relations_[i];
	relations_.clear();

	link_ = false;
}



class nl1 : public trigger_lister {
	int	    n_;
	graph_layout&	t_;
	node* g_;
	bool e_;
public:

	nl1(graph_layout& t,node* g,bool e) : n_(0), t_(t), g_(g), e_(e) {}

	void next_node(node& n,node* p,int mode,node* t) {
		t_.relation(&n,g_,p,mode,t);
		n_++;
	}

	Boolean parents() { return e_; }
	Boolean kids() { return e_; }

	int count() { return n_; }
};

class nl2 : public trigger_lister {
	int	    n_;
	graph_layout&	t_;
	node* g_;
	bool e_;
public:

	nl2(graph_layout& t,node* g,bool e) :  n_(0), t_(t), g_(g), e_(e) {}
	void next_node(node& n,node* p,int mode,node* t) {
		t_.relation(g_,&n,p,mode,t);
		n_++;
	}
	Boolean parents() { return e_; }
	Boolean kids()	  { return e_; }

	int count() { return n_; }
};

graph_node *graph_layout::get_graph_node(node* n)
{
	if(!n) return 0;

	n = n->graph_node();

	graph_node* t = (graph_node*)xnode_of(*n);
	if(t) return t;

	t = new graph_node(*this,n);
	t->getBox(widget_);
	t->visibility(True);

	nodes_.add(t);

	return t;
}

void graph_layout::remove(graph_node* g)
{
	nodes_.remove(g);
}

void graph_layout::click1(node* n,Boolean shift,Boolean control)
{
	if(n != 0) node_window::click1(n,shift,control);
}


void graph_layout::click2(node* n,Boolean shift,Boolean control)
{
	grow(n);

	if(shift && !control)
	{
		node *p = n->parent();
		if(p) {
			relation(p,n,0,trigger_lister::hierarchy,0);
			grow(p);
		}
	}

	if(control)
	{
		grow(n,True);
	}

	if(shift && control)
	{
		int count = 0;
		while(count != nodes_.count())
		{
			count = nodes_.count();
			for(int i = 0; i < count; i++)
				grow(nodes_[i]->get_node());
		}
	}
}

void graph_layout::show(node& n)
{
	clear();
	grow(&n,False);

	graph_node *g = get_graph_node(&n); 
	if(g) {
		g->select();
		g->setFocus();
	}
}

static void clear_menu(Widget menu)
{
	WidgetList wl = 0;
	int count = 0;

	XtVaGetValues(menu,
			XmNchildren,&wl,
			XtNnumChildren,&count,
			NULL);
	XtUnmanageChildren(wl,count);
}


static void add_button(Widget menu,node* n,const char* a,const char* b)
{	
	WidgetList wl = 0;
	int count = 0;
	Widget w = 0;

	XtVaGetValues(menu,
		      XmNchildren,&wl,
		      XtNnumChildren,&count,
		      NULL);

	for(int i = 0 ; i < count; i++)
		if(!XtIsManaged(wl[i]))
		{
			w = wl[i];
			break;
		}

	if(!w) 
	  w = XmCreatePushButtonGadget(menu,"button",NULL,0);

	xmstring s = xmstring(a,"bold") + xmstring(" ") + xmstring(b);

	XtVaSetValues(w,
		      XmNlabelString, XmString(s),
		      XmNuserData,    n,
		      NULL);

	XtManageChild(w);	       
}

static void add_separator(Widget menu)
{	
	WidgetList wl = 0;
	int count = 0;
	Widget w = 0;

	XtVaGetValues(menu,
		      XmNchildren,&wl,
		      XtNnumChildren,&count,
		      NULL);

	for(int i = 0 ; i < count; i++)
		if(!XtIsManaged(wl[i]))
		{
			w = wl[i];
			break;
		}

	if(!w) w = XmCreateSeparatorGadget(menu,"button",NULL,0);
	XtManageChild(w);
}

static void tidy_menu(Widget menu)
{
  WidgetList wl = 0;
  int count = 0;
  XtVaGetValues(menu, XmNchildren,&wl, XtNnumChildren,&count, NULL);

  for(int i = 0 ; i < count; i++)
    if(XmIsPushButtonGadget(wl[i]))
      if(xec_GetUserData(wl[i]) == 0)
	XtUnmanageChild(wl[i]);
}

void graph_layout::link(XEvent* event_node,node* n1,node* n2)
{
  graph_node *g1 = get_graph_node(n1);
  graph_node *g2 = get_graph_node(n2);
  node* n = 0;
  
  link_ = false;
  
  if(g1 && g2)
    {
      link_ = true;
      
      node_relation* r = (node_relation*)g1->relation_data(g2);
      
      tmp_file tmp(tmpnam(0));
      FILE *f = fopen(tmp.c_str(),"w");
      
      if(f) {
	fprintf(f,"From: {%s}\n",n1->full_name().c_str());
	fprintf(f,"To	 : {%s}\n",n2->full_name().c_str());
      }
      
      clear_menu(owner_.linkMenu());
      add_button(owner_.linkMenu(),n1,"From",n1->full_name().c_str());
      add_button(owner_.linkMenu(),n2,"To",n2->full_name().c_str());
      
      while(r)
	{
	  if(f) fprintf(f,"\n");
	  add_separator(owner_.linkMenu());
	  
	  if((n = r->trigger_)) {
	    if(f) fprintf(f,"Trigger: %s\n",n->definition().c_str());
	    add_button(owner_.linkMenu(),n,"Trigger",n->definition().c_str());
	  }
	  else add_button(owner_.linkMenu(),0,"-","-");
	  
	  if((n = r->through_)) {
	    
	    if(f) fprintf(f,"Through: {%s}\n",n->full_name().c_str());
	    add_button(owner_.linkMenu(),n,"Through",n->full_name().c_str());
	  }
	  else add_button(owner_.linkMenu(),0,"-","-");
	  
	  r = r->next_;
	}
      tidy_menu(owner_.linkMenu());
      
      if(f) fclose(f);
      HyperLoadFile(owner_.dependHyperText(),(char*)tmp.c_str());
      if(event_node->xbutton.button == 1)
	owner_.showDependWindow();
    }
  
  if(!link_) {
    HyperSetText(owner_.dependHyperText(),"");
    owner_.hideDependWindow();
  }
}

void graph_layout::selectNode(node *n)
{
  if(n)
    {
      graph_node* g = (graph_node*)xnode_of(*n);
      if(g) g->show();
      selection::notify_new_selection(n);
    }
}

int graph_layout::grow(node* n,Boolean )
{
  nl1 l1(*this,n,owner().extended());
  if(owner().triggers()) n->triggers(l1);
  
  nl2 l2(*this,n,owner().extended());
  if(owner().triggered()) n->triggered(l2);
  
  return l1.count() + l2.count();
}

void graph_layout::relation(node* from, node* to, 
			    node* through, int mode,node *trigger)
{
  graph_node* from_g    = get_graph_node(from);
  graph_node* to_g      = get_graph_node(to);
  
  from_g->relation(to_g);
  
  node_relation* n = (node_relation*)from_g->relation_data(to_g);
  while(n)
    {
      if(n->trigger_ == trigger &&
	 n->through_ == through &&
	 n->mode_    == mode)
	break;
      
      n = n->next_;
    }
  
  if(n == 0) {
    
    n = new node_relation(trigger,through,mode);
    relations_.add(n);
    
    void* x = from_g->relation_data(to_g,n);
		if(x) n->next_ = (node_relation*)x;
  }
  
  switch(mode)
    {
    case trigger_lister::normal:
      break;
      
    case trigger_lister::child:
      /* from_g->relation_gc(to_g,gui::colorGC(STATUS_SUBMITTED)); */
      from_g->relation_gc(to_g,gui::blueGC());
      break;
      
    case trigger_lister::parent:
      //from_g->relation_gc(to_g,gui::colorGC(STATUS_COMPLETE));
      from_g->relation_gc(to_g,gui::blueGC());
      break;
      
    case trigger_lister::hierarchy:
      from_g->relation_gc(to_g,gui::colorGC(STATUS_ABORTED));
      break;
    }
}

class graph_layout_reacher : public reach_lister {
  graph_layout& g_;
public:
  graph_layout_reacher(graph_layout& g) : g_(g) {}
  
  void next(node* from, node* to,node* through, int mode,node *trigger)
  {
    g_.relation(from,to,through,mode,trigger);
  }
};

void graph_layout::reach(node* n1,node* n2)
{
  graph_layout_reacher rl(*this);
  clear();
  reach::join(n1,n2,rl);
}

//====================================================================
