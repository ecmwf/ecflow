#ifndef edit_repeat_H
#define edit_repeat_H
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


#include "uiedit_repeat.h"

#ifndef panel_H
#include "panel.h"
#endif

class edit_repeat : public panel, public edit_repeat_form_c {
public:
	edit_repeat(panel_window&);

	~edit_repeat(); // Change to virtual if base class
	virtual const char* name() const { return "Edit"; }
	virtual void show(node&);
	virtual Boolean enabled(node&);
	virtual void clear();
	virtual Widget widget() { return xd_rootwidget(); }
	virtual Widget tools() { return tools_; }

private:

	edit_repeat(const edit_repeat&);
	edit_repeat& operator=(const edit_repeat&);

	bool loading_;
	int  index_; std::string indexs_;
	bool use_text_;

	virtual void applyCB(Widget,XtPointer);
	virtual void browseCB(Widget,XtPointer);
};

inline void destroy(edit_repeat**) {}
#endif
