#ifndef edit_limit_H
#define edit_limit_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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


#include "uiedit_limit.h"

#ifndef panel_H
#include "panel.h"
#endif

#ifndef node_lister_H
#include "node_lister.h"
#endif

class edit_limit : public panel, public edit_limit_form_c, public node_lister {
public:

	edit_limit(panel_window&);

	~edit_limit(); // Change to virtual if base class

	virtual const char* name() const { return "Edit"; }
	virtual void show(node&);
	virtual Boolean enabled(node&);
	virtual void clear();
	virtual Widget widget() { return xd_rootwidget(); }
	virtual Widget tools() { return tools_; }
private:

	edit_limit(const edit_limit&);
	edit_limit& operator=(const edit_limit&);

	bool loading_;
	char* name_;

	virtual void applyCB(Widget,XtPointer);
	virtual void changedCB(Widget,XtPointer);
	virtual void removeCB(Widget,XtPointer);
	virtual void browseCB(Widget,XtPointer);

	virtual void next(node&);
	virtual void next(const std::string);
};

inline void destroy(edit_limit**) {}
#endif
