#ifndef tree_H
#define tree_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #8 $ 
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


#include "uitop.h"

#ifndef node_H
#include "node.h"
#endif

#ifndef node_window_H
#include "node_window.h"
#endif

#ifndef observer_H
#include "observer.h"
#endif

#ifndef uitree_H
#include "uitree.h"
#endif

#ifndef extent_H
#include "extent.h"
#endif

class tree_node;

class tree : public node_window
  , public tree_c
  , public extent<tree>
  , public observer {
 public:
	tree(host*);

	~tree(); // Change to virtual if base class

	void xd_show();
	void xd_hide();

	void connected(Boolean);
	void update_tree(bool);

	virtual xnode* xnode_of(node& n);

	static tree* new_tree(host*);
	static void update_all(bool);

	virtual void click2(node*,Boolean,Boolean);
private:
	// void ondemand(node&, bool full=false);

	tree(const tree&);
	tree& operator=(const tree&);

	host* host_;

	void build_tree(node*,int);
	int count(node*);

	void fold_unfold_all(node*,Boolean);
	long update_tree(node*,bool);

	// From node window
	virtual void show_node(node&);

	virtual Widget menu1() { return see_menu_; }
	virtual Widget menu2() { return why_menu_; }

	// From observer<host>
	void notification(observable*);

	void gone(observable*)           {}
	void adoption(observable*,observable*) {}

	virtual void new_selection(node&);
	virtual void selection_cleared();

	Widget node_widget() { return tree_; }

	virtual void hideOtherCB( Widget, XtPointer );
	virtual void aroundCB( Widget, XtPointer );
	virtual void foldCB( Widget, XtPointer );
	virtual void unfoldCB( Widget, XtPointer );
	virtual void showCB( Widget, XtPointer );

	virtual void snapshotCB( Widget, XtPointer );
};

inline void destroy(tree**) {}
#endif
