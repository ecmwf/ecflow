#ifndef panel_window_H
#define panel_window_H
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


#ifndef uipanel_H
#include "uipanel.h"
#endif

#ifndef selection_H
#include "selection.h"
#endif

#ifndef panel_H
#include "panel.h"
#endif

#ifndef observer_H
#include "observer.h"
#endif

#ifndef window_H
#include "window.h"
#endif


class panel_window 
  : public panel_top_c
  , public selection
  , public observer
  , public window 
{
public:
	panel_window();
	panel_window(node*,bool,bool,const char*);
	panel_window(panel_window*);
	panel_window(panel_window*,node*,bool,bool);

	~panel_window(); // Change to virtual if base class

	bool frozen();
	bool detached();

	void detach();
	void freeze();

	void title();
	void show(const char*);
	void new_window( node*);
	void submit();

	virtual void create (Widget parent, char *widget_name = 0);
	virtual Widget shell() { return _xd_rootwidget; }

	// From selection
	virtual void new_selection(node&);
	virtual void selection_cleared();

	// From panel_top_c
	void cloneCB(Widget,XtPointer);
	void unmapCB(Widget,XtPointer);
	void mapCB(Widget,XtPointer);
	void nodeCB(Widget,XtPointer);
	void freezeCB(Widget,XtPointer);
	void resizeCB(Widget,XtPointer);

	virtual void xd_show();

	static panel_window* find(node*);
	static void new_window(node*,const char*,bool,bool);

private:
	panel_window(const panel_window&);
	panel_window& operator=(const panel_window&);

	panel*  panels_;
	node*   node_;
	panel*  current_;

	void set(panel*);
	void set_tab(const char*);
	void copy(panel_window&);
	virtual void tabCB(Widget,XtPointer);

	void set_node(node*,const char*,bool);

	panel* find(const char*);
	panel* find(Widget);

	void save_size();
	void load_size();

	// From observer<node>
	virtual void notification(observable*);
	virtual void gone(observable*);
	virtual void adoption(observable*,observable*);

	virtual void printCB(Widget,XtPointer);
	virtual void saveCB(Widget,XtPointer);

	static void tabCB(Widget,XtPointer,XtPointer);
};

inline void destroy(panel_window**) {}
#endif
