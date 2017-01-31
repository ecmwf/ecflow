#ifndef graph_layout_H
#define graph_layout_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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


#include "array.h"

#ifndef node_window_H
#include "node_window.h"
#endif

#ifndef layout_H
#include "layout.h"
#endif

class graph_node;
struct node_relation;

class graph_layout : public layout, 
		         public node_window {
public:
	graph_layout(trigger_panel&,Widget);

	~graph_layout(); // Change to virtual if base class

	graph_node* get_graph_node(node*);
	int grow(node*,Boolean = False);
	void siblings(node*);

	virtual void show(node&);
	virtual void clear();
	virtual void reach(node*,node*);
	virtual void selectNode(node*);
	virtual xnode* xnode_of(node&);
	virtual void click1(node*,Boolean,Boolean);
	virtual void click2(node*,Boolean,Boolean);

	Widget node_widget() { return widget_; }
	void remove(graph_node*);

	void relation(node*,node*,node*,int,node*);

private:

	graph_layout(const graph_layout&);
	graph_layout& operator=(const graph_layout&);
	
	array<graph_node*>    nodes_;
	array<node_relation*> relations_;
	bool  link_;

	void scan(node*,node*);

	virtual Widget menu1();
	virtual Widget menu2();

	virtual void link(XEvent*,node*,node*);
};

inline void destroy(graph_layout**) {}
#endif
