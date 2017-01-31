#ifndef timetable_panel_H
#define timetable_panel_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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


#include "uitimetable.h"

#ifndef panel_H
#include "panel.h"
#endif

#ifndef node_window_H
#include "node_window.h"
#endif

#ifndef str_H
#include "str.h"
#endif

#ifndef array_H
#include "array.h"
#endif

#ifndef log_event_H
#include "log_event.h"
#endif

#ifndef depend_H
#include "depend.h"
#endif

class layout;
class host;
class timetable_node;

class timetable_panel : public panel
  , public node_window
  , public depend
  , public event_lister
  , public timetable_form_c {
public:

	timetable_panel(panel_window&);

	~timetable_panel(); // Change to virtual if base class

	void remove(timetable_node*);

	virtual void create (Widget parent, char *widget_name = NULL);
	virtual void show(node&);
	virtual void changed(node&);
	virtual Boolean enabled(node&);
	virtual void clear();
	virtual const char* name() const { return "Time line"; }
	virtual Widget widget() { return timetable_form_c::xd_rootwidget(); }
	virtual Widget tools() { return tools_; }

private:

	timetable_panel(const timetable_panel&);
	timetable_panel& operator=(const timetable_panel&);

	array<timetable_node*> nodes_;

	DateTime last_;
	DateTime min_time_;
	DateTime max_time_;
	
	bool sorted_by_time_;
	bool tasks_only_;

	DateTime dt1_;
	DateTime dt2_;

	void load(bool);
	void load(const char*,bool);
	timetable_node* add(log_event*);
	void range(timetable_node*,DateTime&,DateTime&);
	timetable_node* main(timetable_node*);

	void reload(bool);

	virtual void chooseCB( Widget, XtPointer );
	virtual void loadCB( Widget, XtPointer );
	virtual void mergeCB( Widget, XtPointer );
	virtual void updateCB( Widget, XtPointer );
	virtual void activateCB( Widget, XtPointer );
	virtual void optionsCB( Widget, XtPointer );

	virtual void setFromCB( Widget, XtPointer );
	virtual void setToCB( Widget, XtPointer );
	virtual void setBothCB( Widget, XtPointer );
	virtual void resetCB( Widget, XtPointer );
	virtual void hyperCB( Widget, XtPointer );

	// From node_window
	virtual xnode* xnode_of(node &);
	virtual Widget node_widget() { return time_; }
	virtual Widget menu1();
	virtual Widget menu2();

	virtual void raw_click1(XEvent*,xnode*);
	virtual void raw_click2(XEvent*,xnode*);
	virtual void raw_click3(XEvent*,xnode*);

	virtual void next(log_event*);
};

inline void destroy(timetable_panel**) {}
#endif
