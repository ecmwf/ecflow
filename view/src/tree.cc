//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #11 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include <stdio.h>

#ifndef tree_H
#include "tree.h"
#endif

#ifndef init_H
#include "init.h"
#endif

#ifndef ecflowview_H
#include "ecflowview.h"
#endif

#ifndef host_H
#include "host.h"
#endif

#ifndef menus_H
#include "menus.h"
#endif

#ifndef gui_H
#include "gui.h"
#endif

#ifndef tmp_file_H
#include "tmp_file.h"
#endif
#ifndef globals_H
#include "globals.h"
#endif

#include "ecf_node.h"

extern "C" {
#include "xec.h"
#include "SimpleTree.h"
}

#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>


//===========================================


tree::tree(host* h):
	host_(h)
{
	observe(h);

#if 0	
	XtAddCallback( tree, XmNhelpCallback, help_callback, 
	(XtPointer) 0 );
#endif

	tree_c::create(gui::trees(),(char*)h->name());

	add_input_CB();
}

tree::~tree()
{
	XtDestroyWidget(tree_);
}

void tree::xd_show()
{
	XtManageChild(tree_);
}

void tree::xd_hide()
{
	XtUnmanageChild(tree_);
}

void tree::build_tree(node* n,int p)
{
  while(n) {
    int w = n->getBox(tree_);
    NodeAddRelation(tree_,p,w);
    build_tree(n->kids(),w);
    n = n->next();
  }
}

int tree::count(node* n)
{
	int c = 0;
	while(n)
	{
		c += count(n->kids()) + 1;
		n = n->next();
	}
	return c;
}

void tree::notification(observable* o)
{
	host* h = (host*)o;

	NodeReset(tree_);

	NodeReserve(tree_,count(h->top()));

	build_tree(h->top(),-1);

	if (!h->top()) return;
	if (h->name() == selection::server()) {
	  node* n = h->top()->find(selection::current_path());
	  if(n) show_node(*n);
	}
	update_all(false);
}

void tree::selection_cleared()
{
	XtSetSensitive(show_current_,False);
	XtSetSensitive(fold_around_,False);
	XtSetSensitive(hide_other_,False);
	node_window::selection_cleared();
}

void tree::new_selection(node& n)
{
	XtSetSensitive(show_current_,True);
	XtSetSensitive(fold_around_,True);
	XtSetSensitive(hide_other_,True);
	node_window::new_selection(n);
}

tree* tree::new_tree(host* h)
{
	if(gui::trees() == 0)
		return 0;

	Widget w = XtNameToWidget(gui::trees(),h->name());
	tree*  t = 0;

	if(w == 0)
		t = new tree(h);
	else
		t = (tree*)xec_GetUserData(w);

	return t;
}

long tree::update_tree(node* n,bool visible)
{
  long changes = 0;
  
  while(n) {
    Boolean vis = visible && ( n->visible() || n->show_it());
    Boolean old = n->visibility(vis);
    
    if(old != vis) changes++;
      
    changes += update_tree(n->kids(),vis && !n->folded());
    
    n = n->next();
  }
  
  return changes;
}

void tree::update_tree(bool redraw)
{
  long changes = host_ ? update_tree(host_->top(),True) : 0;
  if(redraw)  NodeNewSizeAll(tree_);
  if(changes) NodeUpdate(tree_);
}

void tree::fold_unfold_all(node *n,Boolean folding)
{
  while(n) {
    n->folded(folding);   
    // n->ondemand(false); // 20120705
    fold_unfold_all(n->kids(),folding);
    n = n->next();
  }
}

void tree::click2(node* n,Boolean shift,Boolean control)
{
  if(!n) return;
  
  // n->ondemand(true); build_tree(n, -1); update_tree(true); // 20120705

  if (shift && control)
    fold_unfold_all(n,!n->folded());
  else if(shift) {    
    tmp_file f = n->serv().host::output(*n);
    char buf[10240];
    const char *p = getenv("PAGER");    
    const char *fname = f.c_str();
    if (!fname) return;
    sprintf(buf,"xterm -e %s %s&",p?p:"more",fname);
    system(buf);    
    return;
  }	
  else if(control)
    NodeTreeFlip(tree_,n->getBox(tree_));
  else n->folded(!n->folded());
  
  update_tree(false);
}

xnode* tree::xnode_of(node& n)
{
	return &n;
}

void tree::show_node(node& n)
{
	node* p = n.parent();
	while(p)
	{
		p->folded(False);
		// 201207 update_tree(true); // p->serv().redraw(); // 201107
		p = p->parent();
	}
	update_tree(false);
	n.select();
}
void tree::hideOtherCB( Widget w, XtPointer p )
{
	node* n = selection::current_node();
	if(!n) return;
	
	if(n->serv().where() != this) 
		n->serv().where()->hideOtherCB(w,p);
	else
		n->serv().suites(n);
}

void tree::snapshotCB( Widget w, XtPointer p )
{
  char cmd[1024];
  FILE *f = 0;

  gui::message("using SNAPSHOT ; press button \n");
  sprintf(cmd,"${SNAPSHOT:=import} %s\n", snapshotName);
  
  f = popen(cmd,"r");
  if(!f) {
    gui::error("Cannot create snapshot : %s", cmd);
    return;
  } else if (!pclose(f)) {
    gui::message("%s # generated\n", snapshotName);
    sprintf(cmd,"${SNAPVISU:=firefox} %s\n", snapshotName);  
    f = popen(cmd,"r");
  } 
  else {
    gui::error("Cannot create snapshot : %s", cmd);
    return;
  }
}

void tree::aroundCB( Widget w, XtPointer p)
{
	node* n = selection::current_node();
	if(!n) return;
	
	if(n->serv().where() != this) 
	  n->serv().where()->aroundCB(w,p);
	else
	{
	  if (host_) fold_unfold_all(host_->top(),True);
	  show_node(*n);
	  fold_unfold_all(n,False);
	  n->select();
	  update_tree(false);
	}
}

void tree::foldCB( Widget, XtPointer )
{
        if (host_) fold_unfold_all(host_->top(),True);
	update_tree(false);
}

void tree::unfoldCB( Widget, XtPointer )
{
  if (host_) {
	  fold_unfold_all(host_->top(),False);
	  // 201207 build_tree(host_->top(), -1);
  }
  update_tree(true);
}

void tree::showCB( Widget, XtPointer )
{
	node* n = selection::current_node();
	if(n) {
		tree* t = n->serv().where();
		t->show_node(*n);
		n->select();
	}
}

void tree::connected(Boolean ok)
{
  if(ok) {
    XtVaSetValues(tree_,XmNbackgroundPixmap,XmUNSPECIFIED_PIXMAP,NULL);
  } else {
    Pixel fg,bg;
    XtVaGetValues(tree_,XmNforeground, &fg, XmNbackground, &bg,NULL);
    XtVaSetValues(tree_,XmNbackgroundPixmap,
		  XmGetPixmap(XtScreen(tree_),"25_foreground",fg,bg),NULL);
  }
}

void tree::update_all(bool redraw)
{
	tree *t = extent<tree>::first();
	while(t)
	{
		t->update_tree(redraw);
		t = t->extent<tree>::next();
	}
}

IMP(tree)
