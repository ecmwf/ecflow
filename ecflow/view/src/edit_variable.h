#ifndef edit_variable_H
#define edit_variable_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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


#include "uiedit_variable.h"

#ifndef panel_H
#include "panel.h"
#endif

#ifndef node_editor_H
#include "node_editor.h"
#endif

class edit_variable : public panel, public edit_variable_form_c, 
	public node_editor {
public:

	edit_variable(panel_window&);

	~edit_variable(); // Change to virtual if base class

	virtual const char* name() const { return "Edit"; }
	virtual void show(node&);
	virtual Boolean enabled(node&);
	virtual void clear();
	virtual Widget widget() { return xd_rootwidget(); }
	virtual Widget form()   { return xd_rootwidget(); }
	virtual Widget tools()  { return tools_; }

private:

	edit_variable(const edit_variable&);
	edit_variable& operator=(const edit_variable&);

	bool loading_;

	virtual void applyCB(Widget,XtPointer);
	virtual void changedCB(Widget,XtPointer);

};

inline void destroy(edit_variable**) {}
#endif
