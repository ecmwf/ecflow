#ifndef jobcheck_panel_H
#define jobcheck_panel_H
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


#include "uijobcheck.h"

#ifndef panel_H
#include "panel.h"
#endif

#ifndef text_window_H
#include "text_window.h"
#endif

class jobcheck_panel : public panel
  , public jobcheck_form_c
  , public text_window 
{
public:
	jobcheck_panel(panel_window&);
	
	~jobcheck_panel(); // Change to virtual if base class

	void refresh();

	virtual const char* name() const { return "Check"; }
	virtual void show(node&);
	virtual Boolean enabled(node&);
	virtual void clear();
	virtual Widget widget() { return jobcheck_form_c::xd_rootwidget(); }
	virtual Widget tools()  { return tools_; }
	virtual Widget text()   { return text_; }
	virtual void create (Widget parent, char *widget_name = 0 );

private:

	jobcheck_panel(const jobcheck_panel&);
	jobcheck_panel& operator=(const jobcheck_panel&);

	virtual void update();
	virtual void changed(node&);

	virtual void refreshCB(Widget, XtPointer ) { refresh();}

	virtual bool can_print() { return true; }
	virtual bool can_save()  { return true; }
	virtual void print()     { text_window::print(); }
	virtual void save()      { text_window::save(); }
};

inline void destroy(jobcheck_panel**) {}
#endif
