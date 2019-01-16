#ifndef edit_H
#define edit_H
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


#include "uiedit.h"

#ifndef panel_H
#include "panel.h"
#endif

#include "input.h"
#include "text_window.h"

class edit : public panel, public edit_form_c, public input, public text_window {
public:
	edit(panel_window&);

	~edit(); // Change to virtual if base class

	virtual const char* name() const { return "Edit"; }
	virtual void show(node&);
	virtual void clear();
	virtual Boolean enabled(node&);
	virtual Widget widget() { return edit_form_c::xd_rootwidget(); }
	virtual Widget tools()  { return tools_; }
	virtual Widget text()  { return text_; }

	virtual void create (Widget parent, char *widget_name = NULL);

private:

	edit(const edit&);
	edit& operator=(const edit&);

	Boolean loading_;
	Boolean preproc_;
	char*   tmp_;

	char *kStart;
	char *kEnd  ;

	void ready(const char*);
	void done(FILE*);

	virtual void changed(node&);

	virtual void changedCB(Widget,XtPointer);
	virtual void preprocCB(Widget,XtPointer);
	virtual void submitCB(Widget,XtPointer);
	virtual void externalCB(Widget,XtPointer);

	virtual void searchCB(Widget ,XtPointer ) 
		{ text_window::open_search();}

	virtual bool can_print() { return true; }
	virtual bool can_save()  { return true; }
	virtual void print()     { text_window::print(); }
	virtual void save()      { text_window::save(); }
};

inline void destroy(edit**) {}
#endif
