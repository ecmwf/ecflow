#ifndef why_H
#define why_H
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


#include "uiwhy.h"

#ifndef panel_H
#include "panel.h"
#endif

#ifndef node_H
#include "node.h"
#endif

class why : public panel, public why_form_c {
public:
	why(panel_window&);

	~why(); // Change to virtual if base class

	virtual const char* name() const { return "Why?"; }
	virtual void show(node&);
	virtual void clear();
	virtual Widget widget() { return xd_rootwidget(); }
	virtual Boolean enabled(node&);

private:
	why(const why&);
	why& operator=(const why&);

	virtual void hyperCB(Widget,XtPointer);
};

inline void destroy(why**) {}
#endif
