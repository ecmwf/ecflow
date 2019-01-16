#ifndef suites_panel_H
#define suites_panel_H

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

#include "uisuites.h"

#ifndef panel_H
#include "panel.h"
#endif

class suites_panel : public panel, public suites_form_c {
public:

	suites_panel(panel_window&);

	~suites_panel(); // Change to virtual if base class

	virtual const char* name() const { return "Suites"; }
	virtual void show(node&);
	virtual void clear();
	virtual Boolean enabled(node&);
	virtual Widget widget() { return suites_form_c::xd_rootwidget(); }
	virtual Widget tools() { return tools_; }

	virtual void create (Widget parent, char *widget_name = NULL);

private:

	bool done;

	suites_panel(const suites_panel&);
	suites_panel& operator=(const suites_panel&);

	virtual void tellCB( Widget, XtPointer );
	virtual void offCB( Widget, XtPointer );
	virtual void onCB( Widget, XtPointer );
};

inline void destroy(suites_panel**) {}
#endif
