#ifndef node_window_H
#define node_window_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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


#ifndef node_H
#include "node.h"
#endif

#ifndef selection_H
#include "selection.h"
#endif

class xnode;

class node_window : public selection {
public:
	node_window();

	~node_window(); // Change to virtual if base class

	void add_input_CB();

	virtual xnode* xnode_of(node&) = 0;
	virtual Widget node_widget() = 0;

	virtual void new_selection(node&);
	virtual void selection_cleared();

protected:

	virtual void show_node(node&);
	virtual void click(XEvent*);

	virtual void link(XEvent*,node*,node*);

	virtual void keypress(XEvent* event);
	virtual void click1(node*,Boolean,Boolean);
	virtual void click2(node*,Boolean,Boolean);
	virtual void click3(node*,Boolean,Boolean);

	virtual void raw_click1(XEvent* event,xnode*);
	virtual void raw_click2(XEvent* event,xnode*);
	virtual void raw_click3(XEvent* event,xnode*);

	virtual Widget menu1() = 0;
	virtual Widget menu2() = 0;

	static void inputCB(Widget,XtPointer,XtPointer);
	static void linkCB(Widget,XtPointer,XtPointer);

private:

	node_window(const node_window&);
	node_window& operator=(const node_window&);

};

inline void destroy(node_window**) {}
#endif
