#ifndef output_H
#define output_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================


#include "uioutput.h"

#ifndef panel_H
#include "panel.h"
#endif

#include "host.h"
#include "text_window.h"


class output : public output_form_c, public text_window, public panel {
public:
	output(panel_window&);

	~output(); // Change to virtual if base class

	virtual const char* name() const { return "Output"; }
	virtual void show(node&);
	virtual void clear();
	virtual Boolean enabled(node&);
	virtual Widget widget() { return output_form_c::xd_rootwidget(); }

	virtual Widget tools()  { return tools_; }
	virtual Widget text()   { return text_; }
	virtual void create (Widget parent, char *widget_name = 0 );

private:
	output(const output&);
	output& operator=(const output&);

	char *file_;

	void load(node&);

	virtual void browseCB(Widget,XtPointer);
	virtual void updateCB(Widget,XtPointer);
	virtual void externalCB(Widget ,XtPointer ) 
	{ text_window::open_viewer();}
	virtual void searchCB(Widget ,XtPointer ) 
		{ text_window::open_search();}

	virtual bool can_print() { return true; }
	virtual bool can_save()  { return true; }
	virtual void print()     { text_window::print(); }
	virtual void save()      { text_window::save(); }
};

inline void destroy(output**) {}
#endif
